#include "sm-operations.hpp"
#include "sm-search.hpp"
#include "sm-insert.hpp"
#include "ramstream.hpp"
#include "krnl.hpp"


void krnl(
	bptr_t *root,
	Node *hbm,
	Request *req_buffer,
	Response *resp_buffer,
	int loop_max,
	int op_max,
	bool reset
) {
	#pragma HLS INTERFACE m_axi port=root bundle=gmem1
	#pragma HLS INTERFACE m_axi port=hbm bundle=gmem0
	#pragma HLS INTERFACE m_axi port=req_buffer bundle=gmem1
	#pragma HLS INTERFACE m_axi port=resp_buffer bundle=gmem2
	#pragma HLS INTERFACE s_axilite port=loop_max
	#pragma HLS INTERFACE s_axilite port=op_max
	#pragma HLS INTERFACE s_axilite port=reset

	static hls::stream<Request> requests;
	static hls::stream<Response> responses;
	static hls::stream<search_in_t> searchInput;
	static hls::stream<insert_in_t> insertInput;
	static hls::stream<search_out_t> searchOutput;
	static hls::stream<insert_out_t> insertOutput;
	#pragma HLS stream variable=requests type=fifo depth=0x100
	#pragma HLS stream variable=responses type=fifo depth=0x100
	#pragma HLS stream variable=searchInput type=fifo depth=0x100
	#pragma HLS stream variable=insertInput type=fifo depth=0x100
	#pragma HLS stream variable=searchOutput type=fifo depth=0x100
	#pragma HLS stream variable=insertOutput type=fifo depth=0x100

	Node *memory[MAX_LEVELS];
	for (int i = 0; i < MAX_LEVELS; i++)
		memory[i] = hbm + i * MAX_NODES_PER_LEVEL;

	static bptr_t current_root = bptr_make(0, 0);
	uint_fast32_t step_count = 0;
	uint_fast32_t ops_in = 0;
	uint_fast32_t ops_out = 0;
	if (reset) {
		step_count = 0;
		ops_in = 0;
		ops_out = 0;
		current_root = *root;
	}

	while (ops_out < op_max && step_count++ < loop_max) {
		sm_search(
			current_root,
			searchInput, searchOutput,
			(Node const **) memory
		);
		sm_insert(
			current_root,
			insertInput, insertOutput,
			(Node **) memory
		);
		sm_ramstream_req(requests, req_buffer);
		sm_ramstream_resp(responses, resp_buffer);
		sm_decode(requests, searchInput, insertInput, ops_in);
		sm_encode(responses, searchOutput, insertOutput, ops_out);
	}
	*root = current_root;

	return;
}
