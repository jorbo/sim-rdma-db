#include "sm-search.hpp"
#include "../core/node.h"


// Independent per-request traversal.
//
// Each of the SEARCH_WINDOW lanes holds one search and walks it down the
// tree on its own: one node fetch at a time (local HBM read or one RDMA
// read into a landing slot), the next one issued as soon as the previous
// node lands. This is the client model of Ziegler et al. (SIGMOD'19,
// fine-grained design): a lane is a client with one request outstanding,
// no fetch is shared between lanes, and SEARCH_WINDOW is the client count.
// Lanes form a ring: a lane is refilled from the input once its result has
// been emitted, so results leave in input order and sm_encode stays in
// lockstep with sm_insert.

// One RDMA read in flight: which lane, which landing slot.
struct pending_t {
	ap_uint<4> lane;
	ap_uint<4> slot;
};

// Remote inner-node cache (SEARCH_NODE_CACHE, off by default).
//
// The paper's fine-grained design fetches every level over the wire; its
// caching experiment (appendix A.4) keeps upper levels on the compute
// side. With the cache on, inner nodes fetched over RDMA are kept in a
// small direct-mapped on-chip cache and later lookups apply them without a
// round trip. Leaves are never cached (their values are what the search
// returns) and neither are local nodes (an HBM read is cheap, and
// sm_insert modifies them concurrently).
//
// A cached inner node can go stale if its owner splits it. Searches stay
// correct the B-link way: find_next follows `next` when the key is past
// the node's last key, so a stale parent at worst costs extra right-link
// hops on the level below. The cache lives for one kernel invocation:
// the host reloads the tree between runs (reset=true), which a persistent
// cache would need an explicit flush for.
#if SEARCH_NODE_CACHE
typedef ap_uint<6> cache_idx_t;   // log2(NODE_CACHE_SIZE)

static cache_idx_t cache_index(bptr_t p) {
	#pragma HLS inline
	return (bptr_local_addr(p) ^ ((bptr_t)bptr_node_id(p) << 3)) & (NODE_CACHE_SIZE - 1);
}
#endif

// Node `n`, fetched from `node_ptr`, has landed for one lane's search.
// Leaves and failed lookups finish the lane; otherwise `ptr` becomes the
// child (or right sibling) and the lane needs another fetch (returns true).
static bool descend(
	const Node   &n,
	bptr_t        node_ptr,
	bkey_t        key,
	bptr_t       &ptr,
	bool         &done,
	search_out_t &out)
{
	#pragma HLS inline
	if (is_leaf(node_ptr)) {
		out  = find_value(&n, key);
		done = true;
		return false;
	}
	bstatusval_t r = find_next(&n, key);
	if (r.status == SUCCESS) {
		ptr = r.value.ptr;
		return true;
	}
	out  = r;
	done = true;
	return false;
}

void sm_search(
	bptr_t         root,
	node_id_t      local_id,
	Node          *hbm,
	int            local_qpn,
	hls::stream<search_tagged_in_t>&  input,
	hls::stream<search_tagged_out_t>& output,
	hls::stream<pkt256>&              m_axis_tx_meta,
	hls::stream<pkt32>&               s_axis_completion,
	Node                             *resp_in
) {
	hls::stream<ap_uint<4> > work("work");          // lanes waiting to fetch lane_ptr
	hls::stream<pending_t>   pending("pending");
	hls::stream<ap_uint<4> > free_slots("free_slots");
	#pragma HLS stream variable=work       type=fifo depth=SEARCH_WINDOW
	#pragma HLS stream variable=pending    type=fifo depth=RDMA_LANDING_SLOTS
	#pragma HLS stream variable=free_slots type=fifo depth=RDMA_LANDING_SLOTS

	search_tagged_in_t lane_in  [SEARCH_WINDOW];
	search_out_t       lane_out [SEARCH_WINDOW];
	bptr_t             lane_ptr [SEARCH_WINDOW];
	bool               lane_done[SEARCH_WINDOW];
	#pragma HLS array_partition variable=lane_in   complete
	#pragma HLS array_partition variable=lane_out  complete
	#pragma HLS array_partition variable=lane_ptr  complete
	#pragma HLS array_partition variable=lane_done complete

	// Lane indices are 4 bits and wrap with the ring; `busy` runs 0..16
	// and must be 5 bits or `busy < SEARCH_WINDOW` can never be false.
	ap_uint<4> head = 0;             // oldest lane, next to emit
	ap_uint<4> tail = 0;             // next lane to fill
	ap_uint<5> busy = 0;             // lanes in [head, tail)
	bool saw_last     = false;
	bool tag_mismatch = false;       // sticky: a token's tag disagreed with its pending slot

#if SEARCH_NODE_CACHE
	Node   cache_node [NODE_CACHE_SIZE];
	bptr_t cache_tag  [NODE_CACHE_SIZE];
	bool   cache_valid[NODE_CACHE_SIZE];
	#pragma HLS array_partition variable=cache_valid complete

	cache_clear: for (ap_uint<7> c = 0; c < NODE_CACHE_SIZE; c++) {
		#pragma HLS unroll
		cache_valid[c] = false;
	}
#endif

	prefill_slots: for (ap_uint<5> s = 0; s < RDMA_LANDING_SLOTS; s++) {
		free_slots.write(s);
	}

	// One action per iteration: emit > admit > retire > issue. A lane has
	// at most one fetch in flight, so SEARCH_WINDOW <= RDMA_LANDING_SLOTS
	// means a waiting lane never runs out of landing slots.
	event_loop: for (;;) {
		#pragma HLS pipeline off
		#pragma HLS loop_tripcount max=NUM_REQUESTS*(MAX_LEVELS+3)
		const bool can_emit   = busy != 0 && lane_done[head];
		const bool can_admit  = !saw_last && busy < SEARCH_WINDOW && !input.empty();
		const bool can_retire = !s_axis_completion.empty() && !pending.empty();
		const bool can_issue  = !work.empty() && !free_slots.empty();

		if (can_emit) {
			search_tagged_out_t out;
			out.last        = lane_in[head].last;
			out.has_payload = lane_in[head].has_payload;
			out.val         = lane_out[head];
			if (tag_mismatch && out.has_payload) out.val.status = RESTART;
			output.write(out);
			head++;
			busy--;
			if (out.last) break;       // nothing is admitted after `last`
		}
		else if (can_admit) {
			search_tagged_in_t in = input.read();
			lane_in  [tail] = in;
			lane_ptr [tail] = root;
			lane_done[tail] = !in.has_payload;     // fillers are finished on arrival
			if (in.has_payload) work.write(tail);
			else                lane_out[tail] = search_out_t();
			saw_last = in.last;
			tail++;
			busy++;
		}
		else if (can_retire) {
			pkt32     tok = s_axis_completion.read();
			pending_t p   = pending.read();
			// DataMover tag = address[6:3] of the landing address
			// (mem_single_inf.sv) = 5*slot mod 16; 13 is its inverse mod 16.
			ap_uint<4> landed = (tok.data(3, 0) * 13) & 0xF;
			if (landed != p.slot) tag_mismatch = true;
			Node n = resp_in[landed];                    // AXI read completes here
			free_slots.write(landed);                    // slot reusable only now
			const bptr_t fetched = lane_ptr[p.lane];
#if SEARCH_NODE_CACHE
			if (landed == p.slot && !is_leaf(fetched)) { // never cache a node from a mismatched slot
				cache_idx_t c  = cache_index(fetched);
				cache_node [c] = n;
				cache_tag  [c] = fetched;
				cache_valid[c] = true;
			}
#endif
			if (descend(n, fetched, lane_in[p.lane].key, lane_ptr[p.lane], lane_done[p.lane], lane_out[p.lane]))
				work.write(p.lane);
		}
		else if (can_issue) {
			const ap_uint<4> lane = work.read();
			const bptr_t     ptr  = lane_ptr[lane];
#if SEARCH_NODE_CACHE
			const cache_idx_t c   = cache_index(ptr);
			const bool        hit = cache_valid[c] && cache_tag[c] == ptr;
#endif
			if (bptr_node_id(ptr) == local_id) {
				Node n = hbm[bptr_local_addr(ptr)];        // local: no RDMA, no slot
				if (descend(n, ptr, lane_in[lane].key, lane_ptr[lane], lane_done[lane], lane_out[lane]))
					work.write(lane);
			}
#if SEARCH_NODE_CACHE
			else if (hit) {
				Node n = cache_node[c];                    // cached remote inner node: no round trip
				if (descend(n, ptr, lane_in[lane].key, lane_ptr[lane], lane_done[lane], lane_out[lane]))
					work.write(lane);
			}
#endif
			else {
				ap_uint<4>  slot  = free_slots.read();
				ap_uint<64> raddr = (ap_uint<64>)bptr_local_addr(ptr) * sizeof(Node);
				// Non-blocking issue; the completion is consumed by the retire
				// branch above, never waited for here (see the PROTOCOL-region
				// history in git for why that order was once a hang).
				m_axis_tx_meta.write(rdma_bram_read_meta(
					(ap_uint<24>)local_qpn, slot * sizeof(Node), raddr, sizeof(Node)));
				pending_t p; p.lane = lane; p.slot = slot;
				pending.write(p);
			}
		}
	}

	drain_slots: for (ap_uint<5> s = 0; s < RDMA_LANDING_SLOTS; s++) {
		free_slots.read();
	}
}
