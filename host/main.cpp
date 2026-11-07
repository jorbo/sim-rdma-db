#include "host.hpp"
#include "bootstrap.hpp"
#include "roce-setup.hpp"
#include "test.hpp"
#include "run-tree.hpp"
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>


//! Node id that owns the tree and serves peers' RDMA reads.
#define TABLE_NODE_ID 0


static void print_rdma(const RdmaConfig& rdma, size_t n_nodes) {
	std::cout << "Bootstrap complete. Node " << (int)rdma.my_node_id << ":";
	for (size_t i = 0; i < n_nodes; ++i)
		std::cout << " [" << i << "] qpn=0x" << std::hex
		          << rdma.qpn_table[i] << " vaddr=0x"
		          << rdma.vaddr_table[i] << " root=0x"
		          << rdma.root_table[i] << std::dec;
	std::cout << std::endl;
}


static bool wait_for_roce_peers(
	node_id_t my_id,
	const std::vector<NodeConfig>& nodes,
	bool local_ready
) {
	std::cout << "Waiting for peer RoCE setup..." << std::endl;
	try {
		if (!synchronize_roce_ready(my_id, nodes, local_ready)) {
			std::cerr << "RoCE setup failed on this node or a peer" << std::endl;
			return false;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "RoCE readiness barrier failed: " << e.what() << std::endl;
		return false;
	}
	std::cout << "All RoCE endpoints ready." << std::endl;
	return true;
}


//! Positive integer from the environment, or 0 when unset/invalid.
static long env_long(const char* name) {
	const char* v = getenv(name);
	if (v == nullptr || *v == '\0') return 0;
	char* end = nullptr;
	const long n = std::strtol(v, &end, 10);
	return (end != nullptr && *end == '\0' && n > 0) ? n : 0;
}


//! BENCH_SEARCHES mode: run the whole request list in kernel-sized batches
//! (KRNL_MAX_REQUESTS each), verify every batch, and print one BENCH
//! summary line plus a single TEST verdict. Kernel-only throughput comes
//! from the profile summary (sum of krnl_1 CU time, XRT excluded); the
//! wall-clock numbers here include XRT launch overhead per batch.
static int run_bench(
	TreeDevice& dev,
	TreeInput& input,
	std::vector<Response, aligned_allocator<Response> >& expected,
	const RdmaConfig& rdma
) {
	const std::vector<Request, aligned_allocator<Request> > all = input.requests;
	const size_t n = all.size();
	const char* clients_env = getenv("SEARCH_CLIENTS");
	const int clients = clients_env != nullptr ? std::atoi(clients_env) : 0;

	std::vector<double> batch_ms;
	size_t passed = 0;
	for (size_t off = 0; off < n; off += KRNL_MAX_REQUESTS) {
		const size_t len = std::min<size_t>(KRNL_MAX_REQUESTS, n - off);
		input.requests.assign(all.begin() + off, all.begin() + off + len);
		std::vector<Response, aligned_allocator<Response> > exp(
			expected.begin() + off, expected.begin() + off + len);

		double ms = 0;
		TreeOutput output = run_fpga_tree(dev, input, rdma, /*quiet=*/true, &ms);
		batch_ms.push_back(ms);
		if (verify(output.responses, exp, output.memory,
		           /*check_memory=*/false, /*quiet=*/true) == EXIT_SUCCESS)
			passed++;
		else
			std::cerr << "BENCH batch " << off / KRNL_MAX_REQUESTS
			          << " FAILED" << std::endl;
	}
	// assign(), not operator=: copy assignment compares allocators and
	// aligned_allocator (alloc.hpp) defines no operator!=.
	input.requests.assign(all.begin(), all.end());

	const size_t batches = batch_ms.size();
	double total_ms = 0;
	for (double ms : batch_ms) total_ms += ms;
	std::vector<double> sorted = batch_ms;
	std::sort(sorted.begin(), sorted.end());
	const double median_ms = batches ? sorted[batches / 2] : 0;

	printf("BENCH clients=%d searches=%zu batches=%zu passed=%zu/%zu "
	       "wall_comp_ms=%.3f median_batch_ms=%.4f lookups_per_s_wall=%.0f\n",
	       clients, n, batches, passed, batches, total_ms, median_ms,
	       total_ms > 0 ? n / (total_ms / 1000.0) : 0.0);
	printf("BENCH note: kernel-only rate = searches / (krnl_1 Total Time in "
	       "summary.csv); wall numbers include XRT launch overhead\n");

	const bool ok = batches > 0 && passed == batches;
	std::cout << "TEST " << (ok ? "PASSED" : "FAILED") << std::endl;
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}


int main(int argc, char** argv) {
	if (argc != 4) {
		std::cout << "Usage: " << argv[0]
		          << " <xclbin> <my_node_id> <nodes_config>" << std::endl;
		std::cout << "  nodes_config: text file, one"
		             " 'node_id host_ip [fpga_ip]' per line" << std::endl;
		return EXIT_FAILURE;
	}

	const std::string xclbin      = argv[1];
	node_id_t         my_id       = (node_id_t)std::atoi(argv[2]);
	const std::string config_path = argv[3];
	auto nodes = parse_node_config(config_path);

	TreeInput input;
	std::vector<Response, aligned_allocator<Response> > responses_expected;

	if (my_id == TABLE_NODE_ID) {
		// Table node: build the tree with local inserts and verify it.
		// With peers configured, then advertise the final root and serve
		// their RDMA reads; single-node configs stop after verification
		// (the original regression test).
		setup_data(input.requests, responses_expected, input.memory);
		TreeDevice dev = tree_device_setup(xclbin, input);

		// The local build touches no peer state; self-only config suffices.
		RdmaConfig local;
		memset(&local, 0, sizeof(local));
		local.my_node_id = my_id;

		TreeOutput output = run_fpga_tree(dev, input, local);
		int rc = verify(output.responses, responses_expected, output.memory);
		if (nodes.size() <= 1 || rc != EXIT_SUCCESS)
			return rc;

		RdmaConfig rdma = bootstrap_rdma(my_id, nodes, qpn_for(my_id),
		                                 dev.memory_vaddr, output.root);
		print_rdma(rdma, nodes.size());
		bool roce = configure_roce(dev, rdma, nodes, my_id,
		                           dev.buffer_memory);
		if (!wait_for_roce_peers(my_id, nodes, roce)) {
			return EXIT_FAILURE;
		}
		std::cout << "Table node serving; Ctrl-C to exit." << std::endl;
		for (;;) pause();
	} else {
		// Head node: no local tree; search the table node's tree remotely.
		// BENCH_SEARCHES=N swaps the 22-key test for N uniform-random
		// lookups run in kernel-sized batches (BENCH_SEED, default 1).
		const long bench_searches = env_long("BENCH_SEARCHES");
		if (bench_searches > 0) {
			const long seed = env_long("BENCH_SEED");
			setup_bench_search_data((size_t)bench_searches,
			                        seed > 0 ? (uint32_t)seed : 1u,
			                        input.requests, responses_expected);
			std::cout << "BENCH_SEARCHES=" << bench_searches
			          << " (batches of " << KRNL_MAX_REQUESTS << ")"
			          << std::endl;
		} else {
			setup_search_data(input.requests, responses_expected);
		}
		TreeDevice dev = tree_device_setup(xclbin, input);

		RdmaConfig rdma = bootstrap_rdma(my_id, nodes, qpn_for(my_id),
		                                 dev.memory_vaddr, /*local_root=*/0);
		print_rdma(rdma, nodes.size());
		bool roce = configure_roce(dev, rdma, nodes, my_id,
		                           dev.buffer_rdma);
		// Do not let either the optional probe or the B-tree search emit the
		// first packet until every endpoint has finished programming its QP.
		if (!wait_for_roce_peers(my_id, nodes, roce)) {
			return EXIT_FAILURE;
		}

		const char* ila_arming = getenv("ILA_ARMING");
		if (ila_arming != nullptr && std::strcmp(ila_arming, "1") == 0) {
			std::cout << "ILA_ARMING=1: arm the ILA, then press ENTER to "
			             "start the head workload..." << std::flush;
			std::string line;
			if (!std::getline(std::cin, line)) {
				std::cerr << std::endl
				          << "ILA arming input closed; aborting without starting "
				             "the workload" << std::endl;
				return EXIT_FAILURE;
			}
			std::cout << "ILA armed; starting head workload." << std::endl;
		}

		// Bring-up step 3: host-driven RDMA READ of the table node's
		// first node into local offset 0, no B-tree kernel involved.
		// Run with RDMA_SELFTEST=1 and inspect the landing bytes.
		const char* rdma_selftest = getenv("RDMA_SELFTEST");
		if (roce && rdma_selftest != nullptr &&
		    std::strcmp(rdma_selftest, "1") == 0) {
			std::cout << "RDMA selftest: reading " << sizeof(Node)
			          << " bytes from table node..." << std::endl;
			roce_manual_read(dev, /*raddr=*/0, /*laddr=*/0, sizeof(Node));
			// Landing pad = start of our exposed memory. Expect the table
			// node's leaf 0: keys 1,2 then INVALID (0xffffffff).
			cl_int err;
			OCL_CHECK(err, err = dev.q.enqueueMigrateMemObjects(
				{dev.buffer_rdma}, CL_MIGRATE_MEM_OBJECT_HOST));
			dev.q.finish();
			const uint32_t* w = (const uint32_t*)dev.rdma_landing.data();
			std::cout << "RDMA selftest landing bytes:" << std::hex;
			for (size_t i = 0; i < sizeof(Node) / sizeof(uint32_t); ++i)
				std::cout << " " << w[i];
			std::cout << std::dec << std::endl;
			std::cout << "  (expect: 1 2 ffffffff ffffffff ... for leaf 0)"
			          << std::endl;
		}

		// The 22-key workload always splits the root off leaf 0, so a
		// zero root means the table node advertised nothing.
		input.root = rdma.root_table[TABLE_NODE_ID];
		if (input.root == 0) {
			std::cerr << "Table node advertised no tree root" << std::endl;
			return EXIT_FAILURE;
		}

		if (bench_searches > 0)
			return run_bench(dev, input, responses_expected, rdma);

		TreeOutput output = run_fpga_tree(dev, input, rdma);
		return verify(output.responses, responses_expected, output.memory,
		              /*check_memory=*/false);
	}
}
