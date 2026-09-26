#include "sm-insert.hpp"
#include "../core/insert.h"
#include "../core/memory.h"


void sm_insert(
	bptr_t&        root,
	node_id_t      local_id,
	Node          *hbm,
	hls::stream<insert_tagged_in_t>&  input,
	hls::stream<insert_tagged_out_t>& output
) {
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
