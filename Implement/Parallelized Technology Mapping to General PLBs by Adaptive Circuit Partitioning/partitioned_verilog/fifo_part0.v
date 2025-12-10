module fifo_part0 (
    $procmux$32_S,
    $procmux$35_S,
    $procmux$38_S,
    fifo_writer_en,
    $procmux$35_B,
    fifo_writer_rst,
    $auto$proc_memwr.cc:45:proc_memwr$55_CLK,
    fifo_writer_clk
);
input $procmux$32_S;
input $procmux$35_S;
input $procmux$38_S;
input fifo_writer_en;
input [7:0] $procmux$35_B;
input fifo_writer_rst;
input $auto$proc_memwr.cc:45:proc_memwr$55_CLK;
input fifo_writer_clk;
wire [7:0] net_0;
wire [7:0] net_1;
wire [7:0] net_2;
wire [7:0] net_3;
$memwr_v2 $auto$proc_memwr.cc:45:proc_memwr$55 (
    .ADDR(net_1),
    .CLK($auto$proc_memwr.cc:45:proc_memwr$55_CLK),
    .DATA(net_0),
    .EN(net_2)
);
$mux $procmux$32 (
    .A(8'b00000000),
    .B(8'b11111111),
    .S($procmux$32_S),
    .Y(net_2)
);
$mux $procmux$35 (
    .A(8'bxxxxxxxx),
    .B($procmux$35_B),
    .S($procmux$35_S),
    .Y(net_0)
);
$mux $procmux$38 (
    .A(8'bxxxxxxxx),
    .B(net_3),
    .S($procmux$38_S),
    .Y(net_1)
);
$paramod\addr_gen\MAX_DATA=s32'00000000000000000000000100000000 fifo_writer (
    .addr(net_3),
    .clk(fifo_writer_clk),
    .en(fifo_writer_en),
    .rst(fifo_writer_rst)
);
endmodule
