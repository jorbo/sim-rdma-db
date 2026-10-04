#include "batch-dedup-root.hpp"
#include "../../hls/krnl.hpp"
#include "../../hls/ramstream.hpp"
#include <iostream>

static bool expect_beat(hls::stream<pkt256> &meta, ap_uint<64> laddr, ap_uint<64> raddr, const char *what)
{
	if (meta.empty()) {
		std::cerr << what << ": no RDMA metadata emitted" << std::endl;
		return false;
	}
	pkt256 m = meta.read();
	bool ok = true;
	if (m.data.range(74, 27) != laddr) {
		std::cerr << what << ": lAddr 0x" << std::hex << m.data.range(74, 27)
		          << " expected 0x" << laddr << std::dec << std::endl; ok = false;
	}
	if (m.data.range(122, 75) != raddr) {
		std::cerr << what << ": rAddr 0x" << std::hex << m.data.range(122, 75)
		          << " expected 0x" << raddr << std::dec << std::endl; ok = false;
	}
	return ok;
}

bool batch_dedup_root(KERNEL_ARG_DECS) {
	bool pass = true;
	const node_id_t local_id  = 1;
	const node_id_t remote_id = 0;
	const bptr_t remote_root = bptr_make(remote_id, 0x14);   // internal (>= MAX_LEAVES)
	const bptr_t leaf_a      = bptr_make(remote_id, 0x01);   // keys <= 10
	const bptr_t leaf_b      = bptr_make(remote_id, 0x02);   // 10 < keys <= 20
	const bkey_t keys[3]     = {5, 8, 15};                   // 5,8 -> leaf_a ; 15 -> leaf_b

	Node root_node; clear(&root_node);
	root_node.keys[0] = 10; root_node.values[0].ptr = leaf_a;
	root_node.keys[1] = 20; root_node.values[1].ptr = leaf_b;

	Node leaf_a_node; clear(&leaf_a_node);
	leaf_a_node.keys[0] = 5; leaf_a_node.values[0].data = -5;
	leaf_a_node.keys[1] = 8; leaf_a_node.values[1].data = -8;

	Node leaf_b_node; clear(&leaf_b_node);
	leaf_b_node.keys[0] = 15; leaf_b_node.values[0].data = -15;

	*root = remote_root;
	reset_ramstream_offsets();
	for (int i = 0; i < 3; i++) req_buffer[i] = encode_search_req(keys[i]);
	req_buffer[3].opcode = NOP;

	DECLARE_RDMA_ARGS
	my_node_id = local_id;
	local_qpn  = 0x101;
	// Fetch order the kernel must produce: root (slot 0), then the distinct
	// leaves in first-seen key order: leaf_a (slot 1), leaf_b (slot 2).
	SIMULATE_REMOTE_FETCH(0, root_node);
	SIMULATE_REMOTE_FETCH(1, leaf_a_node);
	SIMULATE_REMOTE_FETCH(2, leaf_b_node);

	krnl(KERNEL_ARG_VARS);

	for (int i = 0; i < 3; i++) {
		search_out_t r = resp_buffer[i].search;
		if (r.status != SUCCESS || r.value.data != -(bdata_t)keys[i]) {
			std::cerr << "key " << keys[i] << ": status=" << (int)r.status
			          << " value=" << r.value.data << " expected " << -(bdata_t)keys[i] << std::endl;
			pass = false;
		}
	}

	const ap_uint<64> N = sizeof(Node);
	pass &= expect_beat(m_axis_tx_meta, 0,     0x14 * N, "root fetch");
	pass &= expect_beat(m_axis_tx_meta, 1 * N, 0x01 * N, "leaf_a fetch");
	pass &= expect_beat(m_axis_tx_meta, 2 * N, 0x02 * N, "leaf_b fetch");
	if (!m_axis_tx_meta.empty()) {
		std::cerr << "extra RDMA metadata: root or a leaf was fetched more than once" << std::endl;
		pass = false;
	}
	if (!s_axis_completion.empty()) {
		std::cerr << "completion token left unconsumed" << std::endl;
		pass = false;
	}
	return pass;
}
