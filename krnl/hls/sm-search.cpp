#include "sm-search.hpp"
#include "../core/node.h"


//! Overlay a Node on an array of 64-bit words for stream-based reassembly.
union NodeWords {
	Node        node;
	ap_uint<64> words[(sizeof(Node) + 7) / 8];
	NodeWords() {}
};


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

	ap_uint<64> raddr = (ap_uint<64>)laddr * sizeof(Node);
	rdma_bram_read(
		(ap_uint<24>)qpn_table[nid],
		/*laddr=*/0,
		raddr,
		sizeof(Node),
		tx_meta
	);

	NodeWords buf;
	const int nwords = (sizeof(Node) + 7) / 8;
	for (int i = 0; i < nwords; i++) {
		#pragma HLS pipeline II=1
		pkt64 flit   = rx_data.read();
		buf.words[i] = flit.data;
	}
	return buf.node;
}


static bstatusval_t search_one(
	bkey_t key,
	bptr_t root,
	node_id_t local_id,
	Node *hbm,
	int qpn_table[MAX_KRNL_NODES],
	hls::stream<pkt256>& tx_meta,
	hls::stream<pkt64>&  rx_data
) {
	bptr_t       ptr = root;
	bstatusval_t result;

	while (!is_leaf(ptr)) {
		#pragma HLS loop_tripcount max=MAX_LEVELS
		Node n = fetch_node(ptr, local_id, hbm, qpn_table, tx_meta, rx_data);
		result = find_next(&n, key);
		if (result.status != SUCCESS) {
			return result;
		}
		ptr = result.value.ptr;
	}

	Node leaf = fetch_node(ptr, local_id, hbm, qpn_table, tx_meta, rx_data);
	return find_value(&leaf, key);
}


void sm_search(
	bptr_t const&  root,
	node_id_t      local_id,
	Node          *hbm,
	int            qpn_table[MAX_KRNL_NODES],
	hls::stream<search_tagged_in_t>&  input,
	hls::stream<search_tagged_out_t>& output,
	hls::stream<pkt256>&              m_axis_tx_meta,
	hls::stream<pkt64>&               s_axis_rx_data
) {
	search_loop: for (;;) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		search_tagged_in_t in = input.read();

		search_tagged_out_t out;
		out.last        = in.last;
		out.has_payload = in.has_payload;
		out.val         = search_out_t();

		if (in.has_payload) {
			out.val = search_one(in.key, root, local_id, hbm, qpn_table,
			                     m_axis_tx_meta, s_axis_rx_data);
		}
		output.write(out);

		if (in.last) break;
	}
}
