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

//! @param check_memory  false on head nodes: the tree lives on the table
//!                      node, local memory holds nothing to validate
int verify(
	std::vector<Response, aligned_allocator<Response> >& responses,
	std::vector<Response, aligned_allocator<Response> >& responses_expected,
	std::vector<Node, aligned_allocator<Node> >& memory,
	bool check_memory = true
);


#endif
