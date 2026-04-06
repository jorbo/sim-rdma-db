#ifndef KRNL_HPP
#define KRNL_HPP


extern "C" {
#include "../core/node.h"
#include "../core/operations.h"
};


void krnl(
	//! [inout] Encoded root address shared with the caller
	bptr_t *root,
	//! [inout] Pointer to on-chip high-bandwidth memory
	Node *hbm,
	//! [in]    Buffer to hold the list of operation requests
	Request *req_buffer,
	//! [out]   Buffer to hold responses from operations
	Response *resp_buffer,
	//! [in]    Maximum number of main loop iterations to attempt to execute
	int loop_max,
	//! [in]    Maximum number of operations to attempt to execute
	int op_max,
	//! [in]    Reset operation counter register
	bool reset
);


#endif
