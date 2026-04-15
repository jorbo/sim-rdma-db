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
	int            qpn_table[MAX_KRNL_NODES],
	hls::stream<insert_tagged_in_t>&  input,
	hls::stream<insert_tagged_out_t>& output,
	hls::stream<pkt256>&              m_axis_tx_meta,
	hls::stream<pkt64>&               s_axis_rx_data
);
#endif


#endif
