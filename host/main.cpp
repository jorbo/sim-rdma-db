#include "host.hpp"
#include "bootstrap.hpp"
#include "roce-setup.hpp"
#include "test.hpp"
#include "run-tree.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>


//! Node id that owns the tree and serves peers' RDMA reads.
#define TABLE_NODE_ID 0


static void print_rdma(const RdmaConfig& rdma, size_t n_nodes) {
	std::cout << "Bootstrap complete. Node " << (int)rdma.my_node_id << ":";
	for (size_t i = 0; i < n_nodes; ++i)
		std::cout << " [" << i << "] qpn=0x" << std::hex
		          << rdma.qpn_table[i] << " vaddr=0x"
		          << rdma.vaddr_table[i] << " root=0x"
		          << rdma.root_table[i] << std::dec;
	std::cout << std::endl;
}


int main(int argc, char** argv) {
	if (argc != 4) {
		std::cout << "Usage: " << argv[0]
		          << " <xclbin> <my_node_id> <nodes_config>" << std::endl;
		std::cout << "  nodes_config: text file, one"
		             " 'node_id host_ip [fpga_ip]' per line" << std::endl;
		return EXIT_FAILURE;
	}

	const std::string xclbin      = argv[1];
	node_id_t         my_id       = (node_id_t)std::atoi(argv[2]);
	const std::string config_path = argv[3];
	auto nodes = parse_node_config(config_path);

	TreeInput input;
	std::vector<Response, aligned_allocator<Response> > responses_expected;

	if (my_id == TABLE_NODE_ID) {
		// Table node: build the tree with local inserts and verify it.
		// With peers configured, then advertise the final root and serve
		// their RDMA reads; single-node configs stop after verification
		// (the original regression test).
		setup_data(input.requests, responses_expected, input.memory);
		TreeDevice dev = tree_device_setup(xclbin, input);

		// The local build touches no peer state; self-only config suffices.
		RdmaConfig local;
		memset(&local, 0, sizeof(local));
		local.my_node_id = my_id;

		TreeOutput output = run_fpga_tree(dev, input, local);
		int rc = verify(output.responses, responses_expected, output.memory);
		if (nodes.size() <= 1 || rc != EXIT_SUCCESS)
			return rc;

		RdmaConfig rdma = bootstrap_rdma(my_id, nodes, qpn_for(my_id),
		                                 dev.memory_vaddr, output.root);
		print_rdma(rdma, nodes.size());
		configure_roce(dev, rdma, nodes, my_id, dev.buffer_memory);
		std::cout << "Table node serving; Ctrl-C to exit." << std::endl;
		for (;;) pause();
	} else {
		// Head node: no local tree; search the table node's tree remotely.
		setup_search_data(input.requests, responses_expected);
		TreeDevice dev = tree_device_setup(xclbin, input);

		RdmaConfig rdma = bootstrap_rdma(my_id, nodes, qpn_for(my_id),
		                                 dev.memory_vaddr, /*local_root=*/0);
		print_rdma(rdma, nodes.size());
		bool roce = configure_roce(dev, rdma, nodes, my_id,
		                           dev.buffer_memory);

		// Bring-up step 3: host-driven RDMA READ of the table node's
		// first node into local offset 0, no B-tree kernel involved.
		// Run with RDMA_SELFTEST=1 and inspect the landing bytes.
		if (roce && getenv("RDMA_SELFTEST") != nullptr) {
			std::cout << "RDMA selftest: reading " << sizeof(Node)
			          << " bytes from table node..." << std::endl;
			roce_manual_read(dev, /*raddr=*/0, /*laddr=*/0, sizeof(Node));
			// Landing pad = start of our exposed memory. Expect the table
			// node's leaf 0: keys 1,2 then INVALID (0xffffffff).
			cl_int err;
			OCL_CHECK(err, err = dev.q.enqueueMigrateMemObjects(
				{dev.buffer_memory}, CL_MIGRATE_MEM_OBJECT_HOST));
			dev.q.finish();
			const uint32_t* w = (const uint32_t*)input.memory.data();
			std::cout << "RDMA selftest landing bytes:" << std::hex;
			for (int i = 0; i < 16; ++i)
				std::cout << " " << w[i];
			std::cout << std::dec << std::endl;
			std::cout << "  (expect: 1 2 ffffffff ffffffff ... for leaf 0)"
			          << std::endl;
		}

		// The 22-key workload always splits the root off leaf 0, so a
		// zero root means the table node advertised nothing.
		input.root = rdma.root_table[TABLE_NODE_ID];
		if (input.root == 0) {
			std::cerr << "Table node advertised no tree root" << std::endl;
			return EXIT_FAILURE;
		}

		TreeOutput output = run_fpga_tree(dev, input, rdma);
		return verify(output.responses, responses_expected, output.memory,
		              /*check_memory=*/false);
	}
}
