#ifndef HOST_TEST_HPP
#define HOST_TEST_HPP



extern "C" {
#include "node.h"
#include "operations.h"
};
#include "alloc.hpp"
#include <vector>


//! Number of key/value pairs in the test workload
#define TEST_N_KEYS 22

//! Insert workload: keys 1..TEST_N_KEYS with value -key (table nodes)
void setup_data(
	std::vector<Request, aligned_allocator<Request> >& requests,
	std::vector<Response, aligned_allocator<Response> >& responses_expected,
	std::vector<Node, aligned_allocator<Node> >& memory
);

//! Search workload: looks up keys 1..TEST_N_KEYS expecting value -key
//! (head nodes searching a table node's tree)
void setup_search_data(
	std::vector<Request, aligned_allocator<Request> >& requests,
	std::vector<Response, aligned_allocator<Response> >& responses_expected
);

//! Benchmark search workload: `n` point lookups with keys drawn uniformly
//! from 1..TEST_N_KEYS (every lookup hits), expecting value -key. Same
//! request/response shape as setup_search_data; `seed` makes runs
//! repeatable. Models the paper's closed-loop clients spreading lookups
//! uniformly over the key space.
void setup_bench_search_data(
	size_t n,
	uint32_t seed,
	std::vector<Request, aligned_allocator<Request> >& requests,
	std::vector<Response, aligned_allocator<Response> >& responses_expected
);

//! @param check_memory  false on head nodes: the tree lives on the table
//!                      node, local memory holds nothing to validate
//! @param quiet         true: print mismatches only, no "TEST PASSED/FAILED"
//!                      verdict (benchmark batches report one verdict at
//!                      the end)
int verify(
	std::vector<Response, aligned_allocator<Response> >& responses,
	std::vector<Response, aligned_allocator<Response> >& responses_expected,
	std::vector<Node, aligned_allocator<Node> >& memory,
	bool check_memory = true,
	bool quiet = false
);


#endif
