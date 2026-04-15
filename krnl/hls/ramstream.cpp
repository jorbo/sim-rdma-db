#include "ramstream.hpp"
#include <iostream>


void sm_ramstream_req(
	hls::stream<Request>& requests,
	Request *req_buffer,
	bool do_reset
) {
	static enum {IDLE, READ, DONE} state = IDLE;
	static int req_offset = 0;
	Request req;

	if (do_reset) {
		state = IDLE;
		req_offset = 0;
		return;
	}

	switch (state) {
		case IDLE:
			state = READ;
			break;
		case READ:
			req = req_buffer[req_offset++];
			if (req.opcode == NOP) {
				state = DONE;
			} else {
				if (requests.write_nb(req)) {
					if (req_offset >= NUM_REQUESTS) {
						state = DONE;
					}
				} else {
					// FIFO full — back off and retry same entry next cycle
					req_offset--;
				}
			}
			break;
		case DONE:
			break;
	}
}


void sm_ramstream_resp(
	hls::stream<Response>& responses,
	Response *resp_buffer,
	bool do_reset
) {
	static enum {IDLE, WRITE} state = IDLE;
	static int resp_offset = 0;
	Response resp;

	if (do_reset) {
		state = IDLE;
		resp_offset = 0;
		return;
	}

	switch (state) {
		case IDLE:
		case WRITE:
			if (responses.read_nb(resp)) {
				resp_buffer[resp_offset++] = resp;
			}
			break;
	}
}


void reset_ramstream_offsets() {
	// No-op: reset is now handled via do_reset inside the kernel's reset path,
	// which correctly updates RTL registers in cosim. This stub is kept so
	// existing testbench code that calls reset_ramstream_offsets() still compiles.
}
