#ifndef TEST_HELPERS_HPP
#define TEST_HELPERS_HPP


extern "C" {
#include "../core/node.h"
};
#include <cstddef>


#define VERBOSE
#define INPUT_SEARCH(x) \
	req_buffer[offset++] = encode_search_req(x); \
	input_log.write(x);
#define INPUT_INSERT(key_, value_) \
	last_in.key = key_; last_in.value.data = value_; \
	req_buffer[offset++] = encode_insert_req(last_in); \
	input_log.write(last_in);
#define SET_IKV(addr, i, key_, value_) \
	hbm[bptr_node_id(addr)*MAX_NODES_PER_LEVEL + bptr_local_addr(addr)].keys[i] = key_; \
	hbm[bptr_node_id(addr)*MAX_NODES_PER_LEVEL + bptr_local_addr(addr)].values[i].data = value_;
#define SET_IKP(addr, i, key_, ptr_) \
	hbm[bptr_node_id(addr)*MAX_NODES_PER_LEVEL + bptr_local_addr(addr)].keys[i] = key_; \
	hbm[bptr_node_id(addr)*MAX_NODES_PER_LEVEL + bptr_local_addr(addr)].values[i].ptr = ptr_;
//! @brief Declare a 2D memory view of a flat hbm array for use with core APIs.
//! Usage: DECLARE_MEMORY_VIEW(memory, hbm) — creates Node *memory[MAX_LEVELS]
#define DECLARE_MEMORY_VIEW(mem_, flat_) \
	Node *mem_[MAX_LEVELS]; \
	for (int _i_ = 0; _i_ < MAX_LEVELS; _i_++) mem_[_i_] = (flat_) + _i_ * MAX_NODES_PER_LEVEL;
#define KERNEL_ARG_DECS \
	bptr_t *root, Node *hbm, Request *req_buffer, Response *resp_buffer, \
	int loop_max, int op_max, bool reset
#define DECLARE_RDMA_ARGS \
	node_id_t my_node_id = 0; \
	int local_qpn = 0; \
	hls::stream<pkt256> m_axis_tx_meta; \
	hls::stream<pkt32>  s_axis_completion; \
	Node resp_in_slot = {}; \
	Node *resp_in = &resp_in_slot;
#define KERNEL_ARG_VARS \
	root, hbm, req_buffer, resp_buffer, loop_max, op_max, reset, \
	my_node_id, local_qpn, m_axis_tx_meta, s_axis_completion, resp_in

//! Simulate exactly one in-flight remote RDMA-read response.
//!
//! Call BEFORE invoking krnl(...). Stores `fake_node` in resp_in_slot and
//! pre-pushes one completion token into s_axis_completion so the kernel's
//! fetch_node will (a) emit its RDMA-read meta into m_axis_tx_meta,
//! (b) read the pre-staged completion token, then (c) read the pre-staged Node
//! from resp_in[0].
//!
//! Requires that `DECLARE_RDMA_ARGS` has already been expanded in scope.
//! First-light limitation: at most one outstanding remote fetch per kernel run.
//! Multiple remote fetches in one run will all read the same resp_in_slot.
#define SIMULATE_REMOTE_FETCH(fake_node) \
	do { \
		resp_in_slot = (fake_node); \
		pkt32 _completion_tok; \
		_completion_tok.data = 0; \
		_completion_tok.keep = 0xF; \
		_completion_tok.last = 1; \
		s_axis_completion.write(_completion_tok); \
	} while (0)


//!@brief Print a hex dump of a section of HBM grouped by object
void hbm_dump(
	//! Memory buffer to read from
	uint8_t* hbm,
	//! Offset at which to start the dump
	uint_fast64_t offset,
	//! Size in bytes of object to group by
	size_t size,
	//! Number of objects to print
	uint_fast64_t length
);


#endif
