#include "test.hpp"
extern "C" {
#include "io.h"
};
#include "validate.hpp"
#include "cpp-ext.hpp"
#include <random>


void setup_data(
	std::vector<Request, aligned_allocator<Request> >& requests,
	std::vector<Response, aligned_allocator<Response> >& responses_expected,
	std::vector<Node, aligned_allocator<Node> >& memory
) {
	Request tmp_req = {.opcode = INSERT};
	Response tmp_resp = {.opcode = INSERT, .insert = SUCCESS};

	for (int i = 1; i <= TEST_N_KEYS; i++) {
		tmp_req.insert.key = i;
		tmp_req.insert.value.data = -i;
		requests.push_back(tmp_req);
		responses_expected.push_back(tmp_resp);
	}

	memory.resize(MEM_SIZE);
	memset(memory.data(), INVALID, MEM_SIZE*sizeof(Node));
	for (uint_fast64_t i = 0; i < MEM_SIZE; ++i) {
		memory.at(i).lock = LOCK_INIT;
	}
}


void setup_search_data(
	std::vector<Request, aligned_allocator<Request> >& requests,
	std::vector<Response, aligned_allocator<Response> >& responses_expected
) {
	Request tmp_req = {.opcode = SEARCH};
	Response tmp_resp = {.opcode = SEARCH};
	tmp_resp.search.status = SUCCESS;

	for (int i = 1; i <= TEST_N_KEYS; i++) {
		tmp_req.search = i;
		tmp_resp.search.value.data = -i;
		requests.push_back(tmp_req);
		responses_expected.push_back(tmp_resp);
	}
}


void setup_bench_search_data(
	size_t n,
	uint32_t seed,
	std::vector<Request, aligned_allocator<Request> >& requests,
	std::vector<Response, aligned_allocator<Response> >& responses_expected
) {
	std::mt19937 rng(seed);
	std::uniform_int_distribution<int> key_dist(1, TEST_N_KEYS);
	Request tmp_req = {.opcode = SEARCH};
	Response tmp_resp = {.opcode = SEARCH};
	tmp_resp.search.status = SUCCESS;

	requests.reserve(requests.size() + n);
	responses_expected.reserve(responses_expected.size() + n);
	for (size_t i = 0; i < n; i++) {
		const int k = key_dist(rng);
		tmp_req.search = k;
		tmp_resp.search.value.data = -k;
		requests.push_back(tmp_req);
		responses_expected.push_back(tmp_resp);
	}
}


int verify(
	std::vector<Response, aligned_allocator<Response> >& responses,
	std::vector<Response, aligned_allocator<Response> >& responses_expected,
	std::vector<Node, aligned_allocator<Node> >& memory,
	bool check_memory,
	bool quiet
) {
	bool match = true;

	if (!quiet) printf("Verifying Responses...");
	if (responses.size() != responses_expected.size()) {
		match = false;
	} else {
		for (size_t i = 0; i < responses_expected.size(); i++) {
			if (responses_expected[i] != responses[i]) {
				std::cout << "\nresponses[" << i <<"]:"
					<< "\n\texpected " << to_str(responses_expected[i])
					<< "\n\tgot " << to_str(responses[i]) << std::endl;
				match = false;
			}
		}
	}
	if (!quiet) printf("Done!\n");
	if (check_memory) {
		printf("Verifying memory contents...\n");
		match = check_inserted_leaves(memory.data()) && match;
		printf("Done!\n");

		dump_node_list(stdout, memory.data());
	}

	if (!quiet)
		std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
	return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
