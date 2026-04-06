#ifndef TEST__SPLIT_ROOT_HPP
#define TEST__SPLIT_ROOT_HPP


extern "C" {
#include "../../core/node.h"
};
#include "../test-helpers.hpp"
extern "C" {
#include "../../core/operations.h"
};
#include <hls_stream.h>


bool split_root(KERNEL_ARG_DECS);


#endif
