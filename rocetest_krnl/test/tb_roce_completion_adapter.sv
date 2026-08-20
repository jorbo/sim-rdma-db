`timescale 1ns / 1ps

module tb_roce_completion_adapter;

reg         clk = 1'b0;
reg         status_valid = 1'b0;
wire        status_ready;
reg  [7:0]  status_data = 8'h00;
wire        completion_valid;
reg         completion_ready = 1'b0;
wire [31:0] completion_data;
wire [3:0]  completion_keep;
wire        completion_last;

integer status_handshakes = 0;
integer completion_handshakes = 0;

always #5 clk = ~clk;

always @(posedge clk) begin
  if (status_valid && status_ready)
    status_handshakes <= status_handshakes + 1;
  if (completion_valid && completion_ready)
    completion_handshakes <= completion_handshakes + 1;
end

roce_completion_adapter #(
  .STATUS_WIDTH(8),
  .COMPLETION_WIDTH(32)
) dut (
  .status_valid(status_valid),
  .status_ready(status_ready),
  .status_data(status_data),
  .completion_valid(completion_valid),
  .completion_ready(completion_ready),
  .completion_data(completion_data),
  .completion_keep(completion_keep),
  .completion_last(completion_last)
);

initial begin
  // Backpressure must hold the DataMover status without losing or accepting it.
  status_data = 8'ha5;
  status_valid = 1'b1;
  completion_ready = 1'b0;
  @(posedge clk);
  #1;
  if (!completion_valid || status_ready)
    $fatal(1, "status was accepted while completion output was stalled");
  if (completion_data !== 32'h000000a5 || completion_keep !== 4'hf || !completion_last)
    $fatal(1, "completion payload framing is incorrect");
  if (status_handshakes !== 0 || completion_handshakes !== 0)
    $fatal(1, "a handshake occurred under backpressure");

  // Releasing backpressure must produce exactly one input/output handshake.
  completion_ready = 1'b1;
  @(posedge clk);
  #1;
  status_valid = 1'b0;
  if (status_handshakes !== 1 || completion_handshakes !== 1)
    $fatal(1, "first completion did not handshake exactly once");

  // A second status must map one-for-one and preserve its status byte.
  @(negedge clk);
  status_data = 8'h3c;
  status_valid = 1'b1;
  @(posedge clk);
  #1;
  if (completion_data !== 32'h0000003c || completion_keep !== 4'hf || !completion_last)
    $fatal(1, "second completion payload framing is incorrect");
  status_valid = 1'b0;
  @(posedge clk);
  #1;
  if (status_handshakes !== 2 || completion_handshakes !== 2)
    $fatal(1, "completion handshakes were not one-for-one");

  $display("TEST PASSED: RoCE DataMover status completion bridge");
  $finish;
end

endmodule
