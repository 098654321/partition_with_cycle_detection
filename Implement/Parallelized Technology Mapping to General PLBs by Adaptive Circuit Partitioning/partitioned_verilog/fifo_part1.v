module fifo_part1 (
    $procdff$48_CLK,
    fifo_reader_clk,
    $logic_not$data/demo/demo.v:67$18_A,
    $procdff$48_Q,
    $logic_not$data/demo/demo.v:65$15_Y,
    fifo_reader_rst,
    $logic_and$data/demo/demo.v:67$19_Y,
    $logic_and$data/demo/demo.v:67$19_A,
    $logic_not$data/demo/demo.v:65$15_A,
    fifo_reader_en
);
input $procdff$48_CLK;
input fifo_reader_clk;
input $logic_not$data/demo/demo.v:67$18_A;
output [7:0] $procdff$48_Q;
output $logic_not$data/demo/demo.v:65$15_Y;
input fifo_reader_rst;
output $logic_and$data/demo/demo.v:67$19_Y;
input $logic_and$data/demo/demo.v:67$19_A;
input $logic_not$data/demo/demo.v:65$15_A;
input fifo_reader_en;
wire net_0;
wire [7:0] net_1;
wire [7:0] net_2;
$logic_and $logic_and$data/demo/demo.v:67$19 (
    .A($logic_and$data/demo/demo.v:67$19_A),
    .B(net_0),
    .Y($logic_and$data/demo/demo.v:67$19_Y)
);
$logic_not $logic_not$data/demo/demo.v:65$15 (
    .A($logic_not$data/demo/demo.v:65$15_A),
    .Y($logic_not$data/demo/demo.v:65$15_Y)
);
$logic_not $logic_not$data/demo/demo.v:67$18 (
    .A($logic_not$data/demo/demo.v:67$18_A),
    .Y(net_0)
);
$memrd $memrd$\data$data/demo/demo.v:39$13 (
    .ADDR(net_2),
    .CLK(1'bx),
    .DATA(net_1),
    .EN(1'bx)
);
$dff $procdff$48 (
    .CLK($procdff$48_CLK),
    .D(net_1),
    .Q($procdff$48_Q)
);
$paramod\addr_gen\MAX_DATA=s32'00000000000000000000000100000000 fifo_reader (
    .addr(net_2),
    .clk(fifo_reader_clk),
    .en(fifo_reader_en),
    .rst(fifo_reader_rst)
);
endmodule
