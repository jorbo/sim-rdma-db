#ifndef RAMSTREAM_HPP
#define RAMSTREAM_HPP


#include "../core/operations.h"
#include "../core/defs.h"
#include <hls_stream.h>


#ifndef REQUEST_OFFSET
#define REQUEST_OFFSET (MEM_SIZE * sizeof(Node))
#endif

#ifndef NUM_REQUESTS
#define NUM_REQUESTS (0x100)
#endif

#ifndef RESPONSE_OFFSET
#define RESPONSE_OFFSET (REQUEST_OFFSET + (NUM_REQUESTS * sizeof(Request)))
#endif


//! Tagged stream element for requests flowing through the DATAFLOW pipeline.
//! `last` marks the final element in the stream; downstream processes terminate
//! after observing it. The payload may be valid or a pure sentinel — check
//! opcode against NOP if the process needs to ignore empty flushes.
struct req_tagged_t {
	Request req;
	bool    last;
};

//! Tagged stream element for decoded search inputs. `last` marks terminator.
//! `has_payload` is false for a pure terminator sentinel used when no search
//! ops were present in this invocation (so that sm_search still runs exactly
//! one iteration and exits cleanly).
struct search_tagged_in_t {
	search_in_t key;
	bool        last;
	bool        has_payload;
};

struct insert_tagged_in_t {
	KvPair pair;
	bool   last;
	bool   has_payload;
};

struct search_tagged_out_t {
	search_out_t val;
	bool         last;
	bool         has_payload;
};

struct insert_tagged_out_t {
	insert_out_t status;
	bool         last;
	bool         has_payload;
};

struct resp_tagged_t {
	Response resp;
	bool     last;
	bool     has_payload;
};


//! @brief Read requests from DRAM into a tagged stream. Sets `last=true` on
//!        the final item so downstream processes can exit.
void sm_ramstream_req(
	hls::stream<req_tagged_t>& requests,
	Request *req_buffer,
	int num_requests
);

//! @brief Drain responses from a tagged stream to DRAM. Terminates on `last`.
void sm_ramstream_resp(
	hls::stream<resp_tagged_t>& responses,
	Response *resp_buffer
);

//! @brief No-op kept for backward compatibility with existing testbench code.
void reset_ramstream_offsets();


#endif
