`timescale 1ns / 1ps
`default_nettype none

// Programs one local QP and its peer connection, then resolves the peer's MAC.
// The control transaction completes only after every AXI-stream handshake has
// happened and the ARP table reports a hit.
module roce_setup_control #(
    parameter integer ARP_RETRY_CYCLES = 250000
) (
    input  wire         clk,
    input  wire         resetn,
    input  wire         start,

    input  wire [31:0]  rPSN,
    input  wire [31:0]  lPSN,
    input  wire [31:0]  rQPN,
    input  wire [31:0]  lQPN,
    input  wire [31:0]  rIP,
    input  wire [31:0]  rUDP,
    input  wire [63:0]  vAddr,
    input  wire [31:0]  rKey,

    output reg  [143:0] qp_data,
    output reg          qp_valid,
    input  wire         qp_ready,

    output reg  [183:0] conn_data,
    output reg          conn_valid,
    input  wire         conn_ready,

    output reg  [31:0]  arp_request_data,
    output reg          arp_request_valid,
    input  wire         arp_request_ready,

    input  wire [55:0]  arp_reply_data,
    input  wire         arp_reply_valid,
    output wire         arp_reply_ready,

    output reg          done
);

localparam [2:0] SETUP_IDLE       = 3'd0;
localparam [2:0] SETUP_QP         = 3'd1;
localparam [2:0] SETUP_CONN       = 3'd2;
localparam [2:0] SETUP_ARP_REQ    = 3'd3;
localparam [2:0] SETUP_ARP_REPLY  = 3'd4;
localparam [2:0] SETUP_ARP_RETRY  = 3'd5;

reg [2:0]  setup_state;
reg [31:0] arp_retry_counter;

// Host arguments use conventional IPv4 integer order. The network stack keeps
// IPv4 bytes in wire order, with the final octet in bits [31:24].
wire [31:0] remote_ip_address;
assign remote_ip_address = {rIP[7:0], rIP[15:8], rIP[23:16], rIP[31:24]};

// arpTableReply is byte-compacted by HLS: MAC occupies [47:0] and hit is bit 48.
wire arp_reply_hit;
assign arp_reply_hit = arp_reply_data[48];
assign arp_reply_ready = (setup_state == SETUP_ARP_REPLY);

always @(posedge clk) begin
    if (!resetn) begin
        setup_state       <= SETUP_IDLE;
        arp_retry_counter <= 0;
        qp_data            <= 0;
        qp_valid           <= 1'b0;
        conn_data          <= 0;
        conn_valid         <= 1'b0;
        arp_request_data   <= 0;
        arp_request_valid  <= 1'b0;
        done               <= 1'b0;
    end
    else begin
        done <= 1'b0;

        if (start) begin
            // qpContext: READY_RECV, local QPN, peer/local PSNs, rkey,
            // and this node's logical RDMA base.
            qp_data <= {
                5'b0,
                vAddr[47:0],
                rKey[15:0],
                lPSN[23:0],
                rPSN[23:0],
                lQPN[23:0],
                3'b010
            };
            qp_valid <= 1'b1;

            // ifConnReq: lookup key is the local QPN. The result names the
            // remote QPN and the peer's IPv4/UDP endpoint.
            conn_data <= {
                rUDP[15:0],
                remote_ip_address,
                96'b0,
                rQPN[23:0],
                lQPN[15:0]
            };
            conn_valid <= 1'b0;

            arp_request_data  <= remote_ip_address;
            arp_request_valid <= 1'b0;
            arp_retry_counter <= 0;
            setup_state       <= SETUP_QP;
        end
        else begin
            case (setup_state)
                SETUP_IDLE: begin
                    qp_valid          <= 1'b0;
                    conn_valid        <= 1'b0;
                    arp_request_valid <= 1'b0;
                end

                SETUP_QP: begin
                    if (qp_valid && qp_ready) begin
                        qp_valid   <= 1'b0;
                        conn_valid <= 1'b1;
                        setup_state <= SETUP_CONN;
                    end
                end

                SETUP_CONN: begin
                    if (conn_valid && conn_ready) begin
                        conn_valid        <= 1'b0;
                        arp_request_valid <= 1'b1;
                        setup_state       <= SETUP_ARP_REQ;
                    end
                end

                SETUP_ARP_REQ: begin
                    if (arp_request_valid && arp_request_ready) begin
                        arp_request_valid <= 1'b0;
                        setup_state       <= SETUP_ARP_REPLY;
                    end
                end

                SETUP_ARP_REPLY: begin
                    if (arp_reply_valid) begin
                        if (arp_reply_hit) begin
                            done        <= 1'b1;
                            setup_state <= SETUP_IDLE;
                        end
                        else begin
                            arp_retry_counter <= 0;
                            setup_state       <= SETUP_ARP_RETRY;
                        end
                    end
                end

                SETUP_ARP_RETRY: begin
                    if ((ARP_RETRY_CYCLES <= 1) ||
                        (arp_retry_counter + 1'b1 >= ARP_RETRY_CYCLES)) begin
                        arp_retry_counter <= 0;
                        arp_request_valid <= 1'b1;
                        setup_state       <= SETUP_ARP_REQ;
                    end
                    else begin
                        arp_retry_counter <= arp_retry_counter + 1'b1;
                    end
                end

                default: begin
                    setup_state       <= SETUP_IDLE;
                    qp_valid          <= 1'b0;
                    conn_valid        <= 1'b0;
                    arp_request_valid <= 1'b0;
                end
            endcase
        end
    end
end

endmodule

`default_nettype wire
