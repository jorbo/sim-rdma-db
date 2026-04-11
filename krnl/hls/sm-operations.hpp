#ifndef OPERATIONS_HPP
#define OPERATIONS_HPP


#include <hls_stream.h>
#include "../core/operations.h"


//! @brief State machine to decode and redirect incoming instructions
void sm_decode(
	//! [in]  Incoming instructions
	hls::stream<Request>& requests,
	//! [out] Decoded search instructions
	hls::stream<search_in_t>& searchInput,
	//! [out] Decoded insert instructions
	hls::stream<insert_in_t>& insertInput,
	//! [out] Count of received operations
	uint_fast32_t& opsIn
);

//! @brief State machine to encode and queue responses for send
void sm_encode(
	//! [out] Incoming instructions
	hls::stream<Response>& responses,
	//! [in]  Search results
	hls::stream<search_out_t>& searchOutput,
	//! [in]  Insert results
	hls::stream<insert_out_t>& insertOutput,
	//! [out] Count of completed operations
	uint_fast32_t& opsOut
);


#endif
