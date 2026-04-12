#include "sm-operations.hpp"
#include "sm-search.hpp"
#include "sm-insert.hpp"
#include "ramstream.hpp"
#include "krnl.hpp"


void krnl(
	bptr_t       *root,
	Node         *hbm,
	Request      *req_buffer,
	Response     *resp_buffer,
	int           loop_max,
	int           op_max,
	bool          reset,
	node_id_t     my_node_id,
	int           qpn_table[MAX_KRNL_NODES],
	hls::stream<pkt256>& m_axis_tx_meta,
	hls::stream<pkt64>&  m_axis_tx_data,
	hls::stream<pkt64>&  s_axis_rx_data
) {
	#pragma HLS INTERFACE m_axi port=root        bundle=gmem3 depth=1
	#pragma HLS INTERFACE m_axi port=hbm         bundle=gmem0 depth=40
	#pragma HLS INTERFACE m_axi port=req_buffer  bundle=gmem1 depth=256
	#pragma HLS INTERFACE m_axi port=resp_buffer bundle=gmem2 depth=256
	#pragma HLS INTERFACE s_axilite port=return
	#pragma HLS INTERFACE s_axilite port=loop_max
	#pragma HLS INTERFACE s_axilite port=op_max
	#pragma HLS INTERFACE s_axilite port=reset
	#pragma HLS INTERFACE s_axilite port=my_node_id
	#pragma HLS INTERFACE m_axi     port=qpn_table bundle=gmem4 depth=MAX_KRNL_NODES
	#pragma HLS INTERFACE axis port=m_axis_tx_meta
	#pragma HLS INTERFACE axis port=m_axis_tx_data
	#pragma HLS INTERFACE axis port=s_axis_rx_data

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
			my_node_id,
			hbm,
			qpn_table,
			searchInput, searchOutput,
			m_axis_tx_meta, s_axis_rx_data
		);
		sm_insert(
			current_root,
			my_node_id,
			hbm,
			qpn_table,
			insertInput, insertOutput,
			m_axis_tx_meta, m_axis_tx_data, s_axis_rx_data
		);
		sm_ramstream_req(requests, req_buffer);
		sm_ramstream_resp(responses, resp_buffer);
		sm_decode(requests, searchInput, insertInput, ops_in);
		sm_encode(responses, searchOutput, insertOutput, ops_out);
	}
	*root = current_root;

	return;
}
