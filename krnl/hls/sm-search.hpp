#ifndef SM_SEARCH_HPP
#define SM_SEARCH_HPP


#include "../core/node.h"
#ifdef HLS
#include <hls_stream.h>
#include "rdma.hpp"


//! @brief State machine to execute search operations with local/remote dispatch.
//!
//! For nodes whose bptr_node_id matches local_id, HBM is accessed directly.
//! For remote nodes an RDMA READ is issued on m_axis_tx_meta and the Node is
//! reassembled from 64-bit flits arriving on s_axis_rx_data.
void sm_search(
	//! [in]  Root node of the tree to search
	bptr_t const&  root,
	//! [in]  This FPGA's node_id
	node_id_t      local_id,
	//! [in]  Pointer to local HBM node array
	Node          *hbm,
	//! [in]  QPN table: qpn_table[node_id] → Queue Pair Number for that FPGA
	int            qpn_table[MAX_KRNL_NODES],
	//! [in]  Keys to search for
	hls::stream<search_in_t>&  input,
	//! [out] Results from searches
	hls::stream<search_out_t>& output,
	//! [out] RDMA metadata stream for outgoing READ commands
	hls::stream<pkt256>&       m_axis_tx_meta,
	//! [in]  RDMA receive stream carrying remote READ response data
	hls::stream<pkt64>&        s_axis_rx_data
);
#endif


#endif
