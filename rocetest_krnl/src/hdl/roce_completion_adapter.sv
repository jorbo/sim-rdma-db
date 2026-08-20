`timescale 1ns / 1ps
`default_nettype none

module roce_completion_adapter #(
  parameter integer STATUS_WIDTH = 8,
  parameter integer COMPLETION_WIDTH = 32
) (
  input  wire                                status_valid,
  output wire                                status_ready,
  input  wire [STATUS_WIDTH-1:0]             status_data,
  output wire                                completion_valid,
  input  wire                                completion_ready,
  output wire [COMPLETION_WIDTH-1:0]         completion_data,
  output wire [(COMPLETION_WIDTH/8)-1:0]     completion_keep,
  output wire                                completion_last
);

assign completion_valid = status_valid;
assign status_ready = completion_ready;
assign completion_data = {
  {(COMPLETION_WIDTH-STATUS_WIDTH){1'b0}}, status_data
};
assign completion_keep = {(COMPLETION_WIDTH/8){1'b1}};
assign completion_last = 1'b1;

endmodule

`default_nettype wire
