#ifndef TEST__BATCH_INDEPENDENT_HPP
#define TEST__BATCH_INDEPENDENT_HPP

extern "C" {
#include "../../core/node.h"
#include "../../core/operations.h"
};
#include "../test-helpers.hpp"

//! Three concurrent keys under a remote root: two share a leaf, one does
//! not. Each lane is an independent client (the paper's fine-grained
//! design), so nothing is shared: every key fetches the root and its leaf
//! itself (6 RDMA reads), and every key's value is returned.
bool batch_independent(KERNEL_ARG_DECS);

#endif
