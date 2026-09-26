#ifndef SM_INSERT_HPP
#define SM_INSERT_HPP


#include "../core/node.h"
#ifdef HLS
#include <hls_stream.h>
#include "ramstream.hpp"


//! @brief State machine to execute insert operations.
//!
//! Runs as a DATAFLOW process: reads tagged inputs, performs inserts, writes
//! tagged outputs. Updates `root` in place (root is a scalar argument shared
//! across DATAFLOW processes via immutable convention — only sm_insert writes
//! to it).
void sm_insert(
	bptr_t&        root,
	node_id_t      local_id,
	Node          *hbm,
	hls::stream<insert_tagged_in_t>&  input,
	hls::stream<insert_tagged_out_t>& output
);
#endif


#endif
