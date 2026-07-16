#include "roce-setup.hpp"
#include <arpa/inet.h>
#include <iostream>

// Argument ids from rocetest_krnl.xml. Streams (15-18) are
// stream-connected in the xclbin and never set from the host.
enum RoceArg
{
	ARG_RPSN = 0,
	ARG_LPSN,
	ARG_RQPN,
	ARG_LQPN,
	ARG_RIP,
	ARG_LIP,
	ARG_RUDP,
	ARG_VADDR,
	ARG_RKEY,
	ARG_OP,
	ARG_RADDR,
	ARG_LADDR,
	ARG_LEN,
	ARG_DEBUG,
	ARG_MEM_PTR
};

// Manual-op codes follow the tx_meta opcode encoding (rdma.cpp): 0 READ.
#define ROCE_OP_NONE 0xFFFFFFFFu
#define ROCE_OP_READ 0u

// rKey validation is not enforced by the stack; any constant works as
// long as both sides agree.
#define RKEY_FIRST_LIGHT 0

// Kept between configure_roce and roce_manual_read.
static cl::Kernel roce_krnl;
static bool roce_ready = false;

// TODO(bring-up): confirm byte order against the stack's lIP/rIP
// interpretation; first light assumes host byte order.
static uint32_t ip_to_u32(const std::string &ip)
{
	in_addr a{};
	if (::inet_pton(AF_INET, ip.c_str(), &a) != 1)
		return 0;
	return ntohl(a.s_addr);
}

bool configure_roce(
	TreeDevice &dev,
	const RdmaConfig &rdma,
	const std::vector<NodeConfig> &nodes,
	node_id_t my_id,
	cl::Buffer &mem)
{
	cl_int err;

	cl::Kernel k(dev.program, "rocetest_krnl", &err);
	if (err != CL_SUCCESS)
	{
		std::cout << "No rocetest_krnl in xclbin (emulation build?); "
					 "skipping RoCE setup"
				  << std::endl;
		return false;
	}

	const NodeConfig *me = nullptr, *peer = nullptr;
	for (auto &nc : nodes)
	{
		if (nc.id == my_id)
			me = &nc;
		else
			peer = &nc; // two-node first light: single peer
	}
	if (!me || !peer || me->fpga_ip.empty() || peer->fpga_ip.empty())
	{
		std::cout << "No fpga_ip configured; skipping RoCE setup"
				  << std::endl;
		return false;
	}

	OCL_CHECK(err, err = k.setArg(ARG_RPSN, (uint32_t)PSN_FIRST_LIGHT));
	OCL_CHECK(err, err = k.setArg(ARG_LPSN, (uint32_t)PSN_FIRST_LIGHT));
	OCL_CHECK(err, err = k.setArg(ARG_RQPN, (uint32_t)qpn_for(peer->id)));
	OCL_CHECK(err, err = k.setArg(ARG_LQPN, (uint32_t)qpn_for(my_id)));
	OCL_CHECK(err, err = k.setArg(ARG_RIP, ip_to_u32(peer->fpga_ip)));
	OCL_CHECK(err, err = k.setArg(ARG_LIP, ip_to_u32(me->fpga_ip)));
	OCL_CHECK(err, err = k.setArg(ARG_RUDP, (uint32_t)RDMA_UDP_PORT));
	// TODO(bring-up): confirm whether the stack expects the local or the
	// remote region base here; first light uses the peer's exposed memory
	// (the target of our outgoing reads).
	OCL_CHECK(err, err = k.setArg(ARG_VADDR,
								  (uint64_t)rdma.vaddr_table[peer->id]));
	OCL_CHECK(err, err = k.setArg(ARG_RKEY, (uint32_t)RKEY_FIRST_LIGHT));
	OCL_CHECK(err, err = k.setArg(ARG_OP, (uint32_t)ROCE_OP_NONE));
	OCL_CHECK(err, err = k.setArg(ARG_RADDR, (uint64_t)0));
	OCL_CHECK(err, err = k.setArg(ARG_LADDR, (uint64_t)0));
	OCL_CHECK(err, err = k.setArg(ARG_LEN, (uint32_t)0));
	OCL_CHECK(err, err = k.setArg(ARG_DEBUG, my_id << 2));
	OCL_CHECK(err, err = k.setArg(ARG_MEM_PTR, mem));

	OCL_CHECK(err, err = dev.q.enqueueTask(k));
	dev.q.finish();

	roce_krnl = k;
	roce_ready = true;
	std::cout << "RoCE stack configured: lQPN=0x" << std::hex
			  << qpn_for(my_id) << " rQPN=0x" << qpn_for(peer->id)
			  << std::dec << " lIP=" << me->fpga_ip
			  << " rIP=" << peer->fpga_ip << std::endl;
	return true;
}

bool roce_manual_read(
	TreeDevice &dev,
	uint64_t raddr,
	uint64_t laddr,
	uint32_t len)
{
	cl_int err;

	if (!roce_ready)
	{
		std::cerr << "roce_manual_read before configure_roce" << std::endl;
		return false;
	}
	OCL_CHECK(err, err = roce_krnl.setArg(ARG_OP, (uint32_t)ROCE_OP_READ));
	OCL_CHECK(err, err = roce_krnl.setArg(ARG_RADDR, raddr));
	OCL_CHECK(err, err = roce_krnl.setArg(ARG_LADDR, laddr));
	OCL_CHECK(err, err = roce_krnl.setArg(ARG_LEN, len));
	OCL_CHECK(err, err = dev.q.enqueueTask(roce_krnl));
	dev.q.finish();
	return true;
}
