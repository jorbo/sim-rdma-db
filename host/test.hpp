#ifndef HOST_TEST_HPP
#define HOST_TEST_HPP



extern "C" {
#include "node.h"
#include "operations.h"
};
#include "alloc.hpp"
#include <vector>


void setup_data(
	std::vector<Request, aligned_allocator<Request> >& requests,
	std::vector<Response, aligned_allocator<Response> >& responses_expected,
	std::vector<Node, aligned_allocator<Node> >& memory
);

int verify(
	std::vector<Response, aligned_allocator<Response> >& responses,
	std::vector<Response, aligned_allocator<Response> >& responses_expected,
	std::vector<Node, aligned_allocator<Node> >& memory
);


#endif
