#include "sm-search.hpp"
extern "C" {
#include "../core/node.h"
};


//! Overlay a Node on an array of 64-bit words for stream-based reassembly.
union NodeWords {
	Node        node;
	ap_uint<64> words[(sizeof(Node) + 7) / 8];
};


//! @brief Fetch a single Node by encoded address.
//!
//! If the node_id embedded in @p addr matches @p local_id the node is read
//! directly from @p hbm.  Otherwise an RDMA READ command is issued on
//! @p tx_meta and the response is reassembled from consecutive 64-bit flits
//! on @p rx_data.
static Node fetch_node(
	bptr_t       addr,
	node_id_t    local_id,
	Node        *hbm,
	int          qpn_table[MAX_KRNL_NODES],
	hls::stream<pkt256>& tx_meta,
	hls::stream<pkt64>&  rx_data
) {
	#pragma HLS inline
	node_id_t nid   = bptr_node_id(addr);
	bptr_t    laddr = bptr_local_addr(addr);

	if (nid == local_id) {
		return hbm[laddr];
	}

	// Issue one-sided RDMA READ: fetch sizeof(Node) bytes from the remote
	// FPGA's HBM at byte offset laddr*sizeof(Node).
	ap_uint<64> raddr = (ap_uint<64>)laddr * sizeof(Node);
	rdma_bram_read(
		(ap_uint<24>)qpn_table[nid],
		/*laddr=*/0,
		raddr,
		sizeof(Node),
		tx_meta
	);

	// Reassemble the Node from consecutive 64-bit receive flits.
	NodeWords buf;
	const int nwords = (sizeof(Node) + 7) / 8;
	for (int i = 0; i < nwords; i++) {
		#pragma HLS pipeline II=1
		pkt64 flit   = rx_data.read();
		buf.words[i] = flit.data;
	}
	return buf.node;
}


void sm_search(
	bptr_t const&  root,
	node_id_t      local_id,
	Node          *hbm,
	int            qpn_table[MAX_KRNL_NODES],
	hls::stream<search_in_t>&  input,
	hls::stream<search_out_t>& output,
	hls::stream<pkt256>&       m_axis_tx_meta,
	hls::stream<pkt64>&        s_axis_rx_data
) {
	if (input.empty()) return;

	bkey_t       key = input.read();
	bptr_t       ptr = root;
	bstatusval_t result;

	// Traverse inner nodes, fetching each from local HBM or via RDMA.
	while (!is_leaf(ptr)) {
		Node n = fetch_node(ptr, local_id, hbm, qpn_table, m_axis_tx_meta, s_axis_rx_data);
		result = find_next(&n, key);
		if (result.status != SUCCESS) {
			output.write(result);
			return;
		}
		ptr = result.value.ptr;
	}

	// Read and search the leaf node.
	Node leaf = fetch_node(ptr, local_id, hbm, qpn_table, m_axis_tx_meta, s_axis_rx_data);
	output.write(find_value(&leaf, key));
}
