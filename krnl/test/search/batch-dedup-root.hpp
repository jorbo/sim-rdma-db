#ifndef TEST__BATCH_DEDUP_ROOT_HPP
#define TEST__BATCH_DEDUP_ROOT_HPP

extern "C" {
#include "../../core/node.h"
#include "../../core/operations.h"
};
#include "../test-helpers.hpp"

//! Three keys in one batch under a remote root: two share a leaf, one does
//! not. Level-wise traversal must fetch the root ONCE and each distinct leaf
//! once (3 RDMA reads for 3 keys x 2 levels), and return every key's value.
bool batch_dedup_root(KERNEL_ARG_DECS);

#endif
