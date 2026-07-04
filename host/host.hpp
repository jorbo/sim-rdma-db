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
	cl::Kernel krnl;
	cl::CommandQueue q;
	//! Tree-node memory (HBM-resident, exposed to peers via RDMA reads)
	cl::Buffer buffer_memory;
	//! Device address of buffer_memory, sent to peers during bootstrap
	uint64_t memory_vaddr;
};

//! Program the device and allocate the RDMA-exposed tree memory buffer.
TreeDevice tree_device_setup(std::string const& binaryFile, TreeInput& input);

//! Run the request workload on an already-set-up device.
TreeOutput run_fpga_tree(TreeDevice& dev, TreeInput& input,
                         const RdmaConfig& rdma);


#endif
