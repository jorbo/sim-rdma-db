#pragma once

#include <string>
#include <vector>
#include "../krnl/hls/rdma.hpp"
extern "C" {
#include "../krnl/core/types.h"
};

#define BOOTSTRAP_PORT 7890
//! A failed/crashed peer must not turn the readiness check into another
//! indefinite hardware-looking hang.
#define ROCE_READY_TIMEOUT_SECONDS 120

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
    //! Device address of each node's RDMA-exposed tree memory; RDMA reads
    //! target vaddr_table[nid] + local_addr * sizeof(Node)
    uint64_t  vaddr_table[MAX_KRNL_NODES];
    //! Root pointer of each node's tree (post-split, node-id encoded).
    //! Head nodes traverse root_table[table_node]; 0 = no tree advertised.
    bptr_t    root_table[MAX_KRNL_NODES];
};

//! Parse a whitespace-separated config file: one
//! "node_id host_ip [fpga_ip]" per line. Lines beginning with '#' are
//! ignored. fpga_ip is optional for emulation/single-node configs.
std::vector<NodeConfig> parse_node_config(const std::string& path);

//! The QPN this node uses. Host-assigned (the host programs the QP context
//! into the RoCE stack), so a deterministic per-node scheme suffices.
static inline int qpn_for(node_id_t id) { return QPN_BASE + id; }

//! Exchange QPNs and memory addresses with all peers via TCP on
//! BOOTSTRAP_PORT and return a fully-populated RdmaConfig.  Uses a
//! lower-id-connects / higher-id-accepts strategy to avoid deadlock
//! without threads.
//!
//! @param my_id       This node's id — must appear in @p nodes.
//! @param nodes       Full list of all nodes (including this one).
//! @param local_qpn   This node's QPN (from qpn_for()).
//! @param local_vaddr Device address of this node's RDMA-exposed tree
//!                    memory (TreeDevice::memory_vaddr).
//! @param local_root  Root pointer of this node's finished tree; 0 if this
//!                    node does not serve one (head nodes).
RdmaConfig bootstrap_rdma(
    node_id_t                   my_id,
    const std::vector<NodeConfig>& nodes,
    int                         local_qpn,
    uint64_t                    local_vaddr,
    bptr_t                      local_root
);

//! Reuse the still-open bootstrap connections to wait until every peer has
//! completed configure_roce(). Returns false if this node or any peer reaches
//! the barrier with setup unavailable. Throws if a peer does not reach or
//! finish the barrier before the shared deadline.
bool synchronize_roce_ready(
    node_id_t                      my_id,
    const std::vector<NodeConfig>& nodes,
    bool                           local_ready
);
