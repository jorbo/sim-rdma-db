#ifndef OPERATIONS_HPP
#define OPERATIONS_HPP


#include <hls_stream.h>
#include "../core/operations.h"
#include "ramstream.hpp"


//! @brief Route incoming requests to the search or insert sub-pipelines.
//!
//! Every iteration a tagged request is read. SEARCH goes to searchInput,
//! INSERT goes to insertInput. The unused side receives a `has_payload=false`
//! filler so the two parallel pipelines stay in lockstep. The `last` flag is
//! forwarded on both sides exactly once.
void sm_decode(
	hls::stream<req_tagged_t>&         requests,
	hls::stream<search_tagged_in_t>&   searchInput,
	hls::stream<insert_tagged_in_t>&   insertInput
);

//! @brief Merge search and insert results into a single tagged response stream.
//!
//! Reads one element from each input per iteration and emits the real
//! payload(s); filler elements are dropped. Terminates once both inputs have
//! signalled `last`.
void sm_encode(
	hls::stream<resp_tagged_t>&         responses,
	hls::stream<search_tagged_out_t>&   searchOutput,
	hls::stream<insert_tagged_out_t>&   insertOutput
);


#endif
