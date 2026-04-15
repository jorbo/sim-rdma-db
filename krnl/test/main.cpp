#include "tests.hpp"
extern "C" {
#include "../core/operations.h"
};
#include <iostream>


int main() {
	uint passed = 0, failed = 0;
	Node hbm[MEM_SIZE];
	Request req_buffer[0x100];
	Response resp_buffer[0x100];
	bptr_t root = bptr_make(0, 0);
	int loop_max = 0x100;
	int op_max = 0x0c0;
	bool reset = true;

	std::cout << "\n\n=== Search Tests ===" << std::endl;
	std::cout << "--- Root is Leaf ---" << std::endl;
	if (root_is_leaf(&root, hbm, req_buffer, resp_buffer, loop_max, op_max, reset)) {
		std::cout << "\nPassed!\n" << std::endl;
		passed++;
	} else {
		std::cerr << "\nFailed!\n" << std::endl;
		failed++;
	}
	std::cout << "--- One Internal ---" << std::endl;
	if (one_internal(&root, hbm, req_buffer, resp_buffer, loop_max, op_max, reset)) {
		std::cout << "\nPassed!\n" << std::endl;
		passed++;
	} else {
		std::cerr << "\nFailed!\n" << std::endl;
		failed++;
	}

	std::cout << "\n\n=== Insert Tests ===" << std::endl;
	std::cout << "--- Leaf Node ---" << std::endl;
	if (leaf_node(&root, hbm, req_buffer, resp_buffer, loop_max, op_max, reset)) {
		std::cout << "\nPassed!\n" << std::endl;
		passed++;
	} else {
		std::cerr << "\nFailed!\n" << std::endl;
		failed++;
	}
	std::cout << "--- Split Root ---" << std::endl;
	if (split_root(&root, hbm, req_buffer, resp_buffer, loop_max, op_max, reset)) {
		std::cout << "\nPassed!\n" << std::endl;
		passed++;
	} else {
		std::cerr << "\nFailed!\n" << std::endl;
		failed++;
	}
	std::cout << "--- Insert Until It Breaks ---" << std::endl;
	if (until_it_breaks(&root, hbm, req_buffer, resp_buffer, loop_max, op_max, reset)) {
		std::cout << "\nPassed!\n" << std::endl;
		passed++;
	} else {
		std::cerr << "\nFailed!\n" << std::endl;
		failed++;
	}

	std::cout << "\n" << passed << " tests passed, "
		<< failed << " tests failed." << std::endl;

	return 0;
}
