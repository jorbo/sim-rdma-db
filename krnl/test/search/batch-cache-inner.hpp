#ifndef TEST__BATCH_CACHE_INNER_HPP
#define TEST__BATCH_CACHE_INNER_HPP

extern "C" {
#include "../../core/node.h"
#include "../../core/operations.h"
};
#include "../test-helpers.hpp"

//! Two batches over a three-level remote tree (root -> inner -> two leaves).
//! The first batch fetches root, inner and both leaves. The second batch
//! must reuse the cached root and inner node and fetch only its leaves
//! again: 6 RDMA reads in total instead of 8, every key's value returned.
bool batch_cache_inner(KERNEL_ARG_DECS);

#endif
