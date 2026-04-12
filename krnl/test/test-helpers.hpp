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
	int qpn_table[MAX_KRNL_NODES] = {}; \
	hls::stream<pkt256> m_axis_tx_meta; \
	hls::stream<pkt64>  s_axis_rx_data;
#define KERNEL_ARG_VARS \
	root, hbm, req_buffer, resp_buffer, loop_max, op_max, reset, \
	my_node_id, qpn_table, m_axis_tx_meta, s_axis_rx_data


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
