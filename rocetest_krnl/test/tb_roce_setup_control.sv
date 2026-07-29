`timescale 1ns / 1ps
`default_nettype none

module tb_roce_setup_control;

reg clk = 1'b0;
always #5 clk = ~clk;

reg resetn = 1'b0;
reg start = 1'b0;

reg [31:0] rPSN  = 32'h00112233;
reg [31:0] lPSN  = 32'h00445566;
reg [31:0] rQPN  = 32'h00000100;
reg [31:0] lQPN  = 32'h00000101;
reg [31:0] rIP   = 32'h0afd0001;
reg [31:0] rUDP  = 32'd4791;
reg [63:0] vAddr = 64'd0;
reg [31:0] rKey  = 32'h0000cafe;

wire [143:0] qp_data;
wire         qp_valid;
reg          qp_ready = 1'b0;
wire [183:0] conn_data;
wire         conn_valid;
reg          conn_ready = 1'b0;
wire [31:0]  arp_request_data;
wire         arp_request_valid;
reg          arp_request_ready = 1'b0;
reg [55:0]   arp_reply_data = 56'd0;
reg          arp_reply_valid = 1'b0;
wire         arp_reply_ready;
wire         done;

roce_setup_control #(
    .ARP_RETRY_CYCLES(3)
) dut (
    .clk(clk),
    .resetn(resetn),
    .start(start),
    .rPSN(rPSN),
    .lPSN(lPSN),
    .rQPN(rQPN),
    .lQPN(lQPN),
    .rIP(rIP),
    .rUDP(rUDP),
    .vAddr(vAddr),
    .rKey(rKey),
    .qp_data(qp_data),
    .qp_valid(qp_valid),
    .qp_ready(qp_ready),
    .conn_data(conn_data),
    .conn_valid(conn_valid),
    .conn_ready(conn_ready),
    .arp_request_data(arp_request_data),
    .arp_request_valid(arp_request_valid),
    .arp_request_ready(arp_request_ready),
    .arp_reply_data(arp_reply_data),
    .arp_reply_valid(arp_reply_valid),
    .arp_reply_ready(arp_reply_ready),
    .done(done)
);

task tick;
begin
    @(posedge clk);
    #1;
end
endtask

initial begin
    repeat (2) tick();
    resetn = 1'b1;
    tick();

    start = 1'b1;
    tick();
    start = 1'b0;

    if (!qp_valid) $fatal(1, "QP valid was not asserted");
    if (qp_data[2:0] != 3'b010) $fatal(1, "QP state is not READY_RECV");
    if (qp_data[26:3] != 24'h000101) $fatal(1, "QP context key is not local QPN");
    if (qp_data[50:27] != rPSN[23:0]) $fatal(1, "remote PSN mismatch");
    if (qp_data[74:51] != lPSN[23:0]) $fatal(1, "local PSN mismatch");
    if (qp_data[90:75] != rKey[15:0]) $fatal(1, "rkey mismatch");
    if (qp_data[138:91] != 48'd0) $fatal(1, "logical local base mismatch");

    // QP valid must remain asserted under backpressure.
    repeat (2) tick();
    if (!qp_valid) $fatal(1, "QP valid was not held");
    qp_ready = 1'b1;
    tick();
    qp_ready = 1'b0;

    if (!conn_valid) $fatal(1, "connection valid was not asserted");
    if (conn_data[15:0] != 16'h0101) $fatal(1, "connection key is not local QPN");
    if (conn_data[39:16] != 24'h000100) $fatal(1, "remote QPN mismatch");
    if (conn_data[167:136] != 32'h0100fd0a) $fatal(1, "remote IPv4 byte order mismatch");
    if (conn_data[183:168] != 16'd4791) $fatal(1, "remote UDP port mismatch");

    // Connection valid and payload must also remain stable under backpressure.
    repeat (2) tick();
    if (!conn_valid) $fatal(1, "connection valid was not held");
    if (conn_data[15:0] != 16'h0101 ||
        conn_data[39:16] != 24'h000100 ||
        conn_data[167:136] != 32'h0100fd0a) begin
        $fatal(1, "connection payload changed under backpressure");
    end
    conn_ready = 1'b1;
    tick();
    conn_ready = 1'b0;
    if (!arp_request_valid) $fatal(1, "ARP request was not asserted");
    if (arp_request_data != 32'h0100fd0a) $fatal(1, "ARP target is not exactly rIP");

    // ARP request valid/data must remain stable until accepted.
    repeat (2) tick();
    if (!arp_request_valid) $fatal(1, "ARP request valid was not held");
    if (arp_request_data != 32'h0100fd0a) $fatal(1, "ARP target changed under backpressure");
    arp_request_ready = 1'b1;
    tick();
    arp_request_ready = 1'b0;
    if (!arp_reply_ready) $fatal(1, "ARP reply channel was not ready");

    // First lookup misses. The controller must not complete and must retry.
    arp_reply_data[48] = 1'b0;
    arp_reply_valid = 1'b1;
    tick();
    arp_reply_valid = 1'b0;
    if (done) $fatal(1, "setup completed on an ARP miss");

    repeat (2) tick();
    if (arp_request_valid) $fatal(1, "ARP retry fired too early");
    tick();
    if (!arp_request_valid) $fatal(1, "ARP request was not retried");

    arp_request_ready = 1'b1;
    tick();
    arp_request_ready = 1'b0;
    if (!arp_reply_ready) $fatal(1, "ARP reply channel was not re-armed");

    // Second lookup hits. Completion must be a one-cycle pulse.
    arp_reply_data[48] = 1'b1;
    arp_reply_valid = 1'b1;
    tick();
    arp_reply_valid = 1'b0;
    if (!done) $fatal(1, "setup did not complete on ARP hit");
    tick();
    if (done) $fatal(1, "setup done was not a pulse");

    $display("TEST PASSED: RoCE setup handshakes and ARP retry");
    $finish;
end

endmodule

`default_nettype wire
