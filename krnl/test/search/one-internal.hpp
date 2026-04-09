#ifndef TEST__ONE_INTERNAL_HPP
#define TEST__ONE_INTERNAL_HPP


extern "C" {
#include "../../core/node.h"
};
#include "../test-helpers.hpp"
extern "C" {
#include "../../core/operations.h"
};
#include <hls_stream.h>


bool one_internal(KERNEL_ARG_DECS);


#endif
