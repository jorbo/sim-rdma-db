#ifndef SM_INSERT_HPP
#define SM_INSERT_HPP


extern "C" {
#include "../core/node.h"
};
#ifdef HLS
#include <hls_stream.h>
#endif


#ifdef HLS
//! @brief State machine to execute insert operations
void sm_insert(
	//! [in]  Root node of the tree to insert into
	bptr_t& root,
	//! [in]  Key/value pairs to insert
	hls::stream<insert_in_t>& input,
	//! [out] Status codes from inserts
	hls::stream<insert_out_t>& output,
	Node **memory
);
#endif


#endif
