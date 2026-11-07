#include "full-window.hpp"
#include "../../hls/krnl.hpp"
#include "../../hls/ramstream.hpp"
#include "../test-helpers.hpp"
extern "C" {
#include "../../core/memory.h"
};
#include <iostream>
#include <cstdint>


bool full_window(KERNEL_ARG_DECS) {
	bool pass = true;
	const bptr_t root_addr = bptr_make(0, 0);
	uint_fast64_t offset = 0;

	{ mem_context_t _ctx = mem_context_local(0, hbm); mem_reset_all(&_ctx); }
	reset_ramstream_offsets();
	*root = root_addr;
	SET_IKV(root_addr, 0, 1, 10)
	SET_IKV(root_addr, 1, 2, 20)
	SET_IKV(root_addr, 2, 4, 40)
	SET_IKV(root_addr, 3, 5, 50)

	// Keys 0..7 repeating: 1,2,4,5 hit, the rest miss.
	for (offset = 0; offset < NUM_REQUESTS; offset++)
		req_buffer[offset] = encode_search_req((bkey_t)(offset % 8));
	// Poison the response window so a missing final write is visible.
	for (offset = 0; offset < NUM_REQUESTS; offset++)
		resp_buffer[offset].search.status = RESTART;

	DECLARE_RDMA_ARGS
	op_max = NUM_REQUESTS;
	krnl(KERNEL_ARG_VARS);

	uint_fast64_t bad = 0;
	for (offset = 0; offset < NUM_REQUESTS; offset++) {
		const bkey_t k = (bkey_t)(offset % 8);
		const search_out_t out = resp_buffer[offset].search;
		const bool hit = (k == 1 || k == 2 || k == 4 || k == 5);
		bool ok = hit ? (out.status == SUCCESS && out.value.data == (bdata_t)(10 * k))
		              : (out.status == NOT_FOUND);
		if (!ok) {
			if (bad < 8)
				std::cerr << "\tResponse " << offset << " for key " << k
					<< ": status " << ERROR_CODE_NAMES[out.status]
					<< " value " << out.value.data << std::endl;
			bad++;
		}
	}
	if (bad != 0) {
		std::cerr << "\t" << bad << " of " << NUM_REQUESTS
			<< " responses wrong" << std::endl;
		pass = false;
	} else {
		std::cout << "All " << NUM_REQUESTS << " responses correct" << std::endl;
	}
	return pass;
}
