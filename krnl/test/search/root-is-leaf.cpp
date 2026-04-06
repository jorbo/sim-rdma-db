#include "root-is-leaf.hpp"
#include "../../hls/krnl.hpp"
#include "../../hls/ramstream.hpp"
#include "../test-helpers.hpp"
extern "C" {
#include "../../core/memory.h"
};
#include <iostream>
#include <cstdint>


bool root_is_leaf(KERNEL_ARG_DECS) {
	bool pass = true;
	const bptr_t root_addr = bptr_make(0, 0);
	hls::stream<search_in_t> input_log;
	uint_fast8_t ops_in, ops_out;
	search_in_t last_in;
	search_out_t last_out;
	Response last_resp;
	uint_fast64_t offset = 0;

	// Set up initial state
	DECLARE_MEMORY_VIEW(memory, hbm)
	mem_reset_all(memory);
	reset_ramstream_offsets();
	*root = root_addr;
	SET_IKV(root_addr, 0, 1, 10)
	SET_IKV(root_addr, 1, 2, 20)
	SET_IKV(root_addr, 2, 4, 40)
	SET_IKV(root_addr, 3, 5, 50)
	hbm_dump((uint8_t*) hbm, 0, sizeof(Node), 2);
	// Should fail
	INPUT_SEARCH(0)
	INPUT_SEARCH(3)
	INPUT_SEARCH(6)
	// Should succeed
	INPUT_SEARCH(1)
	INPUT_SEARCH(2)
	INPUT_SEARCH(4)
	INPUT_SEARCH(5)
	hbm_dump((uint8_t*) req_buffer, 0, sizeof(Request), 15);
	hbm_dump((uint8_t*) resp_buffer, 0, sizeof(Response), 15);

	// Perform Operations
	krnl(KERNEL_ARG_VARS);

	// Evalue Results
	offset = 0;
	while (!input_log.empty()) {
		input_log.read(last_in);
		last_resp = resp_buffer[offset++];
		last_out = last_resp.search;
		#ifdef VERBOSE
		std::cout << "Search(" << last_in << "): ";
		if (last_out.status != SUCCESS) {
			std::cout << "Error: "
				<< ERROR_CODE_NAMES[last_out.status]
				<< '(' << (int) last_out.status << ')' << std::endl;
		} else {
			std::cout << last_out.value.data<< "\t0x" << std::hex
				<< last_out.value.data << std::dec << std::endl;
		}
		#endif
		if (last_in % 3 == 0) {
			if (last_out.status != NOT_FOUND) {
				std::cerr << "For search input " << last_in
					<< ": Expected NOT_FOUND, got "
					<< ERROR_CODE_NAMES[last_out.status]
					<< '(' << (int) last_out.status << ')' << std::endl;
				pass = false;
			}
		} else {
			if (last_out.status != SUCCESS) {
				std::cerr << "For search input " << last_in
					<< ": Expected SUCCESS, got "
					<< ERROR_CODE_NAMES[last_out.status]
					<< '(' << (int) last_out.status << ')' << std::endl;
				pass = false;
			}
			if (last_out.value.data != 10*last_in) {
				std::cerr << "For search input " << last_in
					<< ": Expected " << 10*last_in << ", got "
					<< last_out.value.data << std::endl;
				pass = false;
			}
		}
	}
	// Check for non-empty streams
	if (!input_log.empty()) {
		std::cerr << "Error: Response stream empty before input log stream" << std::endl;
		std::cerr << "Contains the data:";
		do {
			input_log.read(last_in);
			std::cerr << "\n\t" << last_in;
		} while (!input_log.empty());
		std::cerr << std::endl;
		pass = false;
	}

	return pass;
}
