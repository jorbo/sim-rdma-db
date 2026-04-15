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
	hls::stream<pkt64>&  s_axis_rx_data
) {
	#pragma HLS INTERFACE m_axi port=root        bundle=gmem3 depth=1   offset=slave latency=64 num_read_outstanding=16 num_write_outstanding=16 max_read_burst_length=1  max_write_burst_length=1
	#pragma HLS INTERFACE m_axi port=hbm         bundle=gmem0 depth=40  offset=slave latency=64 num_read_outstanding=16 num_write_outstanding=16 max_read_burst_length=16 max_write_burst_length=16
	#pragma HLS INTERFACE m_axi port=req_buffer  bundle=gmem1 depth=256 offset=slave latency=64 num_read_outstanding=16 num_write_outstanding=16 max_read_burst_length=16 max_write_burst_length=16
	#pragma HLS INTERFACE m_axi port=resp_buffer bundle=gmem2 depth=256 offset=slave latency=64 num_read_outstanding=16 num_write_outstanding=16 max_read_burst_length=16 max_write_burst_length=16
	#pragma HLS INTERFACE s_axilite port=return    bundle=control
	#pragma HLS INTERFACE s_axilite port=loop_max  bundle=control
	#pragma HLS INTERFACE s_axilite port=op_max    bundle=control
	#pragma HLS INTERFACE s_axilite port=reset     bundle=control
	#pragma HLS INTERFACE s_axilite port=my_node_id bundle=control
	#pragma HLS INTERFACE m_axi     port=qpn_table bundle=gmem4 depth=16 offset=slave latency=64 num_read_outstanding=16 num_write_outstanding=16 max_read_burst_length=16 max_write_burst_length=16
	#pragma HLS INTERFACE axis port=m_axis_tx_meta depth=64
	#pragma HLS INTERFACE axis port=s_axis_rx_data depth=64

	// Snapshot of the persistent root across invocations. We keep a static
	// copy so subsequent non-reset calls see the updated tree, but we copy
	// into a local before the DATAFLOW region so the static does not leak
	// into the dataflow processes (which would break HLS DATAFLOW rules).
	static bptr_t persistent_root = bptr_make(0, 0);
	if (reset) {
		persistent_root = *root;
	}
	bptr_t root_for_search = persistent_root;
	bptr_t root_for_insert = persistent_root;

	// Bound the number of requests by the caller-supplied op_max (clamped to
	// NUM_REQUESTS). sm_ramstream_req stops early at the first NOP regardless.
	int num_requests = op_max;
	if (num_requests <= 0 || num_requests > (int)NUM_REQUESTS) {
		num_requests = (int)NUM_REQUESTS;
	}
	(void)loop_max; // No longer needed: DATAFLOW self-terminates on `last`.

	hls::stream<req_tagged_t>         requests;
	hls::stream<resp_tagged_t>        responses;
	hls::stream<search_tagged_in_t>   searchInput;
	hls::stream<insert_tagged_in_t>   insertInput;
	hls::stream<search_tagged_out_t>  searchOutput;
	hls::stream<insert_tagged_out_t>  insertOutput;
	#pragma HLS stream variable=requests     type=fifo depth=0x100
	#pragma HLS stream variable=responses    type=fifo depth=0x100
	#pragma HLS stream variable=searchInput  type=fifo depth=0x100
	#pragma HLS stream variable=insertInput  type=fifo depth=0x100
	#pragma HLS stream variable=searchOutput type=fifo depth=0x100
	#pragma HLS stream variable=insertOutput type=fifo depth=0x100

	#pragma HLS DATAFLOW
	sm_ramstream_req(requests, req_buffer, num_requests);
	sm_decode(requests, searchInput, insertInput);
	sm_search(
		root_for_search, my_node_id, hbm, qpn_table,
		searchInput, searchOutput,
		m_axis_tx_meta, s_axis_rx_data
	);
	sm_insert(
		root_for_insert, my_node_id, hbm, qpn_table,
		insertInput, insertOutput,
		m_axis_tx_meta, s_axis_rx_data
	);
	sm_encode(responses, searchOutput, insertOutput);
	sm_ramstream_resp(responses, resp_buffer);

	// Propagate any root change made by sm_insert back to the persistent
	// register and to the m_axi-backed scalar.
	persistent_root = root_for_insert;
	*root = persistent_root;
}
