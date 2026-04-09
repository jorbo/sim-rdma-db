#include "test-helpers.hpp"
#include <cstring>
#include <iostream>


void hbm_dump(uint8_t* hbm, uint_fast64_t offset, size_t size, uint_fast64_t length) {
	const uint8_t objs_per_line = 80 / (size*2);
	std::cout << " -  HBM Dump  - " << std::hex;
	for (uint_fast32_t n = 0; n < length; ++n) {
		if (n % objs_per_line == 0) {
			std::cout << std::endl;
		}
		for (uint_fast32_t b = 0; b < size; ++b) {
			if (hbm[offset + n*size + b] < 0x10) {
				std::cout << '0';
			}
			std::cout << (int) hbm[offset + n*size + b];
		}
		std::cout << "  " << std::flush;
	}
	std::cout << std::dec << std::endl;
}
