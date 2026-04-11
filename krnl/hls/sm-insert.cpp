#include "sm-insert.hpp"
#include "../core/insert.h"
#include "../core/memory.h"


void sm_insert(
	bptr_t&        root,
	node_id_t      local_id,
	Node          *hbm,
	int            qpn_table[MAX_KRNL_NODES],
	hls::stream<insert_in_t>&  input,
	hls::stream<insert_out_t>& output,
	hls::stream<pkt256>&       m_axis_tx_meta,
	hls::stream<pkt64>&        m_axis_tx_data,
	hls::stream<pkt64>&        s_axis_rx_data
) {
	(void)qpn_table;
	(void)m_axis_tx_meta;
	(void)m_axis_tx_data;
	(void)s_axis_rx_data;

	KvPair pair;

	if (!input.empty()) {
		input.read(pair);
		mem_context_t ctx = mem_context_local(local_id, hbm);
#ifdef __SYNTHESIS__
		output.write(insert(&root, pair.key, pair.value, &ctx, hbm));
#else
		output.write(insert(&root, pair.key, pair.value, &ctx));
#endif
	}
}
