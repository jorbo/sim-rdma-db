#ifndef ROCE_SETUP_HPP
#define ROCE_SETUP_HPP


#include "host.hpp"
#include "bootstrap.hpp"
#include <vector>


//! Program rocetest_krnl's QP context and start the stack.
//!
//! No-op (returns false) when the xclbin has no rocetest_krnl instance
//! (emulation stub builds) or when the config carries no fpga_ip
//! addresses. Two-node first light: the peer is the single other entry
//! in @p nodes.
//!
//! @param mem  Buffer served by the stack's DataMover (m00_axi).
bool configure_roce(
	TreeDevice& dev,
	const RdmaConfig& rdma,
	const std::vector<NodeConfig>& nodes,
	node_id_t my_id,
	cl::Buffer& mem
);

//! Fire one host-driven RDMA READ through rocetest_krnl's manual-op
//! interface: no B-tree kernel involved. Bring-up step: node 1 reads a
//! known pattern from node 0's memory and the host checks the bytes.
//! Requires a prior successful configure_roce().
bool roce_manual_read(
	TreeDevice& dev,
	uint64_t raddr,
	uint64_t laddr,
	uint32_t len
);


#endif
