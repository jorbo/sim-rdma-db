// ==============================================================
// Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC v2019.2.1 (64-bit)
// Copyright 1986-2019 Xilinx, Inc. All Rights Reserved.
// ==============================================================
`timescale 1ns/1ps
module roce_host_control_s_axi
#(parameter
    C_S_AXI_ADDR_WIDTH = 8,
    C_S_AXI_DATA_WIDTH = 32
)(
    input  wire                          ACLK,
    input  wire                          ARESET,
    input  wire                          ACLK_EN,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0] AWADDR,
    input  wire                          AWVALID,
    output wire                          AWREADY,
    input  wire [C_S_AXI_DATA_WIDTH-1:0] WDATA,
    input  wire [C_S_AXI_DATA_WIDTH/8-1:0] WSTRB,
    input  wire                          WVALID,
    output wire                          WREADY,
    output wire [1:0]                    BRESP,
    output wire                          BVALID,
    input  wire                          BREADY,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0] ARADDR,
    input  wire                          ARVALID,
    output wire                          ARREADY,
    output wire [C_S_AXI_DATA_WIDTH-1:0] RDATA,
    output wire [1:0]                    RRESP,
    output wire                          RVALID,
    input  wire                          RREADY,
    output wire                          interrupt,
    output wire                          ap_start,
    input  wire                          ap_done,
    input  wire                          ap_ready,
    input  wire                          ap_idle,
    output wire [31:0]                   rPSN,
    output wire [31:0]                   lPSN,
    output wire [31:0]                   rQPN,
    output wire [31:0]                   lQPN,
    output wire [31:0]                   rIP,
    output wire [31:0]                   lIP,
    output wire [31:0]                   rUDP,
    output wire [63:0]                   vAddr,
    output wire [31:0]                   rKey,
    output wire [31:0]                   OP,
    output wire [63:0]                   rAddr,
    output wire [63:0]                   lAddr,
    output wire [31:0]                   len,
    output wire [31:0]                   debug,
    output wire [63:0]                   mem_ptr
);
//------------------------Address Info-------------------
// 0x00 : Control signals
//        bit 0  - ap_start (Read/Write/COH)
//        bit 1  - ap_done (Read/COR)
//        bit 2  - ap_idle (Read)
//        bit 3  - ap_ready (Read)
//        bit 7  - auto_restart (Read/Write)
//        others - reserved
// 0x04 : Global Interrupt Enable Register
//        bit 0  - Global Interrupt Enable (Read/Write)
//        others - reserved
// 0x08 : IP Interrupt Enable Register (Read/Write)
//        bit 0  - Channel 0 (ap_done)
//        bit 1  - Channel 1 (ap_ready)
//        others - reserved
// 0x0c : IP Interrupt Status Register (Read/TOW)
//        bit 0  - Channel 0 (ap_done)
//        bit 1  - Channel 1 (ap_ready)
//        others - reserved
// 0x10 : Data signal of rPSN
//        bit 31~0 - rPSN[31:0] (Read/Write)
// 0x14 : reserved
// 0x18 : Data signal of lPSN
//        bit 31~0 - lPSN[31:0] (Read/Write)
// 0x1c : reserved
// 0x20 : Data signal of rQPN
//        bit 31~0 - rQPN[31:0] (Read/Write)
// 0x24 : reserved
// 0x28 : Data signal of lQPN
//        bit 31~0 - lQPN[31:0] (Read/Write)
// 0x2c : reserved
// 0x30 : Data signal of rIP
//        bit 31~0 - rIP[31:0] (Read/Write)
// 0x34 : reserved
// 0x38 : Data signal of lIP
//        bit 31~0 - lIP[31:0] (Read/Write)
// 0x3c : reserved
// 0x40 : Data signal of rUDP
//        bit 31~0 - rUDP[31:0] (Read/Write)
// 0x44 : reserved
// 0x48 : Data signal of vAddr
//        bit 31~0 - vAddr[31:0] (Read/Write)
// 0x4c : Data signal of vAddr
//        bit 31~0 - vAddr[63:32] (Read/Write)
// 0x50 : reserved
// 0x54 : Data signal of rKey
//        bit 31~0 - rKey[31:0] (Read/Write)
// 0x58 : reserved
// 0x5c : Data signal of OP
//        bit 31~0 - OP[31:0] (Read/Write)
// 0x60 : reserved
// 0x64 : Data signal of rAddr
//        bit 31~0 - rAddr[31:0] (Read/Write)
// 0x68 : Data signal of rAddr
//        bit 31~0 - rAddr[63:32] (Read/Write)
// 0x6c : reserved
// 0x70 : Data signal of lAddr
//        bit 31~0 - lAddr[31:0] (Read/Write)
// 0x74 : Data signal of lAddr
//        bit 31~0 - lAddr[63:32] (Read/Write)
// 0x78 : reserved
// 0x7c : Data signal of len
//        bit 31~0 - len[31:0] (Read/Write)
// 0x80 : reserved
// 0x84 : Data signal of debug
//        bit 31~0 - debug[31:0] (Read/Write)
// 0x88 : reserved
// 0x8c : Data signal of mem_ptr
//        bit 31~0 - mem_ptr[31:0] (Read/Write)
// 0x90 : Data signal of mem_ptr
//        bit 31~0 - mem_ptr[63:32] (Read/Write)
// 0x94 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

//------------------------Parameter----------------------
localparam
    ADDR_AP_CTRL        = 8'h00,
    ADDR_GIE            = 8'h04,
    ADDR_IER            = 8'h08,
    ADDR_ISR            = 8'h0c,
    ADDR_RPSN_DATA_0    = 8'h10,
    ADDR_RPSN_CTRL      = 8'h14,
    ADDR_LPSN_DATA_0    = 8'h18,
    ADDR_LPSN_CTRL      = 8'h1c,
    ADDR_RQPN_DATA_0    = 8'h20,
    ADDR_RQPN_CTRL      = 8'h24,
    ADDR_LQPN_DATA_0    = 8'h28,
    ADDR_LQPN_CTRL      = 8'h2c,
    ADDR_RIP_DATA_0     = 8'h30,
    ADDR_RIP_CTRL       = 8'h34,
    ADDR_LIP_DATA_0     = 8'h38,
    ADDR_LIP_CTRL       = 8'h3c,
    ADDR_RUDP_DATA_0    = 8'h40,
    ADDR_RUDP_CTRL      = 8'h44,
    ADDR_VADDR_DATA_0   = 8'h48,
    ADDR_VADDR_DATA_1   = 8'h4c,
    ADDR_VADDR_CTRL     = 8'h50,
    ADDR_RKEY_DATA_0    = 8'h54,
    ADDR_RKEY_CTRL      = 8'h58,
    ADDR_OP_DATA_0      = 8'h5c,
    ADDR_OP_CTRL        = 8'h60,
    ADDR_RADDR_DATA_0   = 8'h64,
    ADDR_RADDR_DATA_1   = 8'h68,
    ADDR_RADDR_CTRL     = 8'h6c,
    ADDR_LADDR_DATA_0   = 8'h70,
    ADDR_LADDR_DATA_1   = 8'h74,
    ADDR_LADDR_CTRL     = 8'h78,
    ADDR_LEN_DATA_0     = 8'h7c,
    ADDR_LEN_CTRL       = 8'h80,
    ADDR_DEBUG_DATA_0   = 8'h84,
    ADDR_DEBUG_CTRL     = 8'h88,
    ADDR_MEM_PTR_DATA_0 = 8'h8c,
    ADDR_MEM_PTR_DATA_1 = 8'h90,
    ADDR_MEM_PTR_CTRL   = 8'h94,
    WRIDLE              = 2'd0,
    WRDATA              = 2'd1,
    WRRESP              = 2'd2,
    WRRESET             = 2'd3,
    RDIDLE              = 2'd0,
    RDDATA              = 2'd1,
    RDRESET             = 2'd2,
    ADDR_BITS         = 8;

//------------------------Local signal-------------------
    reg  [1:0]                    wstate = WRRESET;
    reg  [1:0]                    wnext;
    reg  [ADDR_BITS-1:0]          waddr;
    wire [31:0]                   wmask;
    wire                          aw_hs;
    wire                          w_hs;
    reg  [1:0]                    rstate = RDRESET;
    reg  [1:0]                    rnext;
    reg  [31:0]                   rdata;
    wire                          ar_hs;
    wire [ADDR_BITS-1:0]          raddr;
    // internal registers
    reg                           int_ap_idle;
    reg                           int_ap_ready;
    reg                           int_ap_done = 1'b0;
    reg                           int_ap_start = 1'b0;
    reg                           int_auto_restart = 1'b0;
    reg                           int_gie = 1'b0;
    reg  [1:0]                    int_ier = 2'b0;
    reg  [1:0]                    int_isr = 2'b0;
    reg  [31:0]                   int_rPSN = 'b0;
    reg  [31:0]                   int_lPSN = 'b0;
    reg  [31:0]                   int_rQPN = 'b0;
    reg  [31:0]                   int_lQPN = 'b0;
    reg  [31:0]                   int_rIP = 'b0;
    reg  [31:0]                   int_lIP = 'b0;
    reg  [31:0]                   int_rUDP = 'b0;
    reg  [63:0]                   int_vAddr = 'b0;
    reg  [31:0]                   int_rKey = 'b0;
    reg  [31:0]                   int_OP = 'b0;
    reg  [63:0]                   int_rAddr = 'b0;
    reg  [63:0]                   int_lAddr = 'b0;
    reg  [31:0]                   int_len = 'b0;
    reg  [31:0]                   int_debug = 'b0;
    reg  [63:0]                   int_mem_ptr = 'b0;

//------------------------Instantiation------------------

//------------------------AXI write fsm------------------
assign AWREADY = (wstate == WRIDLE);
assign WREADY  = (wstate == WRDATA);
assign BRESP   = 2'b00;  // OKAY
assign BVALID  = (wstate == WRRESP);
assign wmask   = { {8{WSTRB[3]}}, {8{WSTRB[2]}}, {8{WSTRB[1]}}, {8{WSTRB[0]}} };
assign aw_hs   = AWVALID & AWREADY;
assign w_hs    = WVALID & WREADY;

// wstate
always @(posedge ACLK) begin
    if (ARESET)
        wstate <= WRRESET;
    else if (ACLK_EN)
        wstate <= wnext;
end

// wnext
always @(*) begin
    case (wstate)
        WRIDLE:
            if (AWVALID)
                wnext = WRDATA;
            else
                wnext = WRIDLE;
        WRDATA:
            if (WVALID)
                wnext = WRRESP;
            else
                wnext = WRDATA;
        WRRESP:
            if (BREADY)
                wnext = WRIDLE;
            else
                wnext = WRRESP;
        default:
            wnext = WRIDLE;
    endcase
end

// waddr
always @(posedge ACLK) begin
    if (ACLK_EN) begin
        if (aw_hs)
            waddr <= AWADDR[ADDR_BITS-1:0];
    end
end

//------------------------AXI read fsm-------------------
assign ARREADY = (rstate == RDIDLE);
assign RDATA   = rdata;
assign RRESP   = 2'b00;  // OKAY
assign RVALID  = (rstate == RDDATA);
assign ar_hs   = ARVALID & ARREADY;
assign raddr   = ARADDR[ADDR_BITS-1:0];

// rstate
always @(posedge ACLK) begin
    if (ARESET)
        rstate <= RDRESET;
    else if (ACLK_EN)
        rstate <= rnext;
end

// rnext
always @(*) begin
    case (rstate)
        RDIDLE:
            if (ARVALID)
                rnext = RDDATA;
            else
                rnext = RDIDLE;
        RDDATA:
            if (RREADY & RVALID)
                rnext = RDIDLE;
            else
                rnext = RDDATA;
        default:
            rnext = RDIDLE;
    endcase
end

// rdata
always @(posedge ACLK) begin
    if (ACLK_EN) begin
        if (ar_hs) begin
            rdata <= 1'b0;
            case (raddr)
                ADDR_AP_CTRL: begin
                    rdata[0] <= int_ap_start;
                    rdata[1] <= int_ap_done;
                    rdata[2] <= int_ap_idle;
                    rdata[3] <= int_ap_ready;
                    rdata[7] <= int_auto_restart;
                end
                ADDR_GIE: begin
                    rdata <= int_gie;
                end
                ADDR_IER: begin
                    rdata <= int_ier;
                end
                ADDR_ISR: begin
                    rdata <= int_isr;
                end
                ADDR_RPSN_DATA_0: begin
                    rdata <= int_rPSN[31:0];
                end
                ADDR_LPSN_DATA_0: begin
                    rdata <= int_lPSN[31:0];
                end
                ADDR_RQPN_DATA_0: begin
                    rdata <= int_rQPN[31:0];
                end
                ADDR_LQPN_DATA_0: begin
                    rdata <= int_lQPN[31:0];
                end
                ADDR_RIP_DATA_0: begin
                    rdata <= int_rIP[31:0];
                end
                ADDR_LIP_DATA_0: begin
                    rdata <= int_lIP[31:0];
                end
                ADDR_RUDP_DATA_0: begin
                    rdata <= int_rUDP[31:0];
                end
                ADDR_VADDR_DATA_0: begin
                    rdata <= int_vAddr[31:0];
                end
                ADDR_VADDR_DATA_1: begin
                    rdata <= int_vAddr[63:32];
                end
                ADDR_RKEY_DATA_0: begin
                    rdata <= int_rKey[31:0];
                end
                ADDR_OP_DATA_0: begin
                    rdata <= int_OP[31:0];
                end
                ADDR_RADDR_DATA_0: begin
                    rdata <= int_rAddr[31:0];
                end
                ADDR_RADDR_DATA_1: begin
                    rdata <= int_rAddr[63:32];
                end
                ADDR_LADDR_DATA_0: begin
                    rdata <= int_lAddr[31:0];
                end
                ADDR_LADDR_DATA_1: begin
                    rdata <= int_lAddr[63:32];
                end
                ADDR_LEN_DATA_0: begin
                    rdata <= int_len[31:0];
                end
                ADDR_DEBUG_DATA_0: begin
                    rdata <= int_debug[31:0];
                end
                ADDR_MEM_PTR_DATA_0: begin
                    rdata <= int_mem_ptr[31:0];
                end
                ADDR_MEM_PTR_DATA_1: begin
                    rdata <= int_mem_ptr[63:32];
                end
            endcase
        end
    end
end


//------------------------Register logic-----------------
assign interrupt = int_gie & (|int_isr);
assign ap_start  = int_ap_start;
assign rPSN      = int_rPSN;
assign lPSN      = int_lPSN;
assign rQPN      = int_rQPN;
assign lQPN      = int_lQPN;
assign rIP       = int_rIP;
assign lIP       = int_lIP;
assign rUDP      = int_rUDP;
assign vAddr     = int_vAddr;
assign rKey      = int_rKey;
assign OP        = int_OP;
assign rAddr     = int_rAddr;
assign lAddr     = int_lAddr;
assign len       = int_len;
assign debug     = int_debug;
assign mem_ptr   = int_mem_ptr;
// int_ap_start
always @(posedge ACLK) begin
    if (ARESET)
        int_ap_start <= 1'b0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_AP_CTRL && WSTRB[0] && WDATA[0])
            int_ap_start <= 1'b1;
        else if (ap_ready)
            int_ap_start <= int_auto_restart; // clear on handshake/auto restart
    end
end

// int_ap_done
always @(posedge ACLK) begin
    if (ARESET)
        int_ap_done <= 1'b0;
    else if (ACLK_EN) begin
        if (ap_done)
            int_ap_done <= 1'b1;
        else if (ar_hs && raddr == ADDR_AP_CTRL)
            int_ap_done <= 1'b0; // clear on read
    end
end

// int_ap_idle
always @(posedge ACLK) begin
    if (ARESET)
        int_ap_idle <= 1'b0;
    else if (ACLK_EN) begin
            int_ap_idle <= ap_idle;
    end
end

// int_ap_ready
always @(posedge ACLK) begin
    if (ARESET)
        int_ap_ready <= 1'b0;
    else if (ACLK_EN) begin
            int_ap_ready <= ap_ready;
    end
end

// int_auto_restart
always @(posedge ACLK) begin
    if (ARESET)
        int_auto_restart <= 1'b0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_AP_CTRL && WSTRB[0])
            int_auto_restart <=  WDATA[7];
    end
end

// int_gie
always @(posedge ACLK) begin
    if (ARESET)
        int_gie <= 1'b0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_GIE && WSTRB[0])
            int_gie <= WDATA[0];
    end
end

// int_ier
always @(posedge ACLK) begin
    if (ARESET)
        int_ier <= 1'b0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_IER && WSTRB[0])
            int_ier <= WDATA[1:0];
    end
end

// int_isr[0]
always @(posedge ACLK) begin
    if (ARESET)
        int_isr[0] <= 1'b0;
    else if (ACLK_EN) begin
        if (int_ier[0] & ap_done)
            int_isr[0] <= 1'b1;
        else if (w_hs && waddr == ADDR_ISR && WSTRB[0])
            int_isr[0] <= int_isr[0] ^ WDATA[0]; // toggle on write
    end
end

// int_isr[1]
always @(posedge ACLK) begin
    if (ARESET)
        int_isr[1] <= 1'b0;
    else if (ACLK_EN) begin
        if (int_ier[1] & ap_ready)
            int_isr[1] <= 1'b1;
        else if (w_hs && waddr == ADDR_ISR && WSTRB[0])
            int_isr[1] <= int_isr[1] ^ WDATA[1]; // toggle on write
    end
end

// int_rPSN[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_rPSN[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_RPSN_DATA_0)
            int_rPSN[31:0] <= (WDATA[31:0] & wmask) | (int_rPSN[31:0] & ~wmask);
    end
end

// int_lPSN[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_lPSN[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_LPSN_DATA_0)
            int_lPSN[31:0] <= (WDATA[31:0] & wmask) | (int_lPSN[31:0] & ~wmask);
    end
end

// int_rQPN[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_rQPN[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_RQPN_DATA_0)
            int_rQPN[31:0] <= (WDATA[31:0] & wmask) | (int_rQPN[31:0] & ~wmask);
    end
end

// int_lQPN[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_lQPN[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_LQPN_DATA_0)
            int_lQPN[31:0] <= (WDATA[31:0] & wmask) | (int_lQPN[31:0] & ~wmask);
    end
end

// int_rIP[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_rIP[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_RIP_DATA_0)
            int_rIP[31:0] <= (WDATA[31:0] & wmask) | (int_rIP[31:0] & ~wmask);
    end
end

// int_lIP[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_lIP[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_LIP_DATA_0)
            int_lIP[31:0] <= (WDATA[31:0] & wmask) | (int_lIP[31:0] & ~wmask);
    end
end

// int_rUDP[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_rUDP[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_RUDP_DATA_0)
            int_rUDP[31:0] <= (WDATA[31:0] & wmask) | (int_rUDP[31:0] & ~wmask);
    end
end

// int_vAddr[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_vAddr[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_VADDR_DATA_0)
            int_vAddr[31:0] <= (WDATA[31:0] & wmask) | (int_vAddr[31:0] & ~wmask);
    end
end

// int_vAddr[63:32]
always @(posedge ACLK) begin
    if (ARESET)
        int_vAddr[63:32] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_VADDR_DATA_1)
            int_vAddr[63:32] <= (WDATA[31:0] & wmask) | (int_vAddr[63:32] & ~wmask);
    end
end

// int_rKey[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_rKey[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_RKEY_DATA_0)
            int_rKey[31:0] <= (WDATA[31:0] & wmask) | (int_rKey[31:0] & ~wmask);
    end
end

// int_OP[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_OP[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_OP_DATA_0)
            int_OP[31:0] <= (WDATA[31:0] & wmask) | (int_OP[31:0] & ~wmask);
    end
end

// int_rAddr[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_rAddr[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_RADDR_DATA_0)
            int_rAddr[31:0] <= (WDATA[31:0] & wmask) | (int_rAddr[31:0] & ~wmask);
    end
end

// int_rAddr[63:32]
always @(posedge ACLK) begin
    if (ARESET)
        int_rAddr[63:32] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_RADDR_DATA_1)
            int_rAddr[63:32] <= (WDATA[31:0] & wmask) | (int_rAddr[63:32] & ~wmask);
    end
end

// int_lAddr[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_lAddr[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_LADDR_DATA_0)
            int_lAddr[31:0] <= (WDATA[31:0] & wmask) | (int_lAddr[31:0] & ~wmask);
    end
end

// int_lAddr[63:32]
always @(posedge ACLK) begin
    if (ARESET)
        int_lAddr[63:32] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_LADDR_DATA_1)
            int_lAddr[63:32] <= (WDATA[31:0] & wmask) | (int_lAddr[63:32] & ~wmask);
    end
end

// int_len[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_len[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_LEN_DATA_0)
            int_len[31:0] <= (WDATA[31:0] & wmask) | (int_len[31:0] & ~wmask);
    end
end

// int_debug[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_debug[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_DEBUG_DATA_0)
            int_debug[31:0] <= (WDATA[31:0] & wmask) | (int_debug[31:0] & ~wmask);
    end
end

// int_mem_ptr[31:0]
always @(posedge ACLK) begin
    if (ARESET)
        int_mem_ptr[31:0] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_MEM_PTR_DATA_0)
            int_mem_ptr[31:0] <= (WDATA[31:0] & wmask) | (int_mem_ptr[31:0] & ~wmask);
    end
end

// int_mem_ptr[63:32]
always @(posedge ACLK) begin
    if (ARESET)
        int_mem_ptr[63:32] <= 0;
    else if (ACLK_EN) begin
        if (w_hs && waddr == ADDR_MEM_PTR_DATA_1)
            int_mem_ptr[63:32] <= (WDATA[31:0] & wmask) | (int_mem_ptr[63:32] & ~wmask);
    end
end


//------------------------Memory logic-------------------

endmodule
