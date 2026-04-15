#include "ramstream.hpp"


//! @brief Read requests from DRAM and push to the requests FIFO with a `last`
//!        flag on the final element so downstream DATAFLOW processes can
//!        terminate deterministically.
void sm_ramstream_req(
	hls::stream<req_tagged_t>& requests,
	Request *req_buffer,
	int num_requests
) {
	// Pre-scan to find the real count (first NOP or num_requests cap).
	int n = 0;
	scan_loop: for (int i = 0; i < num_requests; i++) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		#pragma HLS pipeline II=1
		if (req_buffer[i].opcode == NOP) break;
		n++;
	}

	// If there are no requests at all, emit a single `last`-only token with
	// opcode NOP so downstream stages can terminate. Otherwise, stream n items
	// marking the final one as last.
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
		t.req  = req_buffer[i];
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
