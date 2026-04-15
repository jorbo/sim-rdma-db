#include "sm-operations.hpp"


void sm_decode(
	hls::stream<Request>& requests,
	hls::stream<search_in_t>& searchInput,
	hls::stream<insert_in_t>& insertInput,
	uint_fast32_t& opsIn
) {
	Request req;
	KvPair pair;
	if (requests.read_nb(req)) {
		switch (req.opcode) {
			case SEARCH:
				searchInput.write(req.search);
				opsIn++;
				break;
			case INSERT:
				insertInput.write(req.insert);
				opsIn++;
				break;
		}
	}
}

void sm_encode(
	hls::stream<Response>& responses,
	hls::stream<search_out_t>& searchOutput,
	hls::stream<insert_out_t>& insertOutput,
	uint_fast32_t& opsOut
) {
	search_out_t searchResultRaw;
	insert_out_t insertResultRaw;
	Response searchResultEnc, insertResultEnc;
	if (searchOutput.read_nb(searchResultRaw)) {
		responses.write(encode_search_resp(searchResultRaw));
		opsOut++;
	}
	if (insertOutput.read_nb(insertResultRaw)) {
		responses.write(encode_insert_resp(insertResultRaw));
		opsOut++;
	}
}
