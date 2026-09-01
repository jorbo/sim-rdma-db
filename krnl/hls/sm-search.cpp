#include "sm-search.hpp"
#include "../core/node.h"


//! @brief Fetch a Node from local HBM (if owned by us) or from a remote FPGA
//!        via RDMA + HBM-resident response slot.
//!
//! First-light single-in-flight model: each remote fetch (a) emits an RDMA-read
//! metadata beat, (b) blocks on one 32-bit completion token from
//! `m_axis_op_completion` (the DataMover-write status pulse from rocetest_krnl),
//! and (c) reads the freshly-DMAed Node from resp_in[0]. No ring buffer.
static Node fetch_node(
	bptr_t       addr,
	node_id_t    local_id,
	Node        *hbm,
	int          local_qpn,
	hls::stream<pkt256>& tx_meta,
	hls::stream<pkt32>&  completion,
	Node        *resp_in
) {
	#pragma HLS inline
	node_id_t nid   = bptr_node_id(addr);
	bptr_t    laddr = bptr_local_addr(addr);

	if (nid == local_id) {
		return hbm[laddr];
	}

	ap_uint<64> raddr = (ap_uint<64>)laddr * sizeof(Node);
	rdma_bram_read(
		// Outgoing metadata carries the local connection-table lookup key.
		// That entry supplies the peer's packet-destination QPN.
		(ap_uint<24>)local_qpn,
		/*laddr=*/0,
		raddr,
		sizeof(Node),
		tx_meta
	);

	// Block on the per-op completion token. The status byte itself is unused;
	// the handshake is what synchronizes us with the HBM landing pad write.
	(void)completion.read();

	// Read the freshly-DMAed Node from slot 0.
	return resp_in[0];
}


static bstatusval_t search_one(
	bkey_t key,
	bptr_t root,
	node_id_t local_id,
	Node *hbm,
	int local_qpn,
	hls::stream<pkt256>& tx_meta,
	hls::stream<pkt32>&  completion,
	Node *resp_in
) {
	bptr_t       ptr = root;
	bstatusval_t result;

	while (!is_leaf(ptr)) {
		#pragma HLS loop_tripcount max=MAX_LEVELS
		Node n = fetch_node(ptr, local_id, hbm, local_qpn, tx_meta, completion, resp_in);
		result = find_next(&n, key);
		if (result.status != SUCCESS) {
			return result;
		}
		ptr = result.value.ptr;
	}

	Node leaf = fetch_node(ptr, local_id, hbm, local_qpn, tx_meta, completion, resp_in);
	return find_value(&leaf, key);
}


void sm_search(
	bptr_t const&  root,
	node_id_t      local_id,
	Node          *hbm,
	int            local_qpn,
	hls::stream<search_tagged_in_t>&  input,
	hls::stream<search_tagged_out_t>& output,
	hls::stream<pkt256>&              m_axis_tx_meta,
	hls::stream<pkt32>&               s_axis_completion,
	Node                             *resp_in
) {
	search_loop: for (;;) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		search_tagged_in_t in = input.read();

		search_tagged_out_t out;
		out.last        = in.last;
		out.has_payload = in.has_payload;
		out.val         = search_out_t();

		if (in.has_payload) {
			out.val = search_one(in.key, root, local_id, hbm, local_qpn,
			                     m_axis_tx_meta, s_axis_completion, resp_in);
		}
		output.write(out);

		if (in.last) break;
	}
}
