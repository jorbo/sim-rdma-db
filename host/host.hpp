#ifndef HOST_HPP
#define HOST_HPP


#include "run-tree.hpp"
#include "bootstrap.hpp"
#include "myopencl.hpp"


//! OpenCL state that must exist before the bootstrap runs: peers exchange
//! the device address of the RDMA-exposed tree memory, which is only known
//! once the buffer is allocated.
struct TreeDevice {
	cl::Context context;
	cl::Device device;
	cl::Program program; //!< kept so further kernels (rocetest) can be probed
	cl::Kernel krnl;
	cl::CommandQueue q;
	//! Tree-node memory (HBM-resident, exposed to peers via RDMA reads)
	cl::Buffer buffer_memory;
	//! Device address of buffer_memory, sent to peers during bootstrap
	uint64_t memory_vaddr;
	//! Single-node RDMA response landing slot, read by krnl::resp_in.
	std::vector<Node, aligned_allocator<Node> > rdma_landing;
	cl::Buffer buffer_rdma;
};

//! Program the device and allocate the RDMA-exposed tree memory buffer.
TreeDevice tree_device_setup(std::string const& binaryFile, TreeInput& input);

//! Run the request workload on an already-set-up device.
TreeOutput run_fpga_tree(TreeDevice& dev, TreeInput& input,
                         const RdmaConfig& rdma);


#endif
