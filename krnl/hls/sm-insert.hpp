#ifndef SM_INSERT_HPP
#define SM_INSERT_HPP


#include "../core/node.h"
#ifdef HLS
#include <hls_stream.h>
#include "rdma.hpp"


//! @brief State machine to execute insert operations.
//!
//! Builds a local mem_context_t from hbm for the current insert stage.
//! The RDMA streams are accepted to match the kernel dispatch interface and
//! will be used once write-back over RDMA is implemented.
void sm_insert(
	//! [inout] Root node of the tree to insert into
	bptr_t&        root,
	//! [in]    This FPGA's node_id
	node_id_t      local_id,
	//! [in]    Pointer to local HBM node array
	Node          *hbm,
	//! [in]    QPN table: qpn_table[node_id] → Queue Pair Number for that FPGA
	int            qpn_table[MAX_KRNL_NODES],
	//! [in]    Key/value pairs to insert
	hls::stream<insert_in_t>&  input,
	//! [out]   Status codes from inserts
	hls::stream<insert_out_t>& output,
	//! [out]   RDMA metadata stream for outgoing WRITE commands
	hls::stream<pkt256>&       m_axis_tx_meta,
	//! [out]   RDMA data stream for outgoing inline WRITE payloads
	hls::stream<pkt64>&        m_axis_tx_data,
	//! [in]    RDMA receive stream carrying remote READ response data
	hls::stream<pkt64>&        s_axis_rx_data
);
#endif


#endif
