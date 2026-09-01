#ifndef SM_INSERT_HPP
#define SM_INSERT_HPP


#include "../core/node.h"
#ifdef HLS
#include <hls_stream.h>
#include "rdma.hpp"
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
	int            local_qpn,
	hls::stream<insert_tagged_in_t>&  input,
	hls::stream<insert_tagged_out_t>& output,
	hls::stream<pkt256>&              m_axis_tx_meta,
	hls::stream<pkt32>&               s_axis_completion,
	Node                             *resp_in
);
#endif


#endif
