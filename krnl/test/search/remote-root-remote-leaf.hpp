#ifndef TEST__REMOTE_ROOT_REMOTE_LEAF_HPP
#define TEST__REMOTE_ROOT_REMOTE_LEAF_HPP

extern "C" {
#include "../../core/node.h"
#include "../../core/operations.h"
};
#include "../test-helpers.hpp"

//! Two consecutive remote fetches in one search: root lands in slot 0 (tag 0),
//! leaf in slot 1 (tag 5). Exercises the landing-slot rotation and the
//! tag -> slot inverse in fetch_node.
bool remote_root_remote_leaf(KERNEL_ARG_DECS);

#endif
