#ifndef SM_SEARCH_HPP
#define SM_SEARCH_HPP


#include "../core/node.h"
#ifdef HLS
#include <hls_stream.h>
#include "rdma.hpp"
#include "ramstream.hpp"


//! @brief Search with SEARCH_WINDOW independent lookups in flight.
//!
//! Runs as a DATAFLOW process. Each lane holds one tagged input and walks
//! it down the tree one node at a time (local HBM read, or one RDMA read
//! into a landing slot), issuing the next fetch as soon as a node lands;
//! lanes share nothing, like the clients of the paper's fine-grained
//! design. `clients` (1..SEARCH_WINDOW) caps how many lanes are used. With SEARCH_NODE_CACHE, remote inner nodes are cached for the
//! rest of the invocation. Outputs are emitted in input order so sm_encode
//! stays in lockstep with sm_insert. Stops after the input tagged `last`;
//! filler inputs produce filler outputs.
void sm_search(
	bptr_t         root,
	node_id_t      local_id,
	Node          *hbm,
	int            local_qpn,
	ap_uint<5>     clients,
	hls::stream<search_tagged_in_t>&  input,
	hls::stream<search_tagged_out_t>& output,
	hls::stream<pkt256>&              m_axis_tx_meta,
	hls::stream<pkt32>&               s_axis_completion,
	Node                             *resp_in
);
#endif


#endif
