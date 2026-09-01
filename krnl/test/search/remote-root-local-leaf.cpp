#include "remote-root-local-leaf.hpp"
#include "../../hls/krnl.hpp"
#include "../../hls/ramstream.hpp"
#include <iostream>

bool remote_root_local_leaf(KERNEL_ARG_DECS) {
	bool pass = true;
	const node_id_t local_id = 1;
	const node_id_t remote_id = 0;
	const bptr_t remote_root = bptr_make(remote_id, 0x14);
	const bptr_t local_leaf = bptr_make(local_id, 0);
	const bkey_t search_key = 42;

	Node remote_root_node;
	clear(&remote_root_node);
	remote_root_node.keys[0] = search_key;
	remote_root_node.values[0].ptr = local_leaf;

	clear(&hbm[0]);
	hbm[0].keys[0] = search_key;
	hbm[0].values[0].data = -search_key;

	*root = remote_root;
	reset_ramstream_offsets();
	req_buffer[0] = encode_search_req(search_key);
	req_buffer[1].opcode = NOP;

	DECLARE_RDMA_ARGS
	my_node_id = local_id;
	local_qpn = 0x101;
	SIMULATE_REMOTE_FETCH(remote_root_node);

	krnl(KERNEL_ARG_VARS);

	search_out_t result = resp_buffer[0].search;
	if (result.status != SUCCESS || result.value.data != -search_key) {
		std::cerr << "Remote-root/local-leaf search returned the wrong result" << std::endl;
		pass = false;
	}

	if (m_axis_tx_meta.empty()) {
		std::cerr << "Remote-root search emitted no RDMA metadata" << std::endl;
		return false;
	}

	pkt256 meta = m_axis_tx_meta.read();
	const ap_uint<64> expected_raddr =
		(ap_uint<64>)bptr_local_addr(remote_root) * sizeof(Node);

	if (sizeof(Node) != 0x28 || expected_raddr != 0x320) {
		std::cerr << "Unexpected first-light Node layout/address" << std::endl;
		pass = false;
	}
	if (meta.data.range(2, 0) != 0) {
		std::cerr << "Expected RDMA READ opcode 0" << std::endl;
		pass = false;
	}
	if (meta.data.range(26, 3) != 0x101) {
		std::cerr << "Expected local QPN 0x101" << std::endl;
		pass = false;
	}
	if (meta.data.range(74, 27) != 0) {
		std::cerr << "Expected landing-pad address 0" << std::endl;
		pass = false;
	}
	if (meta.data.range(122, 75) != expected_raddr) {
		std::cerr << "Expected remote address 0x320" << std::endl;
		pass = false;
	}
	if (meta.data.range(154, 123) != sizeof(Node)) {
		std::cerr << "Expected length 0x28" << std::endl;
		pass = false;
	}
	if (!m_axis_tx_meta.empty()) {
		std::cerr << "Remote-root search emitted extra RDMA metadata" << std::endl;
		pass = false;
	}

	return pass;
}
