#pragma once

#include <string>
#include <vector>
#include "../krnl/hls/rdma.hpp"
extern "C" {
#include "../krnl/core/types.h"
};

#define BOOTSTRAP_PORT 7890

//! QPNs are host-assigned with this stack (the host writes the QP context
//! into rocetest_krnl); a deterministic scheme keeps every node able to
//! compute every peer's QPN from the config file alone.
#define QPN_BASE 0x100
//! Spec says PSNs should be random; fixed on both sides for first light.
#define PSN_FIRST_LIGHT 0
//! UDP port of the RoCEv2 data plane.
#define RDMA_UDP_PORT 4791

struct NodeConfig {
    node_id_t   id;
    std::string host_ip; //!< Host NIC address, used for the TCP bootstrap
    std::string fpga_ip; //!< FPGA 100G data-plane address (rocetest lIP/rIP);
                         //!< may be empty for emulation/single-node runs
};

struct RdmaConfig {
    node_id_t my_node_id;
    int       qpn_table[MAX_KRNL_NODES];
};

//! Parse a whitespace-separated config file: one
//! "node_id host_ip [fpga_ip]" per line. Lines beginning with '#' are
//! ignored. fpga_ip is optional for emulation/single-node configs.
std::vector<NodeConfig> parse_node_config(const std::string& path);

//! The QPN this node uses. Host-assigned (the host programs the QP context
//! into the RoCE stack), so a deterministic per-node scheme suffices.
static inline int qpn_for(node_id_t id) { return QPN_BASE + id; }

//! Exchange QPNs with all peers via TCP on BOOTSTRAP_PORT and return a
//! fully-populated RdmaConfig.  Uses a lower-id-connects / higher-id-accepts
//! strategy to avoid deadlock without threads.
//!
//! @param my_id     This node's id — must appear in @p nodes.
//! @param nodes     Full list of all nodes (including this one).
//! @param local_qpn This node's QPN (from qpn_for()).
RdmaConfig bootstrap_rdma(
    node_id_t                   my_id,
    const std::vector<NodeConfig>& nodes,
    int                         local_qpn
);
