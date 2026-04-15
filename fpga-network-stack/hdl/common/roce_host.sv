// This is a generated file. Use and modify at your own risk.
//////////////////////////////////////////////////////////////////////////////// 
// default_nettype of none prevents implicit wire declaration.
`default_nettype none
`timescale 1 ns / 1 ps

`include "davos_types.svh"

// Top level of the kernel. Do not modify module name, parameters or ports.
module roce_host #(
  parameter integer C_S_AXI_CONTROL_ADDR_WIDTH  = 12 ,
  parameter integer C_S_AXI_CONTROL_DATA_WIDTH  = 32 ,
  parameter integer C_M00_AXI_ADDR_WIDTH        = 64 ,
  parameter integer C_M00_AXI_DATA_WIDTH        = 512,
  parameter integer C_S_AXIS_NET_RX_TDATA_WIDTH = 512,
  parameter integer C_M_AXIS_NET_TX_TDATA_WIDTH = 512
)
(
  // System Signals
  input  wire                                     ap_clk               ,
  input  wire                                     ap_rst_n             ,
  //  Note: A minimum subset of AXI4 memory mapped signals are declared.  AXI
  // signals omitted from these interfaces are automatically inferred with the
  // optimal values for Xilinx accleration platforms.  This allows Xilinx AXI4 Interconnects
  // within the system to be optimized by removing logic for AXI4 protocol
  // features that are not necessary. When adapting AXI4 masters within the RTL
  // kernel that have signals not declared below, it is suitable to add the
  // signals to the declarations below to connect them to the AXI4 Master.
  // 
  // List of ommited signals - effect
  // -------------------------------
  // ID - Transaction ID are used for multithreading and out of order
  // transactions.  This increases complexity. This saves logic and increases Fmax
  // in the system when ommited.
  // SIZE - Default value is log2(data width in bytes). Needed for subsize bursts.
  // This saves logic and increases Fmax in the system when ommited.
  // BURST - Default value (0b01) is incremental.  Wrap and fixed bursts are not
  // recommended. This saves logic and increases Fmax in the system when ommited.
  // LOCK - Not supported in AXI4
  // CACHE - Default value (0b0011) allows modifiable transactions. No benefit to
  // changing this.
  // PROT - Has no effect in current acceleration platforms.
  // QOS - Has no effect in current acceleration platforms.
  // REGION - Has no effect in current acceleration platforms.
  // USER - Has no effect in current acceleration platforms.
  // RESP - Not useful in most acceleration platforms.
  // 
  // AXI4 master interface m00_axi
  output wire                                     m00_axi_awvalid      ,
  input  wire                                     m00_axi_awready      ,
  output wire [C_M00_AXI_ADDR_WIDTH-1:0]          m00_axi_awaddr       ,
  output wire [8-1:0]                             m00_axi_awlen        ,
  output wire                                     m00_axi_wvalid       ,
  input  wire                                     m00_axi_wready       ,
  output wire [C_M00_AXI_DATA_WIDTH-1:0]          m00_axi_wdata        ,
  output wire [C_M00_AXI_DATA_WIDTH/8-1:0]        m00_axi_wstrb        ,
  output wire                                     m00_axi_wlast        ,
  input  wire                                     m00_axi_bvalid       ,
  output wire                                     m00_axi_bready       ,
  output wire                                     m00_axi_arvalid      ,
  input  wire                                     m00_axi_arready      ,
  output wire [C_M00_AXI_ADDR_WIDTH-1:0]          m00_axi_araddr       ,
  output wire [8-1:0]                             m00_axi_arlen        ,
  input  wire                                     m00_axi_rvalid       ,
  output wire                                     m00_axi_rready       ,
  input  wire [C_M00_AXI_DATA_WIDTH-1:0]          m00_axi_rdata        ,
  input  wire                                     m00_axi_rlast        ,
  // AXI4-Stream (slave) interface s_axis_net_rx
  input  wire                                     s_axis_net_rx_tvalid ,
  output wire                                     s_axis_net_rx_tready ,
  input  wire [C_S_AXIS_NET_RX_TDATA_WIDTH-1:0]   s_axis_net_rx_tdata  ,
  input  wire [C_S_AXIS_NET_RX_TDATA_WIDTH/8-1:0] s_axis_net_rx_tkeep  ,
  input  wire                                     s_axis_net_rx_tlast  ,
  // AXI4-Stream (master) interface m_axis_net_tx
  output wire                                     m_axis_net_tx_tvalid ,
  input  wire                                     m_axis_net_tx_tready ,
  output wire [C_M_AXIS_NET_TX_TDATA_WIDTH-1:0]   m_axis_net_tx_tdata  ,
  output wire [C_M_AXIS_NET_TX_TDATA_WIDTH/8-1:0] m_axis_net_tx_tkeep  ,
  output wire                                     m_axis_net_tx_tlast  ,
  // AXI4-Lite slave interface
  input  wire                                     s_axi_control_awvalid,
  output wire                                     s_axi_control_awready,
  input  wire [C_S_AXI_CONTROL_ADDR_WIDTH-1:0]    s_axi_control_awaddr ,
  input  wire                                     s_axi_control_wvalid ,
  output wire                                     s_axi_control_wready ,
  input  wire [C_S_AXI_CONTROL_DATA_WIDTH-1:0]    s_axi_control_wdata  ,
  input  wire [C_S_AXI_CONTROL_DATA_WIDTH/8-1:0]  s_axi_control_wstrb  ,
  input  wire                                     s_axi_control_arvalid,
  output wire                                     s_axi_control_arready,
  input  wire [C_S_AXI_CONTROL_ADDR_WIDTH-1:0]    s_axi_control_araddr ,
  output wire                                     s_axi_control_rvalid ,
  input  wire                                     s_axi_control_rready ,
  output wire [C_S_AXI_CONTROL_DATA_WIDTH-1:0]    s_axi_control_rdata  ,
  output wire [2-1:0]                             s_axi_control_rresp  ,
  output wire                                     s_axi_control_bvalid ,
  input  wire                                     s_axi_control_bready ,
  output wire [2-1:0]                             s_axi_control_bresp  ,
  output wire                                     interrupt            
);

///////////////////////////////////////////////////////////////////////////////
// Local Parameters
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Wires and Variables
///////////////////////////////////////////////////////////////////////////////
(* DONT_TOUCH = "yes" *)
reg                                 areset                         = 1'b0;
wire                                ap_start                      ;
wire                                ap_idle                       ;
wire                                ap_done                       ;
wire                                ap_ready                      ;
wire [32-1:0]                       rPSN                          ;
wire [32-1:0]                       lPSN                          ;
wire [32-1:0]                       rQPN                          ;
wire [32-1:0]                       lQPN                          ;
wire [32-1:0]                       rIP                           ;
wire [32-1:0]                       lIP                           ;
wire [32-1:0]                       rUDP                          ;
wire [64-1:0]                       vAddr                         ;
wire [32-1:0]                       rKey                          ;
wire [32-1:0]                       OP                            ;
wire [64-1:0]                       rAddr                         ;
wire [64-1:0]                       lAddr                         ;
wire [32-1:0]                       len                           ;
wire [64-1:0]                       mem_ptr                       ;

// cmac interface
axi_stream axis_net_rx_data_aclk();
axi_stream axis_net_tx_data_aclk();
//TCP/IP DDR interface
axis_mem_cmd    axis_tcp_mem_read_cmd();
axi_stream      axis_tcp_mem_read_data();
axis_mem_status axis_tcp_mem_read_status(); 
axis_mem_cmd    axis_tcp_mem_write_cmd();
axi_stream      axis_tcp_mem_write_data();
axis_mem_status axis_tcp_mem_write_status();
// ROCE memory interface
axis_meta       m_axis_roce_read_cmd();
axis_meta       m_axis_roce_write_cmd();
// convert axis_meta to axis_mem_cmd: leave out route
axis_mem_cmd    axis_roce_read_cmd();
axis_mem_cmd    axis_roce_write_cmd();
axi_stream      s_axis_roce_read_data();
axi_stream      m_axis_roce_write_data();
axis_mem_status axis_roce_read_status();
axis_mem_status axis_roce_write_status();
assign axis_roce_read_status.ready = 1'b1;
assign axis_roce_write_status.ready = 1'b1;

// Register and invert reset signal.
always @(posedge ap_clk) begin
  areset <= ~ap_rst_n;
end


///////////////////////////////////////////////////////////////////////////////
// Begin control interface RTL.  Modifying not recommended.
///////////////////////////////////////////////////////////////////////////////

// AXI4-Lite slave interface
roce_host_control_s_axi #(
  .C_S_AXI_ADDR_WIDTH ( C_S_AXI_CONTROL_ADDR_WIDTH ),
  .C_S_AXI_DATA_WIDTH ( C_S_AXI_CONTROL_DATA_WIDTH )
)
inst_control_s_axi (
  .ACLK      ( ap_clk                ),
  .ARESET    ( areset                ),
  .ACLK_EN   ( 1'b1                  ),
  .AWVALID   ( s_axi_control_awvalid ),
  .AWREADY   ( s_axi_control_awready ),
  .AWADDR    ( s_axi_control_awaddr  ),
  .WVALID    ( s_axi_control_wvalid  ),
  .WREADY    ( s_axi_control_wready  ),
  .WDATA     ( s_axi_control_wdata   ),
  .WSTRB     ( s_axi_control_wstrb   ),
  .ARVALID   ( s_axi_control_arvalid ),
  .ARREADY   ( s_axi_control_arready ),
  .ARADDR    ( s_axi_control_araddr  ),
  .RVALID    ( s_axi_control_rvalid  ),
  .RREADY    ( s_axi_control_rready  ),
  .RDATA     ( s_axi_control_rdata   ),
  .RRESP     ( s_axi_control_rresp   ),
  .BVALID    ( s_axi_control_bvalid  ),
  .BREADY    ( s_axi_control_bready  ),
  .BRESP     ( s_axi_control_bresp   ),
  .interrupt ( interrupt             ),
  .ap_start  ( ap_start              ),
  .ap_done   ( ap_done               ),
  .ap_ready  ( ap_ready              ),
  .ap_idle   ( ap_idle               ),
  .rPSN      ( rPSN                  ),
  .lPSN      ( lPSN                  ),
  .rQPN      ( rQPN                  ),
  .lQPN      ( lQPN                  ),
  .rIP       ( rIP                   ),
  .lIP       ( lIP                   ),
  .rUDP      ( rUDP                  ),
  .vAddr     ( vAddr                 ),
  .rKey      ( rKey                  ),
  .OP        ( OP                    ),
  .rAddr     ( rAddr                 ),
  .lAddr     ( lAddr                 ),
  .len       ( len                   ),
  .mem_ptr   ( mem_ptr               )
);

///////////////////////////////////////////////////////////////////////////////
// Add kernel logic here.  Modify/remove example code as necessary.
///////////////////////////////////////////////////////////////////////////////

// roce stack top logic.
stack_top #(
    .RX_DDR_BYPASS_EN   ( 0 ),
    .ROCE_EN            ( 1 )
) inst_stack_top (
    .net_clk                ( ap_clk                 ),
    .net_aresetn            ( ap_rst_n               ),
    // network interface streams
    .s_axis_net             ( axis_net_tx_data_aclk  ),
    .m_axis_net             ( axis_net_rx_data_aclk  ),
    //RoCE Interface
    //DMA
    // let route=0 : ROUTE_DMA (not ROUTE_CUSTOM)
    .m_axis_roce_read_cmd   ( m_axis_roce_read_cmd   ),
    .m_axis_roce_write_cmd  ( m_axis_roce_write_cmd  ),
    .s_axis_roce_read_data  ( s_axis_roce_read_data  ),
    .m_axis_roce_write_data ( m_axis_roce_write_data ),
    // Control Signals
    .ap_start               ( ap_start               ),
    .ap_idle                ( ap_done                ),
    .ap_done                ( ap_idle                ),
    .ap_ready               ( ap_ready               ),
    .rPSN                   ( rPSN                   ),
    .lPSN                   ( lPSN                   ),
    .rQPN                   ( rQPN                   ),
    .lQPN                   ( lQPN                   ),
    .rIP                    ( rIP                    ),
    .lIP                    ( lIP                    ),
    .rUDP                   ( rUDP                   ),
    .vAddr                  ( vAddr                  ), //TODO: vAddr width 48 bits
    .rKey                   ( rKey                   ),
    .OP                     ( OP                     ),
    .rAddr                  ( rAddr                  ),
    .lAddr                  ( lAddr                  ),
    .len                    ( len                    )
    //Role Interface
    // // TODO: control by kernel
    // axis_meta.slave         s_axis_roce_role_tx_meta,
    // axi_stream.slave        s_axis_roce_role_tx_data
);

// TODO: mem interface
mem_single_inf #(
    .ENABLE(1)
) mem_inf_inst0 (
.user_clk(ap_clk),
.user_aresetn(ap_rst_n),
.mem_clk(ap_clk),
.mem_aresetn(ap_rst_n),

/* USER INTERFACE */
//memory read commands
.s_axis_mem_read_cmd(axis_roce_read_cmd),
//memory read status
.m_axis_mem_read_status(axis_roce_read_status),
//memory read stream
.m_axis_mem_read_data(s_axis_roce_read_data),

//memory write commands
.s_axis_mem_write_cmd(axis_roce_write_cmd),
//memory rite status
.m_axis_mem_write_status(axis_roce_write_status),
//memory write stream
.s_axis_mem_write_data(m_axis_roce_write_data),

/* DRIVER INTERFACE */
.m_axi_awid(),
.m_axi_awaddr(m00_axi_awaddr),
.m_axi_awlen(m00_axi_awlen),
.m_axi_awsize(),
.m_axi_awburst(),
.m_axi_awlock(),
.m_axi_awcache(),
.m_axi_awprot(),
.m_axi_awvalid(m00_axi_awvalid),
.m_axi_awready(m00_axi_awready),

.m_axi_wdata(m00_axi_wdata),
.m_axi_wstrb(m00_axi_wstrb),
.m_axi_wlast(m00_axi_wlast),
.m_axi_wvalid(m00_axi_wvalid),
.m_axi_wready(m00_axi_wready),

.m_axi_bready(m00_axi_bready),
.m_axi_bid(),
.m_axi_bresp(),
.m_axi_bvalid(m00_axi_bvalid),

.m_axi_arid(),
.m_axi_araddr(m00_axi_araddr),
.m_axi_arlen(m00_axi_arlen),
.m_axi_arsize(),
.m_axi_arburst(),
.m_axi_arlock(),
.m_axi_arcache(),
.m_axi_arprot(),
.m_axi_arvalid(m00_axi_arvalid),
.m_axi_arready(m00_axi_arready),

.m_axi_rready(m00_axi_rready),
.m_axi_rid(),
.m_axi_rdata(m00_axi_rdata),
.m_axi_rresp(),
.m_axi_rlast(m00_axi_rlast),
.m_axi_rvalid(m00_axi_rvalid),

.addr_offset(mem_ptr)
);

///////////////////////////////////////////////////////////////////////////////
// Assign
///////////////////////////////////////////////////////////////////////////////

// assign tx & rx axis
assign m_axis_net_tx_tvalid = axis_net_tx_data_aclk.valid;
assign m_axis_net_tx_tdata = axis_net_tx_data_aclk.data;
assign m_axis_net_tx_tkeep = axis_net_tx_data_aclk.keep;
assign m_axis_net_tx_tlast = axis_net_tx_data_aclk.last;
assign axis_net_tx_data_aclk.ready = m_axis_net_tx_tready;

assign axis_net_rx_data_aclk.valid = s_axis_net_rx_tvalid;
assign axis_net_rx_data_aclk.data = s_axis_net_rx_tdata;
assign axis_net_rx_data_aclk.keep = s_axis_net_rx_tkeep;
assign axis_net_rx_data_aclk.last = s_axis_net_rx_tlast;
assign s_axis_net_rx_tready = axis_net_rx_data_aclk.ready;

// convert axis_meta to axis_mem_cmd: leave out route (dest)
assign axis_roce_read_cmd.valid = m_axis_roce_read_cmd.valid;
assign axis_roce_read_cmd.address = m_axis_roce_read_cmd.data[63:0];
assign axis_roce_read_cmd.length = m_axis_roce_read_cmd.data[95:64];
assign m_axis_roce_read_cmd.ready = axis_roce_write_cmd.ready;

assign axis_roce_write_cmd.valid = m_axis_roce_write_cmd.valid;
assign axis_roce_write_cmd.address = m_axis_roce_write_cmd.data[63:0];
assign axis_roce_write_cmd.length = m_axis_roce_write_cmd.data[95:64];
assign m_axis_roce_write_cmd.ready = axis_roce_write_cmd.ready;

// TODO: assign k2k streaming signals here

endmodule
`default_nettype wire
