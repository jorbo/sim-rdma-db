#ifndef TEST__BATCH_ONE_CLIENT_HPP
#define TEST__BATCH_ONE_CLIENT_HPP

extern "C" {
#include "../../core/node.h"
#include "../../core/operations.h"
};
#include "../test-helpers.hpp"

//! The batch-independent tree and keys with clients = 1: one search in
//! flight at a time, so each key's root and leaf reads complete before the
//! next key starts (root, leaf, root, leaf, root, leaf).
bool batch_one_client(KERNEL_ARG_DECS);

#endif
