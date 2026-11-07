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
	cl::Kernel roce_krnl;
	bool roce_ready = false;
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

//! Kernel launch cap: requests per run_fpga_tree call. Must match
//! NUM_REQUESTS in krnl/hls/ramstream.hpp (the kernel clamps op_max to it
//! and would silently drop the excess).
#define KRNL_MAX_REQUESTS 256

//! Run the request workload on an already-set-up device.
//! @param quiet    suppress per-launch prints and the LAUNCH_PROBE run
//! @param comp_ms  if non-null, receives the wall-clock run time in ms
TreeOutput run_fpga_tree(TreeDevice& dev, TreeInput& input,
                         const RdmaConfig& rdma, bool quiet = false,
                         double* comp_ms = nullptr);


#endif
