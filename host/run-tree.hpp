#ifndef RUN_TREE_HPP
#define RUN_TREE_HPP


#include "alloc.hpp"
extern "C" {
#include "../krnl/core/operations.h"
};
#include <vector>


struct TreeInput {
	bptr_t root;
	std::vector<Request, aligned_allocator<Request> > requests;
	std::vector<Node, aligned_allocator<Node> > memory;
	TreeInput();
};

struct TreeOutput {
	bptr_t root;
	std::vector<Response, aligned_allocator<Response> > responses;
	std::vector<Node, aligned_allocator<Node> > memory;
	TreeOutput();
};


#endif
