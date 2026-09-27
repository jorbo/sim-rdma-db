`timescale 1ns / 1ps
`default_nettype none

// Host-issued RDMA operation controller: register arguments in, one 160-bit
// tx_meta beat out, done only after the completion token is consumed.
module tb_roce_host_op;

reg clk = 1'b0;
always #5 clk = ~clk;

reg resetn      = 1'b0;
reg start       = 1'b0;
reg configured  = 1'b0;

reg [31:0] OP    = 32'd0;              // 0 = APP_READ
reg [31:0] lQPN  = 32'h00000101;
reg [31:0] len   = 32'h00000028;       // sizeof(Node)
reg [63:0] rAddr = 64'h0000_0000_0000_0320;
reg [63:0] lAddr = 64'h0000_0000_0000_0000;

wire [159:0] meta_data;
wire         meta_valid;
reg          meta_ready = 1'b0;
reg          completion_valid = 1'b0;
wire         completion_ready;
wire         busy;
wire         done;
wire         error;

roce_host_op dut (
    .clk(clk),
    .resetn(resetn),
    .start(start),
    .configured(configured),
    .OP(OP),
    .lQPN(lQPN),
    .len(len),
    .rAddr(rAddr),
    .lAddr(lAddr),
    .meta_data(meta_data),
    .meta_valid(meta_valid),
    .meta_ready(meta_ready),
    .completion_valid(completion_valid),
    .completion_ready(completion_ready),
    .busy(busy),
    .done(done),
    .error(error)
);

task tick;
begin
    @(posedge clk);
    #1;
end
endtask

task pulse_start;
begin
    start = 1'b1;
    tick();
    start = 1'b0;
end
endtask

// Field layout must match rdma_bram_read_meta() in krnl/hls/rdma.hpp.
task check_meta_fields;
begin
    if (meta_data[2:0]     != OP[2:0])     $fatal(1, "meta op code mismatch");
    if (meta_data[26:3]    != lQPN[23:0])  $fatal(1, "meta local QPN mismatch");
    if (meta_data[74:27]   != lAddr[47:0]) $fatal(1, "meta local address mismatch");
    if (meta_data[122:75]  != rAddr[47:0]) $fatal(1, "meta remote address mismatch");
    if (meta_data[154:123] != len)         $fatal(1, "meta length mismatch");
    if (meta_data[159:155] != 5'd0)        $fatal(1, "meta padding bits not zero");
end
endtask

initial begin
    repeat (2) tick();
    resetn = 1'b1;
    tick();
    if (busy || done || meta_valid || completion_ready)
        $fatal(1, "controller not idle after reset");

    // ---- Case A: start before configure_roce has run. Fail fast, send nothing.
    configured = 1'b0;
    pulse_start();
    if (!done)       $fatal(1, "unconfigured start did not pulse done");
    if (!error)      $fatal(1, "unconfigured start did not flag error");
    if (meta_valid)  $fatal(1, "unconfigured start emitted metadata");
    if (busy)        $fatal(1, "unconfigured start went busy");
    tick();
    if (done)        $fatal(1, "done was not a pulse (case A)");

    // ---- Case B: configured READ. Beat held under backpressure, done gated
    // ---- on the completion token, busy spans the whole operation.
    configured = 1'b1;
    pulse_start();
    if (!meta_valid) $fatal(1, "metadata valid was not asserted");
    if (!busy)       $fatal(1, "busy was not asserted with metadata");
    if (error)       $fatal(1, "error still set on a valid start");
    if (completion_ready) $fatal(1, "completion accepted before metadata was sent");
    check_meta_fields();

    // Merger not ready yet: valid and payload must hold.
    repeat (3) tick();
    if (!meta_valid) $fatal(1, "metadata valid was not held under backpressure");
    check_meta_fields();

    // A stray ap_start mid-operation must be ignored.
    pulse_start();
    if (!meta_valid || !busy) $fatal(1, "stray start disturbed an in-flight operation");
    if (done) $fatal(1, "stray start pulsed done");
    check_meta_fields();

    // Merger accepts the beat.
    meta_ready = 1'b1;
    tick();
    meta_ready = 1'b0;
    if (meta_valid) $fatal(1, "metadata valid not dropped after handshake");
    if (!busy)      $fatal(1, "busy dropped before completion");
    if (done)       $fatal(1, "done fired before completion");
    if (!completion_ready) $fatal(1, "completion channel not ready after metadata sent");

    // No completion yet: still busy, no done, stays ready.
    repeat (4) tick();
    if (!busy || done) $fatal(1, "state drifted while waiting for completion");
    if (!completion_ready) $fatal(1, "completion ready was not held");

    // Completion token arrives. Done is a one-cycle pulse, busy drops with it.
    completion_valid = 1'b1;
    tick();
    completion_valid = 1'b0;
    if (!done)  $fatal(1, "done did not pulse on completion");
    if (busy)   $fatal(1, "busy did not drop on completion");
    if (error)  $fatal(1, "error set on a successful operation");
    if (completion_ready) $fatal(1, "completion ready not dropped after token");
    tick();
    if (done)   $fatal(1, "done was not a pulse (case B)");
    if (busy || meta_valid) $fatal(1, "controller not idle after completion");

    // ---- Case C: configured but unsupported op code (WRITE). Refuse.
    OP = 32'd1;
    pulse_start();
    if (!done || !error) $fatal(1, "unsupported op was not refused");
    if (meta_valid || busy) $fatal(1, "unsupported op emitted metadata");
    tick();
    OP = 32'd0;

    // ---- Case D: a second READ after an error clears error and runs normally.
    rAddr = 64'h0000_0000_0000_0640;
    pulse_start();
    if (!meta_valid) $fatal(1, "second read did not assert metadata");
    if (error)       $fatal(1, "error not cleared on a valid start");
    check_meta_fields();
    meta_ready = 1'b1;
    tick();
    meta_ready = 1'b0;
    completion_valid = 1'b1;
    tick();
    completion_valid = 1'b0;
    if (!done) $fatal(1, "second read did not complete");

    $display("TEST PASSED: RoCE host op metadata, backpressure, completion gating");
    $finish;
end

endmodule

`default_nettype wire
