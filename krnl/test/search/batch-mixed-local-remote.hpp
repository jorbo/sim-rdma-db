#ifndef TEST__BATCH_MIXED_LOCAL_REMOTE_HPP
#define TEST__BATCH_MIXED_LOCAL_REMOTE_HPP

extern "C" {
#include "../../core/node.h"
#include "../../core/operations.h"
};
#include "../test-helpers.hpp"

//! Two keys under a remote root whose leaves live on different nodes: one
//! local (HBM, no RDMA) and one remote. The leaf level must issue exactly
//! one RDMA read and read the other leaf from local memory, in the same
//! level step.
bool batch_mixed_local_remote(KERNEL_ARG_DECS);

#endif
