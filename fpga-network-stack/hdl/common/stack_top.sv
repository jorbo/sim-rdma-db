/*
 * Copyright (c) 2019, Systems Group, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
`timescale 1ns / 1ps
`default_nettype none

`define IP_VERSION4
// `define POINTER_CHASING


module stack_top #(
    parameter NET_BANDWIDTH = 10,
    parameter WIDTH = 512,
    parameter MAC_ADDRESS = 48'hE59D02350A00, // LSB first, 00:0A:35:02:9D:E5
    parameter IPV6_ADDRESS= 128'hE59D_02FF_FF35_0A02_0000_0000_0000_80FE, //LSB first: FE80_0000_0000_0000_020A_35FF_FF02_9DE5,
    parameter IP_SUBNET_MASK = 32'h00FFFFFF,
    parameter IP_DEFAULT_GATEWAY = 32'h00000000,
    parameter DHCP_EN   = 0,
    parameter RX_DDR_BYPASS_EN = 0,
    parameter ROCE_EN = 1
)(
    input wire          net_clk,
    input wire          net_aresetn,

    // network interface streams
    axi_stream.slave        s_axis_net,
    axi_stream.master       m_axis_net,

    //RoCE Interface
    //DMA
    // let route=0 : ROUTE_DMA (not ROUTE_CUSTOM)
    axis_meta.rmaster       m_axis_roce_read_cmd,
    axis_meta.rmaster       m_axis_roce_write_cmd,
    axi_stream.slave        s_axis_roce_read_data,
    axi_stream.rmaster      m_axis_roce_write_data,

    // Control Signals
    input  wire                                  ap_start         ,
    output wire                                  ap_idle          ,
    output wire                                  ap_done          ,
    output wire                                  ap_ready         ,
    input  wire [32-1:0]                         rPSN             ,
    input  wire [32-1:0]                         lPSN             ,
    input  wire [32-1:0]                         rQPN             ,
    input  wire [32-1:0]                         lQPN             ,
    input  wire [32-1:0]                         rIP              ,
    input  wire [32-1:0]                         lIP              ,
    input  wire [32-1:0]                         rUDP             ,
    input  wire [32-1:0]                         vAddr            , //TODO: vAddr width 48 bits
    input  wire [32-1:0]                         rKey             ,
    input  wire [32-1:0]                         OP               ,
    input  wire [64-1:0]                         rAddr            ,
    input  wire [64-1:0]                         lAddr            ,
    input  wire [32-1:0]                         len              

    //Role Interface
    // // TODO: control by kernel
    // axis_meta.slave         s_axis_roce_role_tx_meta,
    // axi_stream.slave        s_axis_roce_role_tx_data

    
 );

///////////////////////////////////////////////////////////////////////////////
// Wires and Variables
///////////////////////////////////////////////////////////////////////////////
(* KEEP = "yes" *)
// ap logic
logic                                areset                         = 1'b0;
logic                                ap_start_r                     = 1'b0;
logic                                ap_idle_r                      = 1'b1;
logic                                ap_start_pulse                ;
logic [1:0]          ap_done_i                     ;
logic [1:0]          ap_done_r                      = 2'b0;

//  TODO: control by kernel
axi_stream #(.WIDTH(WIDTH))       s_axis_roce_role_tx_data();
assign s_axis_roce_role_tx_data.valid = 1'b0;

// IP Handler Input
axi_stream #(.WIDTH(WIDTH))     axis_slice_to_ibh();
// IP handler
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_arp_slice();
axi_stream #(.WIDTH(WIDTH))     axis_arp_slice_to_arp();
axi_stream #(.WIDTH(WIDTH))     axis_arp_to_arp_slice();
// ROCE
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_roce_slice();
axi_stream #(.WIDTH(WIDTH))     axis_roce_slice_to_roce();
axi_stream #(.WIDTH(WIDTH))     axis_roce_to_roce_slice();
axi_stream #(.WIDTH(WIDTH))     axis_roce_slice_to_mie();
// MAC merger Inputs
axi_stream #(.WIDTH(WIDTH))     axis_mie_to_intercon();
axi_stream #(.WIDTH(WIDTH))     axis_arp_slice_to_intercon();
// ARP lookup
wire        axis_arp_lookup_request_TVALID;
wire        axis_arp_lookup_request_TREADY;
wire[31:0]  axis_arp_lookup_request_TDATA;
wire        axis_arp_lookup_reply_TVALID;
wire        axis_arp_lookup_reply_TREADY;
wire[55:0]  axis_arp_lookup_reply_TDATA;

// host
// tx metadata
axis_meta #(.WIDTH(160))    axis_tx_metadata();
axis_meta #(.WIDTH(160))    axis_host_tx_metadata();

axis_meta #(.WIDTH(144))  axis_qp_interface();
axis_meta #(.WIDTH(184))  axis_qp_conn_interface();

wire        axis_host_arp_lookup_request_TVALID;
wire        axis_host_arp_lookup_request_TREADY;
wire[31:0]  axis_host_arp_lookup_request_TDATA;
wire        axis_host_arp_lookup_reply_TVALID;
wire        axis_host_arp_lookup_reply_TREADY;
wire[55:0]  axis_host_arp_lookup_reply_TDATA;
 
wire[31:0]    regCrcDropPkgCount;
wire          regCrcDropPkgCount_valid;
 
wire[31:0]    regInvalidPsnDropCount;
wire          regInvalidPsnDropCount_valid;


// DHCP Client IP address output //
wire[31:0]  dhcpAddressOut;

// Register and distribute ip address
wire[31:0]  dhcp_ip_address;
wire        dhcp_ip_address_en;
reg[47:0]   mie_mac_address;
reg[47:0]   arp_mac_address;
reg[31:0]   iph_ip_address;
reg[31:0]   arp_ip_address;
reg[31:0]   ip_subnet_mask;
reg[31:0]   ip_default_gateway;

wire set_ip_addr_valid;
reg[31:0] local_ip_address;
wire[31:0]ip_address_used;

wire set_board_number_valid;
wire[3:0] set_board_number_data;
reg[3:0] board_number;

// statistics
logic[31:0] rx_word_counter; 
logic[31:0] rx_pkg_counter; 
logic[31:0] tx_word_counter; 
logic[31:0] tx_pkg_counter;

logic[31:0] roce_rx_pkg_counter;
logic[31:0] roce_tx_pkg_counter;

logic[31:0] roce_data_rx_word_counter;
logic[31:0] roce_data_rx_pkg_counter;
logic[31:0] roce_data_tx_role_word_counter;
logic[31:0] roce_data_tx_role_pkg_counter;
logic[31:0] roce_data_tx_host_word_counter;
logic[31:0] roce_data_tx_host_pkg_counter;

logic[31:0] arp_rx_pkg_counter;
logic[31:0] arp_tx_pkg_counter;

logic[15:0] arp_request_pkg_counter;
logic[15:0] arp_reply_pkg_counter;

reg[7:0]  axis_stream_down_counter;
reg axis_stream_down;
reg[7:0]  output_stream_down_counter;
reg output_stream_down;

////////////////////////
// logic
/////////////////////
/*
 * ap logic
 */

// Register and invert reset signal.
always @(posedge net_clk) begin
  areset <= ~net_aresetn;
end

// create pulse when ap_start transitions to 1
always @(posedge net_clk) begin
  begin
    ap_start_r <= ap_start;
  end
end

assign ap_start_pulse = ap_start & ~ap_start_r;

// ap_idle is asserted when done is asserted, it is de-asserted when ap_start_pulse
// is asserted
always @(posedge net_clk) begin
  if (areset) begin
    ap_idle_r <= 1'b1;
  end
  else begin
    ap_idle_r <= ap_done ? 1'b1 :
      ap_start_pulse ? 1'b0 : ap_idle;
  end
end

assign ap_idle = ap_idle_r;

// Done logic
always @(posedge net_clk) begin
  if (areset) begin
    ap_done_r <= '0;
  end
  else begin
    ap_done_r <= (ap_done) ? '0 : ap_done_r | ap_done_i;
  end
end

assign ap_done = &ap_done_r;

// Ready Logic (non-pipelined case)
assign ap_ready = ap_done;


/*
 * Set IP address
 */


assign set_ip_addr_valid = ap_start_pulse;
assign set_board_number_valid = ap_start_pulse;
assign set_board_number_data = 0;

always @(posedge net_clk) begin
    if (~net_aresetn) begin
        local_ip_address <= 32'hD1D4010B;
        board_number <= 0;
    end
    else begin
        if (set_ip_addr_valid) begin
            local_ip_address[7:0] <= lIP[31:24];
            local_ip_address[15:8] <= lIP[23:16];
            local_ip_address[23:16] <= lIP[15:8];
            local_ip_address[31:24] <= lIP[7:0];
        end
        if (set_board_number_valid) begin
            board_number <= set_board_number_data;
        end
    end
end

//assign dhcp_ip_address_en = 1'b1;
//assign dhcp_ip_address = 32'hD1D4010A;

always @(posedge net_clk)
begin
    if (net_aresetn == 0) begin
        mie_mac_address <= 48'h000000000000;
        arp_mac_address <= 48'h000000000000;
        iph_ip_address <= 32'h00000000;
        arp_ip_address <= 32'h00000000;
        ip_subnet_mask <= 32'h00000000;
        ip_default_gateway <= 32'h00000000;
    end
    else begin
        mie_mac_address <= {MAC_ADDRESS[47:44], (MAC_ADDRESS[43:40]+board_number), MAC_ADDRESS[39:0]};
        arp_mac_address <= {MAC_ADDRESS[47:44], (MAC_ADDRESS[43:40]+board_number), MAC_ADDRESS[39:0]};
        if (DHCP_EN == 1) begin
            if (dhcp_ip_address_en == 1'b1) begin
                iph_ip_address <= dhcp_ip_address;
                arp_ip_address <= dhcp_ip_address;
            end
        end
        else begin
            iph_ip_address <= local_ip_address;
            arp_ip_address <= local_ip_address;
            ip_subnet_mask <= IP_SUBNET_MASK;
            ip_default_gateway <= {local_ip_address[31:28], 8'h01, local_ip_address[23:0]};
        end
    end
end
// ip address output
assign ip_address_used = iph_ip_address;


/*
 * qp interface 
 * conn interface
 */
logic qp_valid          = 1'b0;
logic qp_valid_r        = 1'b0;
logic qp_written        = 1'b0;
logic conn_valid        = 1'b0;
logic conn_valid_r      = 1'b0;
logic conn_written      = 1'b0;
logic host_meta_valid   = 1'b0;
logic host_meta_valid_r = 1'b0;
logic host_meta_written = 1'b0;
// create valid pulse
always @(posedge net_clk) begin
    if (~net_aresetn) begin
        qp_valid_r          <= 1'b0;
        conn_valid_r        <= 1'b0;
        host_meta_valid_r   <= 1'b0;
    end
    else begin
        qp_valid_r          <= qp_valid;
        conn_valid_r        <= conn_valid;
        host_meta_valid_r   <= host_meta_valid;
    end
end
assign axis_qp_interface.valid = qp_valid & ~qp_valid_r;
assign axis_qp_conn_interface.valid = conn_valid & ~conn_valid_r;
assign axis_host_tx_metadata.valid = host_meta_valid & ~host_meta_valid_r;

// write qp_interface, qp_conn_interface and tx_meta from host sw
always @(posedge net_clk)
begin
    if (~net_aresetn) begin
        qp_valid                    <= 1'b0;
        qp_written                  <= 1'b0;
        axis_qp_interface.data      <= 0;
        conn_valid                  <= 1'b0;
        conn_written                <= 1'b0;
        axis_qp_conn_interface.data <= 0;
        host_meta_valid             <= 1'b0;
        host_meta_written           <= 1'b0;
        axis_host_tx_metadata.data  <= 0;
    end
    else begin
        if (ap_start && axis_qp_interface.ready && ~qp_written) begin
            axis_qp_interface.data[2:0]     <= 3'b010; // 2 READY_RECV
            axis_qp_interface.data[26:3]    <= rQPN[23:0];
            axis_qp_interface.data[50:27]   <= rPSN[23:0];
            axis_qp_interface.data[74:51]   <= lPSN[23:0];
            axis_qp_interface.data[90:75]   <= rKey[15:0];
            axis_qp_interface.data[122:91]  <= vAddr[31:0]; //uint<48> vAddr
            axis_qp_interface.data[138:123] <= 0;
            qp_valid                        <= 1'b1;
            qp_written                      <= 1'b1;
        end
        if (ap_start && axis_qp_conn_interface.ready && ~conn_written) begin
            axis_qp_conn_interface.data[15:0]       <= lQPN[15:0];
            axis_qp_conn_interface.data[39:16]      <= rQPN[23:0];
            axis_qp_conn_interface.data[135:40]     <= 0;
            axis_qp_conn_interface.data[167:136]    <= rIP[31:0];
            axis_qp_conn_interface.data[183:168]    <= rUDP[15:0];
            conn_valid                              <= 1'b1;
            conn_written                            <= 1'b1;
        end
        if (ap_start && axis_host_tx_metadata.ready && ~host_meta_written) begin
            axis_host_tx_metadata.data[2:0]     <= OP[2:0];
            axis_host_tx_metadata.data[6:3]     <= lQPN[3:0]; //TODO: check local/remote qpn
            axis_host_tx_metadata.data[26:7]    <= 0; // only use 4 LSB
            axis_host_tx_metadata.data[74:27]   <= lAddr[47:0];
            axis_host_tx_metadata.data[122:75]  <= rAddr[47:0];
            axis_host_tx_metadata.data[154:123] <= len[31:0];
            host_meta_valid     <= 1'b1;
            host_meta_written   <= 1'b1;
        end
    end
end




/*
 * RoCEv2
 */


roce_stack #(
    .ROCE_EN(ROCE_EN),
    .WIDTH(WIDTH)
) rocev2_stack_inst(
    .net_clk(net_clk), // input aclk
    .net_aresetn(net_aresetn), // input aresetn
    //RX
    .s_axis_rx_data(axis_roce_slice_to_roce),
    //TX
    .s_axis_tx_meta(axis_tx_metadata),
    .s_axis_tx_data(s_axis_roce_role_tx_data), //TODO: role tx data

`ifndef ENABLE_DROP 
    .m_axis_tx_data(axis_roce_to_roce_slice),
`else
    .m_axis_tx_data(roce_2_drop),
`endif
    //Memory
    .m_axis_mem_write_cmd(m_axis_roce_write_cmd),
    .m_axis_mem_read_cmd(m_axis_roce_read_cmd),
    .m_axis_mem_write_data(m_axis_roce_write_data),
    .s_axis_mem_read_data(s_axis_roce_read_data),

    //Pointer chaising
// `ifdef POINTER_CHASING
//     .m_axis_rx_pcmeta(m_axis_rx_pcmeta),
//     .s_axis_tx_pcmeta(s_axis_tx_pcmeta),
// `endif
    //CONTROL
    .s_axis_qp_interface(axis_qp_interface),
    .s_axis_qp_conn_interface(axis_qp_conn_interface),
        //.local_ip_address_V(link_local_ipv6_address), // Use IPv6 addr
    .local_ip_address({iph_ip_address, 96'b0}), //Use IPv4 addr (still 128 bits)
    .crc_drop_pkg_count_valid(regCrcDropPkgCount_valid),
    .crc_drop_pkg_count_data(regCrcDropPkgCount),
    .psn_drop_pkg_count_valid(regInvalidPsnDropCount_valid),
    .psn_drop_pkg_count_data(regInvalidPsnDropCount)
);



axis_register_slice_512 axis_register_AXI_S (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(s_axis_net.valid),  // input wire s_axis_tvalid
  .s_axis_tready(s_axis_net.ready),  // output wire s_axis_tready
  .s_axis_tdata(s_axis_net.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(s_axis_net.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(s_axis_net.last),    // input wire s_axis_tlast
  .m_axis_tvalid(axis_slice_to_ibh.valid),  // output wire m_axis_tvalid
  .m_axis_tready(axis_slice_to_ibh.ready),  // input wire m_axis_tready
  .m_axis_tdata(axis_slice_to_ibh.data),    // output wire [63 : 0] m_axis_tdata
  .m_axis_tkeep(axis_slice_to_ibh.keep),    // output wire [7 : 0] m_axis_tkeep
  .m_axis_tlast(axis_slice_to_ibh.last)    // output wire m_axis_tlast
);
 
ip_handler_ip ip_handler_inst (
.m_axis_arp_TVALID(axis_iph_to_arp_slice.valid), // output AXI4Stream_M_TVALID
.m_axis_arp_TREADY(axis_iph_to_arp_slice.ready), // input AXI4Stream_M_TREADY
.m_axis_arp_TDATA(axis_iph_to_arp_slice.data), // output [63 : 0] AXI4Stream_M_TDATA
.m_axis_arp_TKEEP(axis_iph_to_arp_slice.keep), // output [7 : 0] AXI4Stream_M_TSTRB
.m_axis_arp_TLAST(axis_iph_to_arp_slice.last), // output [0 : 0] AXI4Stream_M_TLAST

.m_axis_icmp_TVALID(), // output AXI4Stream_M_TVALID
.m_axis_icmp_TREADY(), // input AXI4Stream_M_TREADY
.m_axis_icmp_TDATA(), // output [63 : 0] AXI4Stream_M_TDATA
.m_axis_icmp_TKEEP(), // output [7 : 0] AXI4Stream_M_TSTRB
.m_axis_icmp_TLAST(), // output [0 : 0] AXI4Stream_M_TLAST

.m_axis_icmpv6_TVALID(),
.m_axis_icmpv6_TREADY(),
.m_axis_icmpv6_TDATA(),
.m_axis_icmpv6_TKEEP(),
.m_axis_icmpv6_TLAST(),

.m_axis_ipv6udp_TVALID(),
.m_axis_ipv6udp_TREADY(),
.m_axis_ipv6udp_TDATA(), 
.m_axis_ipv6udp_TKEEP(),
.m_axis_ipv6udp_TLAST(),

.m_axis_udp_TVALID(),
.m_axis_udp_TREADY(),
.m_axis_udp_TDATA(),
.m_axis_udp_TKEEP(),
.m_axis_udp_TLAST(),

.m_axis_tcp_TVALID(),
.m_axis_tcp_TREADY(),
.m_axis_tcp_TDATA(),
.m_axis_tcp_TKEEP(),
.m_axis_tcp_TLAST(),

.m_axis_roce_TVALID(axis_iph_to_roce_slice.valid),
.m_axis_roce_TREADY(axis_iph_to_roce_slice.ready),
.m_axis_roce_TDATA(axis_iph_to_roce_slice.data),
.m_axis_roce_TKEEP(axis_iph_to_roce_slice.keep),
.m_axis_roce_TLAST(axis_iph_to_roce_slice.last),

.s_axis_raw_TVALID(axis_slice_to_ibh.valid),
.s_axis_raw_TREADY(axis_slice_to_ibh.ready),
.s_axis_raw_TDATA(axis_slice_to_ibh.data),
.s_axis_raw_TKEEP(axis_slice_to_ibh.keep),
.s_axis_raw_TLAST(axis_slice_to_ibh.last),

.myIpAddress_V(iph_ip_address),

.ap_clk(net_clk), // input aclk
.ap_rst_n(net_aresetn) // input aresetn
);


mac_ip_encode_ip mac_ip_encode_inst (
.m_axis_ip_TVALID(axis_mie_to_intercon.valid),
.m_axis_ip_TREADY(axis_mie_to_intercon.ready),
.m_axis_ip_TDATA(axis_mie_to_intercon.data),
.m_axis_ip_TKEEP(axis_mie_to_intercon.keep),
.m_axis_ip_TLAST(axis_mie_to_intercon.last),
.m_axis_arp_lookup_request_V_V_TVALID(axis_arp_lookup_request_TVALID),
.m_axis_arp_lookup_request_V_V_TREADY(axis_arp_lookup_request_TREADY),
.m_axis_arp_lookup_request_V_V_TDATA(axis_arp_lookup_request_TDATA),
.s_axis_ip_TVALID(axis_roce_slice_to_mie.valid),
.s_axis_ip_TREADY(axis_roce_slice_to_mie.ready),
.s_axis_ip_TDATA(axis_roce_slice_to_mie.data),
.s_axis_ip_TKEEP(axis_roce_slice_to_mie.keep),
.s_axis_ip_TLAST(axis_roce_slice_to_mie.last),
.s_axis_arp_lookup_reply_V_TVALID(axis_arp_lookup_reply_TVALID),
.s_axis_arp_lookup_reply_V_TREADY(axis_arp_lookup_reply_TREADY),
.s_axis_arp_lookup_reply_V_TDATA(axis_arp_lookup_reply_TDATA),

.myMacAddress_V(mie_mac_address),                                    // input wire [47 : 0] regMacAddress_V
.regSubNetMask_V(ip_subnet_mask),                                    // input wire [31 : 0] regSubNetMask_V
.regDefaultGateway_V(ip_default_gateway),                            // input wire [31 : 0] regDefaultGateway_V
  
.ap_clk(net_clk), // input aclk
.ap_rst_n(net_aresetn) // input aresetn
);


// merges ip and arp
axis_interconnect_512_2to1 mac_merger (
  .ACLK(net_clk), // input ACLK
  .ARESETN(net_aresetn), // input ARESETN
  .S00_AXIS_ACLK(net_clk), // input S00_AXIS_ACLK
  .S01_AXIS_ACLK(net_clk), // input S01_AXIS_ACLK
  //.S02_AXIS_ACLK(net_clk), // input S01_AXIS_ACLK
  .S00_AXIS_ARESETN(net_aresetn), // input S00_AXIS_ARESETN
  .S01_AXIS_ARESETN(net_aresetn), // input S01_AXIS_ARESETN
  //.S02_AXIS_ARESETN(net_aresetn), // input S01_AXIS_ARESETN
  .S00_AXIS_TVALID(axis_arp_slice_to_intercon.valid), // input S00_AXIS_TVALID
  .S00_AXIS_TREADY(axis_arp_slice_to_intercon.ready), // output S00_AXIS_TREADY
  .S00_AXIS_TDATA(axis_arp_slice_to_intercon.data), // input [63 : 0] S00_AXIS_TDATA
  .S00_AXIS_TKEEP(axis_arp_slice_to_intercon.keep), // input [7 : 0] S00_AXIS_TKEEP
  .S00_AXIS_TLAST(axis_arp_slice_to_intercon.last), // input S00_AXIS_TLAST
  
  .S01_AXIS_TVALID(axis_mie_to_intercon.valid), // input S01_AXIS_TVALID
  .S01_AXIS_TREADY(axis_mie_to_intercon.ready), // output S01_AXIS_TREADY
  .S01_AXIS_TDATA(axis_mie_to_intercon.data), // input [63 : 0] S01_AXIS_TDATA
  .S01_AXIS_TKEEP(axis_mie_to_intercon.keep), // input [7 : 0] S01_AXIS_TKEEP
  .S01_AXIS_TLAST(axis_mie_to_intercon.last), // input S01_AXIS_TLAST
  
  .M00_AXIS_ACLK(net_clk), // input M00_AXIS_ACLK
  .M00_AXIS_ARESETN(net_aresetn), // input M00_AXIS_ARESETN
  .M00_AXIS_TVALID(m_axis_net.valid), // output M00_AXIS_TVALID
  .M00_AXIS_TREADY(m_axis_net.ready), // input M00_AXIS_TREADY
  .M00_AXIS_TDATA(m_axis_net.data), // output [63 : 0] M00_AXIS_TDATA
  .M00_AXIS_TKEEP(m_axis_net.keep), // output [7 : 0] M00_AXIS_TKEEP
  .M00_AXIS_TLAST(m_axis_net.last), // output M00_AXIS_TLAST
  .S00_ARB_REQ_SUPPRESS(1'b0), // input S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0) // input S01_ARB_REQ_SUPPRESS
  //.S02_ARB_REQ_SUPPRESS(1'b0) // input S01_ARB_REQ_SUPPRESS
);


arp_server_subnet_ip arp_server_inst(
.m_axis_TVALID(axis_arp_to_arp_slice.valid),
.m_axis_TREADY(axis_arp_to_arp_slice.ready),
.m_axis_TDATA(axis_arp_to_arp_slice.data),
.m_axis_TKEEP(axis_arp_to_arp_slice.keep),
.m_axis_TLAST(axis_arp_to_arp_slice.last),
.m_axis_arp_lookup_reply_V_TVALID(axis_arp_lookup_reply_TVALID),
.m_axis_arp_lookup_reply_V_TREADY(axis_arp_lookup_reply_TREADY),
.m_axis_arp_lookup_reply_V_TDATA(axis_arp_lookup_reply_TDATA),
.m_axis_host_arp_lookup_reply_V_TVALID(axis_host_arp_lookup_reply_TVALID),
.m_axis_host_arp_lookup_reply_V_TREADY(axis_host_arp_lookup_reply_TREADY),
.m_axis_host_arp_lookup_reply_V_TDATA(axis_host_arp_lookup_reply_TDATA),
.s_axis_TVALID(axis_arp_slice_to_arp.valid),
.s_axis_TREADY(axis_arp_slice_to_arp.ready),
.s_axis_TDATA(axis_arp_slice_to_arp.data),
.s_axis_TKEEP(axis_arp_slice_to_arp.keep),
.s_axis_TLAST(axis_arp_slice_to_arp.last),
.s_axis_arp_lookup_request_V_V_TVALID(axis_arp_lookup_request_TVALID),
.s_axis_arp_lookup_request_V_V_TREADY(axis_arp_lookup_request_TREADY),
.s_axis_arp_lookup_request_V_V_TDATA(axis_arp_lookup_request_TDATA),
.s_axis_host_arp_lookup_request_V_V_TVALID(axis_host_arp_lookup_request_TVALID),
.s_axis_host_arp_lookup_request_V_V_TREADY(axis_host_arp_lookup_request_TREADY),
.s_axis_host_arp_lookup_request_V_V_TDATA(axis_host_arp_lookup_request_TDATA),

.myMacAddress_V(arp_mac_address),
.myIpAddress_V(arp_ip_address),
.regRequestCount_V(arp_request_pkg_counter),
.regRequestCount_V_ap_vld(),
.regReplyCount_V(arp_reply_pkg_counter),
.regReplyCount_V_ap_vld(),

.ap_clk(net_clk), // input aclk
.ap_rst_n(net_aresetn) // input aresetn
);



/*
 * Slices
 */
 // ARP Input Slice
register_slice_wrapper #(.WIDTH(WIDTH)) axis_register_arp_in_slice(
 .aclk(net_clk),
 .aresetn(net_aresetn),
 .s_axis(axis_iph_to_arp_slice),
 .m_axis(axis_arp_slice_to_arp)
);
 // ARP Output Slice
register_slice_wrapper #(.WIDTH(WIDTH)) axis_register_arp_out_slice(
 .aclk(net_clk),
 .aresetn(net_aresetn),
 .s_axis(axis_arp_to_arp_slice),
 .m_axis(axis_arp_slice_to_intercon)
);
 // ROCE Input Slice
register_slice_wrapper #(.WIDTH(WIDTH)) axis_register_roce_in_slice(
.aclk(net_clk),
.aresetn(net_aresetn),
.s_axis(axis_iph_to_roce_slice),
.m_axis(axis_roce_slice_to_roce)
);
// ROCE Output Slice
register_slice_wrapper #(.WIDTH(WIDTH)) axis_register_roce_out_slice(
.aclk(net_clk),
.aresetn(net_aresetn),
.s_axis(axis_roce_to_roce_slice),
.m_axis(axis_roce_slice_to_mie)
);


axis_interconnect_merger_160 tx_metadata_merger (
  .ACLK(net_clk),                                  // input wire ACLK
  .ARESETN(net_aresetn),                            // input wire ARESETN
  .S00_AXIS_ACLK(net_clk),                // input wire S00_AXIS_ACLK
  .S00_AXIS_ARESETN(net_aresetn),          // input wire S00_AXIS_ARESETN
  .S00_AXIS_TVALID(axis_host_tx_metadata.valid),            // input wire S00_AXIS_TVALID
  .S00_AXIS_TREADY(axis_host_tx_metadata.ready),            // output wire S00_AXIS_TREADY
  .S00_AXIS_TDATA(axis_host_tx_metadata.data),              // input wire [159 : 0] S00_AXIS_TDATA
  .S01_AXIS_ACLK(net_clk),                // input wire S01_AXIS_ACLK
  .S01_AXIS_ARESETN(net_aresetn),          // input wire S01_AXIS_ARESETN
  //TODO: role tx meta
  .S01_AXIS_TVALID(0),//s_axis_roce_role_tx_meta.valid),            // input wire S01_AXIS_TVALID
  .S01_AXIS_TREADY(),//s_axis_roce_role_tx_meta.ready),            // output wire S01_AXIS_TREADY
  .S01_AXIS_TDATA(),//s_axis_roce_role_tx_meta.data),              // input wire [159 : 0] S01_AXIS_TDATA
  .M00_AXIS_ACLK(net_clk),                // input wire M00_AXIS_ACLK
  .M00_AXIS_ARESETN(net_aresetn),          // input wire M00_AXIS_ARESETN
  .M00_AXIS_TVALID(axis_tx_metadata.valid),            // output wire M00_AXIS_TVALID
  .M00_AXIS_TREADY(axis_tx_metadata.ready),            // input wire M00_AXIS_TREADY
  .M00_AXIS_TDATA(axis_tx_metadata.data),              // output wire [159 : 0] M00_AXIS_TDATA
  .S00_ARB_REQ_SUPPRESS(1'b0),  // input wire S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0)  // input wire S01_ARB_REQ_SUPPRESS
);


/*
 * Statistics
 */

always @(posedge net_clk) begin
    if (~net_aresetn) begin
        rx_word_counter <= '0;
        rx_pkg_counter <= '0;
        tx_word_counter <= '0;
        tx_pkg_counter <= '0;

        roce_data_rx_word_counter <= '0;
        roce_data_rx_pkg_counter <= '0;
        roce_data_tx_role_word_counter <= '0;
        roce_data_tx_role_pkg_counter <= '0;
        roce_data_tx_host_word_counter <= '0;
        roce_data_tx_host_pkg_counter <= '0;
        
        arp_rx_pkg_counter <= '0;
        arp_tx_pkg_counter <= '0;
        
        roce_rx_pkg_counter <= '0;
        roce_tx_pkg_counter <= '0;

        axis_stream_down_counter <= '0;
        axis_stream_down <= 1'b0;
    end
    else begin
        if (s_axis_net.ready) begin
            axis_stream_down_counter <= '0;
        end
        if (s_axis_net.valid && ~s_axis_net.ready) begin
            axis_stream_down_counter <= axis_stream_down_counter + 1;
        end
        if (axis_stream_down_counter > 2) begin
            axis_stream_down <= 1'b1;
        end
        if (s_axis_net.valid && s_axis_net.ready) begin
            rx_word_counter <= rx_word_counter + 1;
            if (s_axis_net.last) begin
                rx_pkg_counter <= rx_pkg_counter + 1;
            end
        end
        if (m_axis_net.valid && m_axis_net.ready) begin
            tx_word_counter <= tx_word_counter + 1;
            if (m_axis_net.last) begin
                tx_pkg_counter <= tx_pkg_counter + 1;
            end
        end
        //arp
        if (axis_arp_slice_to_arp.valid && axis_arp_slice_to_arp.ready) begin
            if (axis_arp_slice_to_arp.last) begin
                arp_rx_pkg_counter <= arp_rx_pkg_counter + 1;
            end
        end
        if (axis_arp_to_arp_slice.valid && axis_arp_to_arp_slice.ready) begin
            if (axis_arp_to_arp_slice.last) begin
                arp_tx_pkg_counter <= arp_tx_pkg_counter + 1;
            end
        end
        //roce
        if (axis_roce_slice_to_roce.valid && axis_roce_slice_to_roce.ready) begin
            if (axis_roce_slice_to_roce.last) begin
                roce_rx_pkg_counter <= roce_rx_pkg_counter + 1;
            end
        end
        if (axis_roce_to_roce_slice.valid && axis_roce_to_roce_slice.ready) begin
            if (axis_roce_to_roce_slice.last) begin
                roce_tx_pkg_counter <= roce_tx_pkg_counter + 1;
            end
        end
        //roce data
        if (m_axis_roce_write_data.valid && m_axis_roce_write_data.ready) begin
            roce_data_rx_word_counter <= roce_data_rx_word_counter + 1;
            if (m_axis_roce_write_data.last) begin
                roce_data_rx_pkg_counter <= roce_data_rx_pkg_counter + 1;
            end
        end
        if (s_axis_roce_read_data.valid && s_axis_roce_read_data.ready) begin
            roce_data_tx_host_word_counter <= roce_data_tx_host_word_counter + 1;
            if (s_axis_roce_read_data.last) begin
                roce_data_tx_host_pkg_counter <= roce_data_tx_host_pkg_counter + 1;
            end
        end
        if (s_axis_roce_role_tx_data.valid && s_axis_roce_role_tx_data.ready) begin
            roce_data_tx_role_word_counter <= roce_data_tx_role_word_counter + 1;
            if (s_axis_roce_role_tx_data.last) begin
                roce_data_tx_role_pkg_counter <= roce_data_tx_role_pkg_counter + 1;
            end
        end
    end
end

endmodule

`default_nettype wire
