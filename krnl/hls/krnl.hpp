#ifndef KRNL_HPP
#define KRNL_HPP

#include "rdma.hpp"
#include <hls_stream.h>
#include "../core/node.h"
#include "../core/operations.h"


void krnl(
	//! [inout] Encoded root address shared with the caller
	bptr_t       *root,
	//! [inout] Pointer to on-chip high-bandwidth memory
	Node         *hbm,
	//! [in]    Buffer to hold the list of operation requests
	Request      *req_buffer,
	//! [out]   Buffer to hold responses from operations
	Response     *resp_buffer,
	//! [in]    Maximum number of main loop iterations to attempt to execute
	int           loop_max,
	//! [in]    Maximum number of operations to attempt to execute
	int           op_max,
	//! [in]    Reset operation counter register
	bool          reset,
	//! [in]    This FPGA's node_id within the distributed tree (0 = local root)
	node_id_t     my_node_id,
	//! [in]    QPN table: qpn_table[node_id] gives the Queue Pair Number for
	//!         that remote FPGA.  Written by the host after connection setup.
	//!         If your HLS tool does not support s_axilite arrays, change this
	//!         to `int *qpn_table` with an m_axi bundle and copy it locally.
	int           qpn_table[MAX_KRNL_NODES],
	//! [out]   RDMA metadata stream for outgoing READ/WRITE commands
	hls::stream<pkt256>& m_axis_tx_meta,
	//! [out]   RDMA data stream for outgoing inline WRITE payloads
	hls::stream<pkt64>&  m_axis_tx_data,
	//! [in]    RDMA receive stream carrying incoming READ response data
	hls::stream<pkt64>&  s_axis_rx_data
);


#endif
