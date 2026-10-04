#ifndef SM_SEARCH_HPP
#define SM_SEARCH_HPP


#include "../core/node.h"
#ifdef HLS
#include <hls_stream.h>
#include "rdma.hpp"
#include "ramstream.hpp"


//! @brief Level-wise batch search with local/remote dispatch.
//!
//! Runs as a DATAFLOW process. Admits up to SEARCH_WINDOW tagged inputs per
//! batch and walks them down the tree one level at a time: at each level the
//! distinct nodes the batch is on are fetched once each (local HBM read, or
//! one RDMA read into a landing slot, up to RDMA_LANDING_SLOTS in flight)
//! and applied to every key waiting on them. Root and internal fetches are
//! shared across the batch; only the leaf level scales with it. Outputs are
//! emitted in input order so sm_encode stays in lockstep with sm_insert.
//! Stops after the input tagged `last`; filler inputs produce filler outputs.
void sm_search(
	bptr_t         root,
	node_id_t      local_id,
	Node          *hbm,
	int            local_qpn,
	hls::stream<search_tagged_in_t>&  input,
	hls::stream<search_tagged_out_t>& output,
	hls::stream<pkt256>&              m_axis_tx_meta,
	hls::stream<pkt32>&               s_axis_completion,
	Node                             *resp_in
);
#endif


#endif
