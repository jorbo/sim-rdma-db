#ifndef SM_SEARCH_HPP
#define SM_SEARCH_HPP


#include "../core/node.h"
#ifdef HLS
#include <hls_stream.h>
#include "rdma.hpp"
#include "ramstream.hpp"


//! @brief State machine to execute search operations with local/remote dispatch.
//!
//! Runs as a DATAFLOW process: consumes tagged search inputs until `last` is
//! observed, emitting a tagged output for each (filler in → filler out) to
//! keep the pipeline in lockstep.
void sm_search(
	bptr_t const&  root,
	node_id_t      local_id,
	Node          *hbm,
	int            qpn_table[MAX_KRNL_NODES],
	hls::stream<search_tagged_in_t>&  input,
	hls::stream<search_tagged_out_t>& output,
	hls::stream<pkt256>&              m_axis_tx_meta,
	hls::stream<pkt64>&               s_axis_rx_data
);
#endif


#endif
