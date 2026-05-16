#pragma once

#include <hls_stream.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>

typedef ap_axiu<512, 0, 0, 0> pkt512;
typedef ap_axiu<256, 0, 0, 0> pkt256;
typedef ap_axiu<64,  0, 0, 0> pkt64;

// Width adapter between krnl (256b meta / 64b rx) and rocetest_krnl role
// interfaces (512b meta / data / status / rx). krnl already packs the 160-bit
// RoCE-legacy meta layout in the low bits of its pkt256; this kernel zero-pads
// to 512b and applies the qpn[26:7] = 0 mask the RoCE stack expects.
//
// First-light scope: read-path functional. tx_data is driven with zero-payload
// beats on WRITE opcodes purely to keep the port live for cfgen; krnl does not
// produce real write payload yet (sm-insert is a stub).
void bridge_krnl(
    hls::stream<pkt256>& s_axis_krnl_meta,
    hls::stream<pkt512>& m_axis_role_meta,
    hls::stream<pkt512>& m_axis_role_data,
    hls::stream<pkt512>& s_axis_role_status,
    hls::stream<pkt512>& s_axis_role_rx_data,
    hls::stream<pkt64>&  m_axis_krnl_rx_data
);
