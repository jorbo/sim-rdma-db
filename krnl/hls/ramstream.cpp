#include "ramstream.hpp"
#include <iostream>


static uint_fast64_t req_offset = 0;
static uint_fast64_t resp_offset = 0;


void sm_ramstream_req(
	hls::stream<Request>& requests,
	Request *req_buffer
) {
	static enum {IDLE, READ, DONE} state = IDLE;
	Request req;

	switch (state) {
		case IDLE:
			state = READ;
			break;
		case READ:
			if (!requests.full()) {
				req = req_buffer[req_offset++];
				if (req.opcode == NOP) {
					state = DONE;
				} else {
					requests.write(req);
					// Buffer overrun
					if (req_offset >= NUM_REQUESTS*sizeof(Request)) {
						state = DONE;
					}
				}
			}
			break;
		case DONE:
			// A reset has occurred
			if (req_offset == 0) {
				state = IDLE;
			}
			break;
	}
}


void sm_ramstream_resp(
	hls::stream<Response>& responses,
	Response *resp_buffer
) {
	static enum {IDLE, WRITE, RESET} state = IDLE;
	Response resp;

	switch (state) {
		case IDLE:
			if (!responses.empty()) {
				state = WRITE;
			}
			break;
		case WRITE:
			responses.read(resp);
			resp_buffer[resp_offset++] = resp;
			if (responses.empty()) {
				state = IDLE;
			}
			break;
	}
}


void reset_ramstream_offsets() {
	req_offset = 0;
	resp_offset = 0;
}
