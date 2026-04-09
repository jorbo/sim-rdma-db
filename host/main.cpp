#include "host.hpp"
#include "test.hpp"
#include "run-tree.hpp"
#include <iostream>


int main(int argc, char** argv) {
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
		return EXIT_FAILURE;
	}
	TreeInput input;
	std::vector<Response, aligned_allocator<Response> > responses_expected;
	setup_data(input.requests, responses_expected, input.memory);
	TreeOutput output = run_fpga_tree(input, argv[1]);

	return verify(output.responses, responses_expected, output.memory);
}
