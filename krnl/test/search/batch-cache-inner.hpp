#ifndef TEST__BATCH_CACHE_INNER_HPP
#define TEST__BATCH_CACHE_INNER_HPP

extern "C" {
#include "../../core/node.h"
#include "../../core/operations.h"
};
#include "../test-helpers.hpp"

//! Three keys over a three-level remote tree (root -> inner -> two leaves)
//! with SEARCH_NODE_CACHE on. The first key fetches root and inner; the
//! other two must take them from the cache and fetch only their leaves:
//! 5 RDMA reads instead of 9, every key's value returned. Skipped (passes)
//! when the cache is compiled out.
bool batch_cache_inner(KERNEL_ARG_DECS);

#endif
