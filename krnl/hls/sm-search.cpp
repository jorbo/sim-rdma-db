#include "sm-search.hpp"
#include "../core/node.h"


// One pending tree step: "fetch node `ptr` on behalf of window entry `sid`".
struct work_t {
	ap_uint<3> sid;      // index into the window arrays, 0..SEARCH_WINDOW-1
	bptr_t     ptr;      // node to fetch (local or remote)
};

// One RDMA read in flight: which window entry asked, which slot it lands in.
struct pending_t {
	ap_uint<3> sid;
	ap_uint<4> slot;     // 0..RDMA_LANDING_SLOTS-1
};

// Consume a fetched node for search `sid`: either queue the next level or
// record the final result. Returns true when the search finished.
static bool advance(
	ap_uint<3>           sid,
	const Node          &n,
	bkey_t               key,
	bptr_t               cur_ptr[SEARCH_WINDOW],
	search_out_t         win_out[SEARCH_WINDOW],
	hls::stream<work_t> &work)
{
	#pragma HLS inline
	if (is_leaf(cur_ptr[sid])) {
		win_out[sid] = find_value(&n, key);
		return true;
	}
	bstatusval_t r = find_next(&n, key);
	if (r.status != SUCCESS) {
		win_out[sid] = r;
		return true;
	}
	cur_ptr[sid] = r.value.ptr;
	work_t w; w.sid = sid; 
	w.ptr = r.value.ptr;
	work.write(w);
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
	hls::stream<work_t>      work("work");
	hls::stream<pending_t>   pending("pending");
	hls::stream<ap_uint<4> > free_slots("free_slots");
	#pragma HLS stream variable=work       type=fifo depth=SEARCH_WINDOW
	#pragma HLS stream variable=pending    type=fifo depth=RDMA_LANDING_SLOTS
	#pragma HLS stream variable=free_slots type=fifo depth=RDMA_LANDING_SLOTS

	search_tagged_in_t win_in [SEARCH_WINDOW];
	search_out_t       win_out[SEARCH_WINDOW];
	bptr_t             cur_ptr[SEARCH_WINDOW];
	#pragma HLS array_partition variable=win_in  complete
	#pragma HLS array_partition variable=win_out complete
	#pragma HLS array_partition variable=cur_ptr complete

	bool tag_mismatch = false;  

	prefill_slots: for (ap_uint<4> s = 0; s < RDMA_LANDING_SLOTS; s++) {
		free_slots.write(s);
	}

	batch_loop: for (;;) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS/SEARCH_WINDOW

		
		ap_uint<4> admitted  = 0;
		ap_uint<4> remaining = 0;     // payload searches not yet finished
		bool       saw_last  = false;
		admit_loop: while (admitted < SEARCH_WINDOW && !saw_last) {
			search_tagged_in_t in = input.read();   // blocking: nothing is outstanding here
			win_in[admitted]  = in;
			cur_ptr[admitted] = root;
			saw_last = in.last;
			if (in.has_payload) {
				work_t w; w.sid = admitted; 
				w.ptr = root;
				work.write(w);
				remaining++;
			} else {
				win_out[admitted] = search_out_t();
			}
			admitted++;
		}

		event_loop: while (remaining != 0) {
			#pragma HLS pipeline off
			if (!s_axis_completion.empty() && !pending.empty()) {
				pkt32     tok = s_axis_completion.read();
				pending_t p   = pending.read();
				ap_uint<4> landed = (tok.data(3, 0) * 13) & 0xF;   // inverse of 5*slot mod 16
				if (landed != p.slot) {
					tag_mismatch = true;
				}
				Node n = resp_in[landed];                           // AXI read completes here
				free_slots.write(landed);                           // slot reusable only now
				if (advance(p.sid, n, win_in[p.sid].key, cur_ptr, win_out, work)) {
					remaining--;
				}
			}
			else if (!work.empty() && !free_slots.empty()) {
				work_t w = work.read();
				if (bptr_node_id(w.ptr) == local_id) {
					Node n = hbm[bptr_local_addr(w.ptr)];           // local: no RDMA, no slot
					if (advance(w.sid, n, win_in[w.sid].key, cur_ptr, win_out, work)) {
						remaining--;
					}
				} else {
					ap_uint<4>  slot  = free_slots.read();
					ap_uint<64> raddr = (ap_uint<64>)bptr_local_addr(w.ptr) * sizeof(Node);
					m_axis_tx_meta.write(rdma_bram_read_meta(
						(ap_uint<24>)local_qpn, slot * sizeof(Node), raddr, sizeof(Node)));
					pending_t p; p.sid = w.sid; p.slot = slot;
					pending.write(p);
				}
			}
		}

		
		emit_loop: for (ap_uint<4> i = 0; i < admitted; i++) {
			search_tagged_out_t out;
			out.last        = win_in[i].last;
			out.has_payload = win_in[i].has_payload;
			out.val         = win_out[i];
			if (tag_mismatch && out.has_payload) {
				out.val.status = RESTART;
			} 
			output.write(out);
		}

		if (saw_last) break;
	}

	drain_slots: for (ap_uint<4> s = 0; s < RDMA_LANDING_SLOTS; s++) {
		free_slots.read();
	}

}
