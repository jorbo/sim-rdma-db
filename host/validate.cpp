#include "validate.hpp"
#include "node.h"
#include <iostream>


bool check_inserted_leaves(Node const *memory) {
	bool match = true;
	uint_fast8_t next_val = 1;
	AddrNode node = {.addr = 0};
	int i = 0;

	while (node.addr != INVALID) {
		node.node = memory[bptr_local_addr(node.addr)];
		for (li_t j = 0; j < TREE_ORDER; ++j) {
			if (node.node.keys[j] == INVALID) {
				if (i == 0 && j == 0) {
					std::cerr << "Fail, memory is empty" << std::endl;
					match = false;
				}
				break;
			} else {
				if (node.node.keys[j] != next_val) {
					std::cerr << "mem[" << node.addr << "].keys[" << (uint) j <<"]:"
						<< "\n\texpected " << (int) node.node.keys[j]
						<< "\n\tgot " << (int) next_val << std::endl;
					match = false;
				}
				if (node.node.values[j].data != -next_val) {
					std::cerr << "mem[" << node.addr << "].values[" << (uint) j <<"]:"
						<< "\n\texpected " << (int) node.node.values[j].data
						<< "\n\tgot " << (int) -next_val << std::endl;
					match = false;
				}
				std::cout << "Verified value " << next_val << std::endl;
				next_val++;
			}
		}
		i++;
		node.addr = node.node.next;
	}
	if (match) {
		std::cout << "Verified "
			<< next_val-1 << " k/v pairs across "
			<< i << " leaves" << std::endl;
	}
	return match;
}
