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

	// Bootstrap: exchange QPNs with all peers.
	auto nodes      = parse_node_config(config_path);
	int  local_qpn  = get_local_qpn();
	RdmaConfig rdma = bootstrap_rdma(my_id, nodes, local_qpn);

	std::cout << "Bootstrap complete. Node " << (int)my_id
	          << " QPN table:";
	for (int i = 0; i < (int)nodes.size(); ++i)
		std::cout << " [" << i << "]=" << rdma.qpn_table[i];
	std::cout << std::endl;

	TreeInput input;
	std::vector<Response, aligned_allocator<Response> > responses_expected;
	setup_data(input.requests, responses_expected, input.memory);
	TreeOutput output = run_fpga_tree(input, rdma, xclbin);

	return verify(output.responses, responses_expected, output.memory);
}
