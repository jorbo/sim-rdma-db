#include "sm-insert.hpp"
extern "C" {
#include "../core/insert.h"
};
#include <iostream>


void sm_insert(
	bptr_t& root,
	hls::stream<insert_in_t>& input,
	hls::stream<insert_out_t>& output,
	Node **memory
) {
	KvPair pair;

	if (!input.empty()) {
		input.read(pair);
		output.write(insert(&root, pair.key, pair.value, memory));
	}
}
