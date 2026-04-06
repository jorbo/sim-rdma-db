#ifndef SM_SEARCH_HPP
#define SM_SEARCH_HPP


extern "C" {
#include "../core/node.h"
};
#ifdef HLS
#include <hls_stream.h>
#endif


#ifdef HLS
//! @brief State machine to execute search operations
void sm_search(
	//! [in]  Root node of the tree to search
	bptr_t const& root,
	//! [in]  Keys to search for
	hls::stream<search_in_t>& input,
	//! [out] Results from searches
	hls::stream<search_out_t>& output,
	Node const **memory
);
#endif


#endif
