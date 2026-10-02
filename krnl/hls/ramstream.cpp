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
	#pragma HLS bind_storage variable=local type=ram_1p impl=bram
	memcpy(local, req_buffer, num_requests * sizeof(Request));

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
