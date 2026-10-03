#include "ramstream.hpp"
#include <cstring>

void sm_ramstream_req(
	hls::stream<req_tagged_t>& requests,
	Request *req_buffer,
	int num_requests
) {
	// Fetch the whole request window in one AXI burst. The old scan loop's
	// data-dependent break stopped HLS from bursting (one ~70-cycle round
	// trip per 16-byte Request), and the emit loop then re-read every entry.
	Request local[NUM_REQUESTS];
	copy_loop: for (int i = 0; i < num_requests; i++) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		#pragma HLS pipeline II=1
		local[i] = req_buffer[i];
	}

	int n = 0;
	scan_loop: for (int i = 0; i < num_requests; i++) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		#pragma HLS pipeline II=1
		if (local[i].opcode == NOP) break;
		n++;
	}

	if (n == 0) {
		req_tagged_t t;
		t.req.opcode = NOP;
		t.last       = true;
		requests.write(t);
		return;
	}

	emit_loop: for (int i = 0; i < n; i++) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		#pragma HLS pipeline II=1
		req_tagged_t t;
		t.req  = local[i];
		t.last = (i == n - 1);
		requests.write(t);
	}
}



//! @brief Drain the responses FIFO to DRAM until a `last` token is seen.
void sm_ramstream_resp(
	hls::stream<resp_tagged_t>& responses,
	Response *resp_buffer
) {
	int offset = 0;
	drain_loop: for (;;) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		#pragma HLS pipeline II=1
		resp_tagged_t t = responses.read();
		// Only record "real" responses (decoder may send last-only sentinels
		// to flush a pipeline that had no work).
		if (t.has_payload) {
			resp_buffer[offset++] = t.resp;
		}
		if (t.last) break;
	}
}


void reset_ramstream_offsets() {
	// No-op: state is now fully local to each DATAFLOW invocation.
}
