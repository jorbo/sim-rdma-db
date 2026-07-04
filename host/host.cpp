#include "host.hpp"
#include "bootstrap.hpp"
#include "myopencl.hpp"
#include "device.hpp"
extern "C" {
#include "../krnl/core/node.h"
};
#if __has_include(<CL/cl_ext_xilinx.h>)
#include <CL/cl_ext_xilinx.h>
#define HAVE_XILINX_EXT 1
#endif


static void setup_ocl(
	std::string const& binaryFile,
	cl::Context& context,
	cl::Device& device_out,
	cl::Kernel& krnl1,
	cl::CommandQueue& q
) {
	cl_int err;

	auto devices = get_xil_devices();
	auto fileBuf = read_binary_file(binaryFile);
	cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
	bool valid_device = false;

	for (size_t i = 0; i < devices.size(); i++) {
		auto device = devices[i];
		OCL_CHECK(err,
			context = cl::Context(device, nullptr, nullptr, nullptr, &err));
		OCL_CHECK(err, q = cl::CommandQueue(context, device, 0, &err));

		std::cout << "Trying to program device[" << i << "]: "
			<< device.getInfo<CL_DEVICE_NAME>() << std::endl;
		cl::Program program(context, {device}, bins, nullptr, &err);

		if (err != CL_SUCCESS) {
			std::cerr << "Failed to program device[" << i << "]"
				<< " with xclbin file '" << binaryFile << "'!\n";
		} else {
			std::cout << "Device[" << i << "]: program successful!\n";
			std::cout << "Setting CU(s) up..." << std::endl;
			OCL_CHECK(err, krnl1 = cl::Kernel(program, "krnl", &err));
			device_out = device;
			valid_device = true;
			break;
		}
	}
	if (!valid_device) {
		std::cerr << "Failed to program any device found, exit!\n";
		exit(EXIT_FAILURE);
	}
}


//! Device-side address of a buffer (Xilinx extension). Peers RDMA-read the
//! tree memory directly, so its physical address is exchanged at bootstrap.
static uint64_t device_address(cl::Buffer& buf, cl::Device& device) {
#ifdef HAVE_XILINX_EXT
	uint64_t addr = 0;
	cl_int err = xclGetMemObjectDeviceAddress(
		buf(), device(), sizeof(addr), &addr);
	if (err != CL_SUCCESS) {
		printf("WARNING: xclGetMemObjectDeviceAddress failed (%d); "
			"advertising vAddr=0\n", err);
		return 0;
	}
	return addr;
#else
	(void)buf; (void)device;
	printf("WARNING: CL/cl_ext_xilinx.h unavailable; advertising vAddr=0\n");
	return 0;
#endif
}


TreeDevice tree_device_setup(std::string const& binaryFile, TreeInput& input) {
	TreeDevice dev;
	cl_int err;

	setup_ocl(binaryFile, dev.context, dev.device, dev.krnl, dev.q);
	OCL_CHECK(err, dev.buffer_memory = cl::Buffer(
		dev.context,
		CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
		sizeof(Node)*input.memory.size(), input.memory.data(), &err
	));
	dev.memory_vaddr = device_address(dev.buffer_memory, dev.device);
	return dev;
}


static void run_kernel(
	cl::Context& context,
	cl::Kernel& krnl1,
	cl::CommandQueue& q,
	cl::Buffer& buffer_memory,
	bptr_t& root,
	std::vector<Request, aligned_allocator<Request> >& requests,
	std::vector<Response, aligned_allocator<Response> >& responses,
	const RdmaConfig& rdma
) {
	constexpr int FROM_HOST_FLAGS = 0;
	cl_int err;
	clock_t htod, dtoh, comp;

	// Mutable local copy of qpn_table for OpenCL buffer mapping.
	int qpn_table[MAX_KRNL_NODES];
	memcpy(qpn_table, rdma.qpn_table, sizeof(qpn_table));

	// BUFFERS (buffer_memory was created before bootstrap; see
	// tree_device_setup)
	OCL_CHECK(err, cl::Buffer buffer_root(
		context,
		CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
		sizeof(bptr_t), &root, &err
	));
	OCL_CHECK(err, cl::Buffer buffer_requests(
		context,
		CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
		sizeof(Request)*requests.size(), requests.data(), &err
	));
	OCL_CHECK(err, cl::Buffer buffer_responses(
		context,
		CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
		sizeof(Response)*responses.size(), responses.data(), &err
	));
	OCL_CHECK(err, cl::Buffer buffer_qpn_table(
		context,
		CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
		sizeof(qpn_table), qpn_table, &err
	));
	// resp_in: HBM-resident RDMA-read landing pad (single slot). On hardware
	// it shares HBM[0] with rocetest m00_axi; the host never touches the
	// contents, but XRT requires every pointer argument to be bound.
	std::vector<Node, aligned_allocator<Node> > resp_in(1);
	OCL_CHECK(err, cl::Buffer buffer_resp_in(
		context,
		CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
		sizeof(Node)*resp_in.size(), resp_in.data(), &err
	));

	// KERNEL ARGS (must match krnl() parameter order)
	int loop_max = 6 * (int)requests.size();
	int op_max   = (int)(1.25 * requests.size());
	OCL_CHECK(err, err = krnl1.setArg(0, buffer_root));
	OCL_CHECK(err, err = krnl1.setArg(1, buffer_memory));
	OCL_CHECK(err, err = krnl1.setArg(2, buffer_requests));
	OCL_CHECK(err, err = krnl1.setArg(3, buffer_responses));
	OCL_CHECK(err, err = krnl1.setArg(4, loop_max));
	OCL_CHECK(err, err = krnl1.setArg(5, op_max));
	OCL_CHECK(err, err = krnl1.setArg(6, true));
	OCL_CHECK(err, err = krnl1.setArg(7, (uint8_t)rdma.my_node_id));
	OCL_CHECK(err, err = krnl1.setArg(8, buffer_qpn_table));
	// Args 9/10 are the AXIS streams (m_axis_tx_meta, s_axis_completion):
	// stream-connected in the xclbin, never set from the host.
	OCL_CHECK(err, err = krnl1.setArg(11, buffer_resp_in));

	// HOST -> DEVICE
	std::cout << "HOST -> DEVICE" << std::endl;
	htod = clock();
	OCL_CHECK(err, err = q.enqueueMigrateMemObjects(
		{buffer_root, buffer_memory, buffer_requests, buffer_qpn_table},
		FROM_HOST_FLAGS
	));
	q.finish();
	htod = clock() - htod;

	// RUN
	std::cout << "STARTING KERNEL(S)" << std::endl;
	comp = clock();
	OCL_CHECK(err, err = q.enqueueTask(krnl1));
	q.finish();
	comp = clock() - comp;
	std::cout << "KERNEL(S) FINISHED" << std::endl;

	// DEVICE -> HOST (root can be updated by the kernel on tree splits)
	std::cout << "HOST <- DEVICE" << std::endl;
	dtoh = clock();
	OCL_CHECK(err, err = q.enqueueMigrateMemObjects(
		{buffer_root, buffer_memory, buffer_responses},
		CL_MIGRATE_MEM_OBJECT_HOST
	));
	q.finish();
	dtoh = clock() - dtoh;

	printf("Host -> Device : %lf ms\n", 1000.0 * htod/CLOCKS_PER_SEC);
	printf("Device -> Host : %lf ms\n", 1000.0 * dtoh/CLOCKS_PER_SEC);
	printf("Computation    : %lf ms\n", 1000.0 * comp/CLOCKS_PER_SEC);
}


TreeOutput run_fpga_tree(TreeDevice& dev, TreeInput& input,
                         const RdmaConfig& rdma) {
	TreeOutput output;
	output.responses.resize(input.requests.size(), {.opcode=NOP});

	run_kernel(
		dev.context, dev.krnl, dev.q, dev.buffer_memory,
		input.root, input.requests, output.responses, rdma
	);
	memcpy(output.memory.data(), input.memory.data(), MEM_SIZE*sizeof(Node));
	output.root = input.root; // root may have been updated by splits

	return output;
}
