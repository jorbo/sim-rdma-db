#include "remote-root-remote-leaf.hpp"
#include "../../hls/krnl.hpp"
#include "../../hls/ramstream.hpp"
#include <iostream>

static bool check_meta(pkt256 meta, ap_uint<64> laddr, ap_uint<64> raddr, const char *what)
{
	bool ok = true;
	if (meta.data.range(2, 0) != 0) {
		std::cerr << what << ": expected RDMA READ opcode 0" << std::endl; ok = false;
	}
	if (meta.data.range(26, 3) != 0x101) {
		std::cerr << what << ": expected local QPN 0x101" << std::endl; ok = false;
	}
	if (meta.data.range(74, 27) != laddr) {
		std::cerr << what << ": expected landing-pad address 0x" << std::hex << laddr
		          << ", got 0x" << meta.data.range(74, 27) << std::dec << std::endl; ok = false;
	}
	if (meta.data.range(122, 75) != raddr) {
		std::cerr << what << ": expected remote address 0x" << std::hex << raddr
		          << ", got 0x" << meta.data.range(122, 75) << std::dec << std::endl; ok = false;
	}
	if (meta.data.range(154, 123) != sizeof(Node)) {
		std::cerr << what << ": expected length 0x28" << std::endl; ok = false;
	}
	return ok;
}

bool remote_root_remote_leaf(KERNEL_ARG_DECS) {
	bool pass = true;
	const node_id_t local_id  = 1;
	const node_id_t remote_id = 0;
	// Root is internal (local addr >= MAX_LEAVES), leaf is a leaf slot.
	const bptr_t remote_root = bptr_make(remote_id, 0x14);
	const bptr_t remote_leaf = bptr_make(remote_id, 0x03);
	const bkey_t search_key  = 42;

	Node remote_root_node;
	clear(&remote_root_node);
	remote_root_node.keys[0]       = search_key;
	remote_root_node.values[0].ptr = remote_leaf;

	Node remote_leaf_node;
	clear(&remote_leaf_node);
	remote_leaf_node.keys[0]        = search_key;
	remote_leaf_node.values[0].data = -search_key;

	// Nothing of this search lives in local HBM; make sure the kernel does
	// not read the right answer from there by accident.
	clear(&hbm[0]);
	clear(&hbm[3]);

	*root = remote_root;
	reset_ramstream_offsets();
	req_buffer[0] = encode_search_req(search_key);
	req_buffer[1].opcode = NOP;

	DECLARE_RDMA_ARGS
	my_node_id = local_id;
	local_qpn  = 0x101;
	// Pre-stage both responses in the order the kernel will fetch them:
	// root -> slot 0 (token tag 0), leaf -> slot 1 (token tag 5).
	SIMULATE_REMOTE_FETCH(0, remote_root_node);
	SIMULATE_REMOTE_FETCH(1, remote_leaf_node);

	krnl(KERNEL_ARG_VARS);

	search_out_t result = resp_buffer[0].search;
	if (result.status != SUCCESS || result.value.data != -search_key) {
		std::cerr << "Remote-root/remote-leaf search returned the wrong result" << std::endl;
		pass = false;
	}

	if (m_axis_tx_meta.empty()) {
		std::cerr << "Remote-root search emitted no RDMA metadata" << std::endl;
		return false;
	}
	pkt256 meta0 = m_axis_tx_meta.read();
	pass &= check_meta(meta0, /*laddr=*/0,
	                   (ap_uint<64>)bptr_local_addr(remote_root) * sizeof(Node), "root fetch");

	if (m_axis_tx_meta.empty()) {
		std::cerr << "Leaf fetch emitted no RDMA metadata" << std::endl;
		return false;
	}
	pkt256 meta1 = m_axis_tx_meta.read();
	pass &= check_meta(meta1, /*laddr=*/sizeof(Node),
	                   (ap_uint<64>)bptr_local_addr(remote_leaf) * sizeof(Node), "leaf fetch");

	if (!m_axis_tx_meta.empty()) {
		std::cerr << "Search emitted extra RDMA metadata" << std::endl;
		pass = false;
	}
	if (!s_axis_completion.empty()) {
		std::cerr << "Search left a completion token unconsumed" << std::endl;
		pass = false;
	}

	return pass;
}
