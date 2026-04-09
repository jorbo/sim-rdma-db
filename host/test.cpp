#include "test.hpp"
extern "C" {
#include "io.h"
};
#include "validate.hpp"
#include "cpp-ext.hpp"


void setup_data(
	std::vector<Request, aligned_allocator<Request> >& requests,
	std::vector<Response, aligned_allocator<Response> >& responses_expected,
	std::vector<Node, aligned_allocator<Node> >& memory
) {
	Request tmp_req = {.opcode = INSERT};
	Response tmp_resp = {.opcode = INSERT, .insert = SUCCESS};

	for (int i = 1; i <= 22; i++) {
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


int verify(
	std::vector<Response, aligned_allocator<Response> >& responses,
	std::vector<Response, aligned_allocator<Response> >& responses_expected,
	std::vector<Node, aligned_allocator<Node> >& memory
) {
	bool match = true;

	printf("Verifying Responses...");
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
	printf("Done!\n");
	printf("Verifying memory contents...\n");
	check_inserted_leaves(memory.data());
	printf("Done!\n");

	dump_node_list(stdout, memory.data());

	std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
	return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
