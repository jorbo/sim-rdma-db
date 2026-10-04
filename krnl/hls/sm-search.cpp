#include "sm-search.hpp"
#include "../core/node.h"


// Level-wise batch traversal.
//
// A batch of up to SEARCH_WINDOW keys moves down the tree one level at a
// time. At each level the kernel collects the DISTINCT nodes the batch is
// sitting on, fetches each of them once (local HBM read or one RDMA read
// into a landing slot), and when a node lands applies find_next/find_value
// to every key that was waiting on it. All keys share the root fetch, most
// share the internal-level fetches, only the leaf level scales with the
// batch. Fetches within a level overlap (up to RDMA_LANDING_SLOTS in
// flight); there is a barrier between levels.

// One fetch to issue: distinct-pointer index `uidx` at this level, node `ptr`.
struct work_t {
	ap_uint<4> uidx;
	bptr_t     ptr;
};

// One RDMA read in flight: which distinct pointer, which landing slot.
struct pending_t {
	ap_uint<4> uidx;
	ap_uint<4> slot;
};

// A node for distinct pointer `uidx` has landed: advance every key in the
// window that is waiting on that pointer. Returns nothing; keys that reach
// a result are marked done, the rest get next_ptr for the next level.
static void apply_node(
	ap_uint<4>               uidx,
	const Node              &n,
	ap_uint<4>               admitted,
	const search_tagged_in_t win_in [SEARCH_WINDOW],
	const bptr_t             uniq_ptr[SEARCH_WINDOW],
	bptr_t                   cur_ptr [SEARCH_WINDOW],
	bptr_t                   next_ptr[SEARCH_WINDOW],
	bool                     done    [SEARCH_WINDOW],
	search_out_t             win_out [SEARCH_WINDOW])
{
	#pragma HLS inline
	const bptr_t node_ptr = uniq_ptr[uidx];
	apply_loop: for (ap_uint<4> i = 0; i < SEARCH_WINDOW; i++) {
		#pragma HLS pipeline II=1
		if (i < admitted && !done[i] && cur_ptr[i] == node_ptr) {
			const bkey_t key = win_in[i].key;
			if (is_leaf(node_ptr)) {
				win_out[i] = find_value(&n, key);
				done[i]    = true;
			} else {
				bstatusval_t r = find_next(&n, key);
				if (r.status == SUCCESS) {
					next_ptr[i] = r.value.ptr;
				} else {
					win_out[i] = r;
					done[i]    = true;
				}
			}
		}
	}
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
	hls::stream<work_t>      work("work");
	hls::stream<pending_t>   pending("pending");
	hls::stream<ap_uint<4> > free_slots("free_slots");
	#pragma HLS stream variable=work       type=fifo depth=SEARCH_WINDOW
	#pragma HLS stream variable=pending    type=fifo depth=RDMA_LANDING_SLOTS
	#pragma HLS stream variable=free_slots type=fifo depth=RDMA_LANDING_SLOTS

	search_tagged_in_t win_in  [SEARCH_WINDOW];
	search_out_t       win_out [SEARCH_WINDOW];
	bptr_t             cur_ptr [SEARCH_WINDOW];
	bptr_t             next_ptr[SEARCH_WINDOW];
	bool               done    [SEARCH_WINDOW];
	bptr_t             uniq_ptr[SEARCH_WINDOW];
	#pragma HLS array_partition variable=win_in   complete
	#pragma HLS array_partition variable=win_out  complete
	#pragma HLS array_partition variable=cur_ptr  complete
	#pragma HLS array_partition variable=next_ptr complete
	#pragma HLS array_partition variable=done     complete
	#pragma HLS array_partition variable=uniq_ptr complete

	bool tag_mismatch = false;   // sticky: a token's tag disagreed with its pending slot

	prefill_slots: for (ap_uint<4> s = 0; s < RDMA_LANDING_SLOTS; s++) {
		free_slots.write(s);
	}

	batch_loop: for (;;) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS/SEARCH_WINDOW

		// ---- admit up to SEARCH_WINDOW inputs, stop at `last` ----
		ap_uint<4> admitted = 0;
		bool       saw_last = false;
		admit_loop: while (admitted < SEARCH_WINDOW && !saw_last) {
			search_tagged_in_t in = input.read();   // blocking: nothing is outstanding here
			win_in [admitted] = in;
			cur_ptr[admitted] = root;
			done   [admitted] = !in.has_payload;    // fillers are finished on arrival
			if (!in.has_payload) win_out[admitted] = search_out_t();
			saw_last = in.last;
			admitted++;
		}

		// ---- walk the batch down one level at a time ----
		level_loop: for (;;) {
			#pragma HLS loop_tripcount max=MAX_LEVELS+1

			// 1. distinct pointers among the keys still walking
			ap_uint<4> n_uniq = 0;
			dedup_loop: for (ap_uint<4> i = 0; i < SEARCH_WINDOW; i++) {
				#pragma HLS pipeline II=1
				if (i < admitted && !done[i]) {
					bool seen = false;
					match_loop: for (ap_uint<4> j = 0; j < SEARCH_WINDOW; j++) {
						#pragma HLS unroll
						if (j < n_uniq && uniq_ptr[j] == cur_ptr[i]) seen = true;
					}
					if (!seen) {
						uniq_ptr[n_uniq] = cur_ptr[i];
						work_t w; w.uidx = n_uniq; w.ptr = cur_ptr[i];
						work.write(w);
						n_uniq++;
					}
				}
			}
			if (n_uniq == 0) break;          // every key has a result

			// 2. fetch the distinct nodes, overlapping remote reads; apply
			//    each node to all keys waiting on it as it lands.
			ap_uint<4> outstanding  = n_uniq;
			bool       prefer_issue = false;
			event_loop: while (outstanding != 0) {
				#pragma HLS pipeline off
				const bool can_retire = !s_axis_completion.empty() && !pending.empty();
				const bool can_issue  = !work.empty() && !free_slots.empty();

				if (can_retire && !(can_issue && prefer_issue)) {
					pkt32     tok = s_axis_completion.read();
					pending_t p   = pending.read();
					// DataMover tag = address[6:3] of the landing address
					// (mem_single_inf.sv) = 5*slot mod 16; 13 is its inverse mod 16.
					ap_uint<4> landed = (tok.data(3, 0) * 13) & 0xF;
					if (landed != p.slot) tag_mismatch = true;
					Node n = resp_in[landed];                    // AXI read completes here
					free_slots.write(landed);                    // slot reusable only now
					apply_node(p.uidx, n, admitted, win_in, uniq_ptr, cur_ptr, next_ptr, done, win_out);
					outstanding--;
					prefer_issue = true;                         // let a queued issue go next
				}
				else if (can_issue) {
					work_t w = work.read();
					if (bptr_node_id(w.ptr) == local_id) {
						Node n = hbm[bptr_local_addr(w.ptr)];    // local: no RDMA, no slot
						apply_node(w.uidx, n, admitted, win_in, uniq_ptr, cur_ptr, next_ptr, done, win_out);
						outstanding--;
					} else {
						ap_uint<4>  slot  = free_slots.read();
						ap_uint<64> raddr = (ap_uint<64>)bptr_local_addr(w.ptr) * sizeof(Node);
						// Non-blocking issue; the completion is consumed by the retire
						// branch above, never waited for here (see the PROTOCOL-region
						// history in git for why that order was once a hang).
						m_axis_tx_meta.write(rdma_bram_read_meta(
							(ap_uint<24>)local_qpn, slot * sizeof(Node), raddr, sizeof(Node)));
						pending_t p; p.uidx = w.uidx; p.slot = slot;
						pending.write(p);
					}
					prefer_issue = false;
				}
			}

			// 3. level barrier: everyone still walking moves to its child
			advance_loop: for (ap_uint<4> i = 0; i < SEARCH_WINDOW; i++) {
				#pragma HLS unroll
				if (i < admitted && !done[i]) cur_ptr[i] = next_ptr[i];
			}
		}

		// ---- emit in input order ----
		emit_loop: for (ap_uint<4> i = 0; i < admitted; i++) {
			search_tagged_out_t out;
			out.last        = win_in[i].last;
			out.has_payload = win_in[i].has_payload;
			out.val         = win_out[i];
			if (tag_mismatch && out.has_payload) out.val.status = RESTART;
			output.write(out);
		}

		if (saw_last) break;
	}

	drain_slots: for (ap_uint<4> s = 0; s < RDMA_LANDING_SLOTS; s++) {
		free_slots.read();
	}
}
