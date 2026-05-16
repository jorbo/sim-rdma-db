#include "bridge_krnl.hpp"

static void bridge_meta_and_data(
    hls::stream<pkt256>& s_axis_krnl_meta,
    hls::stream<pkt512>& m_axis_role_meta,
    hls::stream<pkt512>& m_axis_role_data
) {
    #pragma HLS inline off

    bridge_loop: while (1) {
        #pragma HLS pipeline II=1
        pkt256 in = s_axis_krnl_meta.read();

        ap_uint<3>  opcode = in.data.range(2, 0);
        ap_uint<32> length = in.data.range(154, 123);

        pkt512 meta;
        meta.data = 0;
        meta.data.range(159, 0) = in.data.range(159, 0);
        meta.data.range(26, 7)  = 0;
        meta.keep = 0;
        meta.keep.range(19, 0)  = 0xFFFFF;
        meta.last = 1;
        m_axis_role_meta.write(meta);

        if (opcode == 1) {
            ap_uint<32> beats = (length + 63) >> 6;
            if (beats == 0) beats = 1;
            data_beats: for (ap_uint<32> i = 0; i < beats; i++) {
                #pragma HLS pipeline II=1
                #pragma HLS loop_tripcount max=64
                pkt512 d;
                d.data = 0;
                d.keep = ~ap_uint<64>(0);
                d.last = (i == beats - 1) ? 1 : 0;
                m_axis_role_data.write(d);
            }
        }
    }
}

static void bridge_rx(
    hls::stream<pkt512>& s_axis_role_rx_data,
    hls::stream<pkt64>&  m_axis_krnl_rx_data
) {
    #pragma HLS inline off

    rx_loop: while (1) {
        #pragma HLS pipeline II=1
        pkt512 in = s_axis_role_rx_data.read();
        pkt64 out;
        out.data = in.data.range(63, 0);
        out.keep = 0xFF;
        out.last = in.last;
        m_axis_krnl_rx_data.write(out);
    }
}

static void status_sink(
    hls::stream<pkt512>& s_axis_role_status
) {
    #pragma HLS inline off

    status_loop: while (1) {
        #pragma HLS pipeline II=1
        pkt512 unused = s_axis_role_status.read();
        (void)unused;
    }
}

void bridge_krnl(
    hls::stream<pkt256>& s_axis_krnl_meta,
    hls::stream<pkt512>& m_axis_role_meta,
    hls::stream<pkt512>& m_axis_role_data,
    hls::stream<pkt512>& s_axis_role_status,
    hls::stream<pkt512>& s_axis_role_rx_data,
    hls::stream<pkt64>&  m_axis_krnl_rx_data
) {
    #pragma HLS interface axis port=s_axis_krnl_meta
    #pragma HLS interface axis port=m_axis_role_meta
    #pragma HLS interface axis port=m_axis_role_data
    #pragma HLS interface axis port=s_axis_role_status
    #pragma HLS interface axis port=s_axis_role_rx_data
    #pragma HLS interface axis port=m_axis_krnl_rx_data
    #pragma HLS interface ap_ctrl_none port=return
    #pragma HLS dataflow

    bridge_meta_and_data(s_axis_krnl_meta, m_axis_role_meta, m_axis_role_data);
    bridge_rx(s_axis_role_rx_data, m_axis_krnl_rx_data);
    status_sink(s_axis_role_status);
}
