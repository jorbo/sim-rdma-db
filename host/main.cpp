#include "host.hpp"
#include "bootstrap.hpp"
#include "test.hpp"
#include "run-tree.hpp"
#include <iostream>
#include <cstdlib>


int main(int argc, char** argv) {
	if (argc != 4) {
		std::cout << "Usage: " << argv[0]
		          << " <xclbin> <my_node_id> <nodes_config>" << std::endl;
		std::cout << "  nodes_config: text file, one 'node_id ip' per line"
		          << std::endl;
		return EXIT_FAILURE;
	}

	const std::string xclbin      = argv[1];
	node_id_t         my_id       = (node_id_t)std::atoi(argv[2]);
	const std::string config_path = argv[3];

	TreeInput input;
	std::vector<Response, aligned_allocator<Response> > responses_expected;
	setup_data(input.requests, responses_expected, input.memory);

	// Device setup precedes the bootstrap: peers exchange the device
	// address of the RDMA-exposed tree memory, which only exists once the
	// buffer is allocated.
	TreeDevice dev = tree_device_setup(xclbin, input);

	auto nodes      = parse_node_config(config_path);
	RdmaConfig rdma = bootstrap_rdma(my_id, nodes, qpn_for(my_id),
	                                 dev.memory_vaddr);

	std::cout << "Bootstrap complete. Node " << (int)my_id << ":";
	for (int i = 0; i < (int)nodes.size(); ++i)
		std::cout << " [" << i << "] qpn=0x" << std::hex
		          << rdma.qpn_table[i] << " vaddr=0x"
		          << rdma.vaddr_table[i] << std::dec;
	std::cout << std::endl;

	TreeOutput output = run_fpga_tree(dev, input, rdma);

	return verify(output.responses, responses_expected, output.memory);
}
