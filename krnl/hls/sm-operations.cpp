#include "sm-operations.hpp"


void sm_decode(
	hls::stream<req_tagged_t>&       requests,
	hls::stream<search_tagged_in_t>& searchInput,
	hls::stream<insert_tagged_in_t>& insertInput
) {
	decode_loop: for (;;) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		#pragma HLS pipeline II=1
		req_tagged_t in = requests.read();

		search_tagged_in_t s;
		insert_tagged_in_t i;
		s.last = in.last;
		i.last = in.last;
		s.has_payload = false;
		i.has_payload = false;
		s.key  = search_in_t();
		i.pair = KvPair();

		if (in.req.opcode == SEARCH) {
			s.key         = in.req.search;
			s.has_payload = true;
		} else if (in.req.opcode == INSERT) {
			i.pair        = in.req.insert;
			i.has_payload = true;
		}

		searchInput.write(s);
		insertInput.write(i);

		if (in.last) break;
	}
}


void sm_encode(
	hls::stream<resp_tagged_t>&       responses,
	hls::stream<search_tagged_out_t>& searchOutput,
	hls::stream<insert_tagged_out_t>& insertOutput
) {
	encode_loop: for (;;) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		#pragma HLS pipeline II=1
		search_tagged_out_t s = searchOutput.read();
		insert_tagged_out_t i = insertOutput.read();
		const bool final_iter = s.last && i.last;

		// At most one token per request: the final payload carries `last`,
		// and a payload-free terminator is sent only when the final
		// iteration has no payload (e.g. an empty run). A separate
		// terminator after every run made a full window (NUM_REQUESTS
		// requests) emit NUM_REQUESTS+1 tokens into the NUM_REQUESTS-deep
		// `responses` FIFO, and the kernel hung at exactly 256 requests on
		// hardware while 255 passed.
		if (s.has_payload) {
			resp_tagged_t t;
			t.resp        = encode_search_resp(s.val);
			t.has_payload = true;
			t.last        = final_iter && !i.has_payload;
			responses.write(t);
		}
		if (i.has_payload) {
			resp_tagged_t t;
			t.resp        = encode_insert_resp(i.status);
			t.has_payload = true;
			t.last        = final_iter;
			responses.write(t);
		}

		if (final_iter) {
			if (!s.has_payload && !i.has_payload) {
				resp_tagged_t term;
				term.resp        = Response();
				term.has_payload = false;
				term.last        = true;
				responses.write(term);
			}
			break;
		}
	}
}
