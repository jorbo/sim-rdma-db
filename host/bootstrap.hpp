#pragma once

#include <string>
#include <vector>
#include "../krnl/hls/rdma.hpp"
extern "C" {
#include "../krnl/core/types.h"
};

#define BOOTSTRAP_PORT 7890

struct NodeConfig {
    node_id_t   id;
    std::string ip;
};

struct RdmaConfig {
    node_id_t my_node_id;
    int       qpn_table[MAX_KRNL_NODES];
};

//! Parse a whitespace-separated config file: one "node_id ip_address" per line.
//! Lines beginning with '#' are ignored.
std::vector<NodeConfig> parse_node_config(const std::string& path);

//! Get the local RDMA Queue Pair Number from the FPGA hardware.
//! STUB: returns 0 until Coyote HAL is wired up.
int get_local_qpn();

//! Exchange QPNs with all peers via TCP on BOOTSTRAP_PORT and return a
//! fully-populated RdmaConfig.  Uses a lower-id-connects / higher-id-accepts
//! strategy to avoid deadlock without threads.
//!
//! @param my_id     This node's id — must appear in @p nodes.
//! @param nodes     Full list of all nodes (including this one).
//! @param local_qpn This node's QPN (from get_local_qpn()).
RdmaConfig bootstrap_rdma(
    node_id_t                   my_id,
    const std::vector<NodeConfig>& nodes,
    int                         local_qpn
);
