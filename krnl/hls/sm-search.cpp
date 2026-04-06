#include "sm-search.hpp"
extern "C" {
#include "../core/search.h"
};


void sm_search(
	bptr_t const& root,
	hls::stream<search_in_t>& input,
	hls::stream<search_out_t>& output,
	Node const **memory
) {
	bkey_t key;
	if (!input.empty()) {
		input.read(key);
		output.write(search(root, key, memory));
	}
}
