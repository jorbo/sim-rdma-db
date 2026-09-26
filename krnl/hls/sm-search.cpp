#include "sm-search.hpp"
#include "../core/node.h"
#include <ap_utils.h>


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
	// Outgoing metadata carries the local connection-table lookup key.
	// That entry supplies the peer's packet-destination QPN.
	pkt256 meta = rdma_bram_read_meta(
		(ap_uint<24>)local_qpn, /*laddr=*/0, raddr, sizeof(Node));

	// Emit the RDMA-read request, THEN block on the per-op completion token.
	//
	// The order matters and HLS will not keep it on its own: the write and
	// the read touch different streams with no data dependence, so the
	// scheduler is free to place the blocking read in the same FSM state as
	// the write (observed in search_one.verbose.sched.rpt, ST_5). A blocked
	// state holds every op in it, including the write, so the kernel waits
	// for a completion to a request it never sent: krnl_1 sits in START,
	// tx_meta never asserts TVALID. Cosim cannot catch it because the TB
	// pre-stages the token. The PROTOCOL region keeps program order and
	// ap_wait() forces a cycle boundary between the two.
	pkt32 tok;
	{
		#pragma HLS protocol fixed
		tx_meta.write(meta);
		ap_wait();
		tok = completion.read();
	}

	// The token must feed the read's address: with a plain `resp_in[0]`
	// there is no data dependence on the stream read, so the scheduler
	// hoists the AXI load above it and every fetch returns the PREVIOUS
	// response (observed on hardware as searches resolving in the parent
	// node). The DataMover command tag is 0 (mem_single_inf.sv), so bit 0
	// of the status byte is always 0 and the index is always slot 0 -- but
	// the scheduler cannot prove that, which is the point.
	return resp_in[tok.data & 0x1];
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
		// HLS auto-pipelines this loop (II=77 at HEAD). A PROTOCOL region
		// is not allowed inside a pipelined loop, and a remote fetch is a
		// round trip anyway, so pipelining buys nothing here.
		#pragma HLS pipeline off
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
	bptr_t         root,
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
