#include "sm-insert.hpp"
#include "../core/insert.h"
#include "../core/memory.h"


void sm_insert(
	bptr_t&        root,
	node_id_t      local_id,
	Node          *hbm,
	int            qpn_table[MAX_KRNL_NODES],
	hls::stream<insert_tagged_in_t>&  input,
	hls::stream<insert_tagged_out_t>& output,
	hls::stream<pkt256>&              m_axis_tx_meta,
	hls::stream<pkt64>&               s_axis_rx_data
) {
	(void)qpn_table;
	(void)m_axis_tx_meta;
	(void)s_axis_rx_data;

	insert_loop: for (;;) {
		#pragma HLS loop_tripcount max=NUM_REQUESTS
		insert_tagged_in_t in = input.read();

		insert_tagged_out_t out;
		out.last        = in.last;
		out.has_payload = in.has_payload;
		out.status      = insert_out_t();

		if (in.has_payload) {
			mem_context_t ctx = mem_context_local(local_id, hbm);
#ifdef __SYNTHESIS__
			out.status = insert(&root, in.pair.key, in.pair.value, &ctx, hbm);
#else
			out.status = insert(&root, in.pair.key, in.pair.value, &ctx);
#endif
		}
		output.write(out);

		if (in.last) break;
	}
}
