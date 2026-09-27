`timescale 1ns / 1ps
`default_nettype none

module roce_host_op (
    input  wire         clk, resetn,
    input  wire         start,          // ap_start_pulse & (OP != NONE)
    input  wire         configured,     // set once setup_done has ever pulsed
    input  wire [31:0]  OP, lQPN, len,
    input  wire [63:0]  rAddr, lAddr,
    output reg  [159:0] meta_data,      // -> merger S00
    output reg          meta_valid,
    input  wire         meta_ready,
    input  wire         completion_valid,   // demuxed adapter output
    output wire         completion_ready,
    output reg          busy,           // steers the completion demux
    output reg          done,           // one-cycle pulse -> ap_done
    output reg          error           // started while !configured
);



localparam [2:0] OP_IDLE = 3'd0;
localparam [2:0] OP_META = 3'd1;
localparam [2:0] OP_WAIT_COMPLETION = 3'd2;
reg [2:0] state;
assign completion_ready = (state == OP_WAIT_COMPLETION);
always @(posedge clk) begin
    if (!resetn) begin
        op_state <= OP_IDLE;
        meta_valid <= 0;
        busy <= 0;
        error <= 0;
        done <= 0;
    end
    else begin
        done <= 1'b0;

        case (state)
            OP_IDLE:
                if (start) begin
                    if(!configured || OP != 32'd0) begin
                        error <= 1'b1;
                        done <= 1'b1;
                    end
                    else begin
                        meta_data <= {5'b0, len, rAddr[47:0], lAddr[47:0], lQPN[23:0], OP[2:0]};
                        meta_valid <= 1'b1;
                        busy <= 1'b1;
                        error <= 1'b0;
                        state <= OP_META;
                    end
                end
            OP_META:
                if (meta_ready) begin
                    meta_valid <= 1'b0;
                    state <= OP_WAIT_COMPLETION;
                end
            OP_WAIT_COMPLETION:
                if(completion_valid) begin
                    busy <= 1'b0;
                    done <= 1'b1;
                    state <= OP_IDLE;
                end
        endcase

        
    end

end


endmodule