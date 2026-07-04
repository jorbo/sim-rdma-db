#include "rdma.hpp"


//! @brief Emulation-only stand-in for rocetest_krnl.
//!
//! Consumes RDMA command metadata beats and immediately returns one
//! completion token per beat. No network, no memory writes: emulation builds
//! use this to keep krnl's stream ports connected without simulating the
//! RoCE stack or CMAC hard IP (whose sim models crash xsim at init).
//!
//! The completion token payload is ignored by krnl — the handshake alone
//! synchronizes it (see fetch_node in sm-search.cpp) — so a constant is
//! returned. resp_in is never written, meaning fetches of nodes owned by a
//! remote node_id return garbage: single-node emulation only.
void roce_stub(
	hls::stream<pkt256>& s_axis_tx_meta,
	hls::stream<pkt32>&  m_axis_completion
) {
	#pragma HLS INTERFACE axis port=s_axis_tx_meta
	#pragma HLS INTERFACE axis port=m_axis_completion
	#pragma HLS INTERFACE ap_ctrl_none port=return

	if (!s_axis_tx_meta.empty()) {
		(void)s_axis_tx_meta.read();
		pkt32 token;
		token.data = 0;
		token.keep = 0xF;
		token.last = 1;
		m_axis_completion.write(token);
	}
}
