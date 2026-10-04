#include "batch-mixed-local-remote.hpp"
#include "../../hls/krnl.hpp"
#include "../../hls/ramstream.hpp"
#include <iostream>

bool batch_mixed_local_remote(KERNEL_ARG_DECS) {
	bool pass = true;
	const node_id_t local_id  = 1;
	const node_id_t remote_id = 0;
	const bptr_t remote_root = bptr_make(remote_id, 0x14);
	const bptr_t local_leaf  = bptr_make(local_id,  0x00);   // hbm[0]
	const bptr_t remote_leaf = bptr_make(remote_id, 0x02);
	const bkey_t key_local = 5, key_remote = 15;

	Node root_node; clear(&root_node);
	root_node.keys[0] = 10; root_node.values[0].ptr = local_leaf;
	root_node.keys[1] = 20; root_node.values[1].ptr = remote_leaf;

	clear(&hbm[0]);
	hbm[0].keys[0] = key_local; hbm[0].values[0].data = -key_local;

	Node remote_leaf_node; clear(&remote_leaf_node);
	remote_leaf_node.keys[0] = key_remote; remote_leaf_node.values[0].data = -key_remote;

	*root = remote_root;
	reset_ramstream_offsets();
	req_buffer[0] = encode_search_req(key_local);
	req_buffer[1] = encode_search_req(key_remote);
	req_buffer[2].opcode = NOP;

	DECLARE_RDMA_ARGS
	my_node_id = local_id;
	local_qpn  = 0x101;
	SIMULATE_REMOTE_FETCH(0, root_node);          // level 0
	SIMULATE_REMOTE_FETCH(1, remote_leaf_node);   // level 1: only the remote leaf uses a slot

	krnl(KERNEL_ARG_VARS);

	search_out_t r0 = resp_buffer[0].search, r1 = resp_buffer[1].search;
	if (r0.status != SUCCESS || r0.value.data != -key_local) {
		std::cerr << "local-leaf key: status=" << (int)r0.status << " value=" << r0.value.data << std::endl;
		pass = false;
	}
	if (r1.status != SUCCESS || r1.value.data != -key_remote) {
		std::cerr << "remote-leaf key: status=" << (int)r1.status << " value=" << r1.value.data << std::endl;
		pass = false;
	}

	int beats = 0;
	while (!m_axis_tx_meta.empty()) { m_axis_tx_meta.read(); beats++; }
	if (beats != 2) {
		std::cerr << "expected 2 RDMA reads (root + remote leaf), got " << beats << std::endl;
		pass = false;
	}
	if (!s_axis_completion.empty()) {
		std::cerr << "completion token left unconsumed" << std::endl;
		pass = false;
	}
	return pass;
}
