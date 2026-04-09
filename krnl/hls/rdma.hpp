#pragma once

#include <hls_stream.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>


//! Maximum number of remote nodes the HLS kernel can address via RDMA
#define MAX_KRNL_NODES 16u

typedef ap_axiu<512, 0, 0, 0> pkt512;
typedef ap_axiu<256, 0, 0, 0> pkt256;
typedef ap_axiu<128, 0, 0, 0> pkt128;
typedef ap_axiu<64, 0, 0, 0> pkt64;
typedef ap_axiu<32, 0, 0, 0> pkt32;
typedef ap_axiu<16, 0, 0, 0> pkt16;
typedef ap_axiu<8, 0, 0, 0> pkt8;


//! @brief One-sided RDMA read operation
void rdma_read(
	//! Local Queue Pair Number
	int s_axi_lqpn,
	//! Local address to write to
	ap_uint<64> s_axi_laddr,
	//! Remote address to read from
	ap_uint<64> s_axi_raddr,
	//! Length in bytes of the data to read
	int s_axi_len,
	//! Carries opcode for the network kernel
	hls::stream<pkt256>& m_axis_tx_meta
);

//! @brief One-sided RDMA write operation
void rdma_write(
	//! Local Queue Pair Number
	int s_axi_lqpn,
	//! Local address to read from
	ap_uint<64> s_axi_laddr,
	//! Remote address to write to
	ap_uint<64> s_axi_raddr,
	//! Length in bytes of the data to write
	int s_axi_len,
	//! Value to write
	ap_uint<64> write_value,
	//! Opcode for the network kernel
	hls::stream<pkt256>& m_axis_tx_meta,
	//! Data stream for the network kernel
	hls::stream<pkt64>& m_axis_tx_data
);

void rdma_write_through(
	//! Local Queue Pair Number
	int s_axi_lqpn,
	//! Local address to read from
	ap_uint<64> s_axi_laddr,
	//! Remote address to write to
	ap_uint<64> s_axi_raddr,
	//! Length in bytes of the data to read
	int s_axi_len,
	//! Value to write
	ap_uint<64> write_value,
	//! Opcode for the network kernel
	hls::stream<pkt256>& m_axis_tx_meta,
	//! Data stream for the network kernel
	hls::stream<pkt64>& m_axis_tx_data
);

void rdma_bram_read(
	//! Local Queue Pair Number
	int s_axi_lqpn,
	//! Local address to write to
	ap_uint<64> s_axi_laddr,
	//! Remote address to read from
	ap_uint<64> s_axi_raddr,
	//! Length in bytes of the data to read
	int s_axi_len,
	//! Opcode for the network kernel
	hls::stream<pkt256>& m_axis_tx_meta
);

void rdma_bram_write(
	//! Local Queue Pair Number
	int s_axi_lqpn,
	//! Local address to read from
	ap_uint<64> s_axi_laddr,
	//! Remote address to write to
	ap_uint<64> s_axi_raddr,
	//! Length in bytes of the data to read
	int s_axi_len,
	//! Value to write
	ap_uint<64> write_value,
	//! Opcode for the network kernel
	hls::stream<pkt256>& m_axis_tx_meta,
	//! Data stream for the network kernel
	hls::stream<pkt64>& m_axis_tx_data
);
