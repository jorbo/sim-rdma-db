#ifndef TEST__LEAF_NODE_HPP
#define TEST__LEAF_NODE_HPP


extern "C" {
#include "../../core/node.h"
};
#include "../test-helpers.hpp"
extern "C" {
#include "../../core/operations.h"
};
#include <hls_stream.h>


bool leaf_node(KERNEL_ARG_DECS);


#endif
