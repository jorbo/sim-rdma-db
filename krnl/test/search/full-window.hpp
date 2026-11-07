#ifndef TEST__FULL_WINDOW_HPP
#define TEST__FULL_WINDOW_HPP


extern "C" {
#include "../../core/node.h"
};
#include "../test-helpers.hpp"
extern "C" {
#include "../../core/operations.h"
};
#include <hls_stream.h>


//! Exactly NUM_REQUESTS searches in one launch (the window is full, no NOP
//! terminator in req_buffer). Regression for the hardware hang at 256
//! requests: sm_encode used to emit NUM_REQUESTS+1 tokens into the
//! NUM_REQUESTS-deep `responses` FIFO. csim streams are unbounded, so only
//! cosim/hardware exercise the depth; csim checks the last-with-payload path.
bool full_window(KERNEL_ARG_DECS);


#endif
