#ifndef TEST__UNTIL_IT_BREAKS_HPP
#define TEST__UNTIL_IT_BREAKS_HPP


extern "C" {
#include "../../core/node.h"
};
#include "../test-helpers.hpp"
extern "C" {
#include "../../core/operations.h"
};
#include <hls_stream.h>


bool until_it_breaks(KERNEL_ARG_DECS);


#endif
