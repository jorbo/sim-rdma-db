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


//! @brief Convert a DRAM buffer of requests to an HLS stream
void sm_ramstream_req(
	//! [out] Streams of requests decoded from memory
	hls::stream<Request>& requests,
	//! [in]  Pointer to high bandwidth memory
	Request *req_buffer,
	//! [in]  When true, reset internal offset and state (must be called before loop)
	bool do_reset
);

//! @brief Write an HLS stream of responses to a DRAM buffer
void sm_ramstream_resp(
	//! [in]  Streams of responses to be encoded in memory
	hls::stream<Response>& responses,
	//! [out] Pointer to high bandwidth memory
	Response *resp_buffer,
	//! [in]  When true, reset internal offset and state (must be called before loop)
	bool do_reset
);

//! @brief No-op kept for backward compatibility. Reset is now handled via
//!        do_reset parameter inside the kernel's reset path.
void reset_ramstream_offsets();


#endif
