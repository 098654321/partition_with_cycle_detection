module fifo_part2 (
    $add$data/demo/demo.v:66$17_A,
    $procdff$47_Q,
    $procmux$26_A,
    $sub$data/demo/demo.v:68$20_A,
    $procdff$47_CLK,
    $logic_and$data/demo/demo.v:65$16_A,
    $logic_and$data/demo/demo.v:65$16_B,
    $procdff$47_ARST,
    $procmux$26_S
);
input [8:0] $add$data/demo/demo.v:66$17_A;
output [8:0] $procdff$47_Q;
input [8:0] $procmux$26_A;
input [8:0] $sub$data/demo/demo.v:68$20_A;
input $procdff$47_CLK;
input $logic_and$data/demo/demo.v:65$16_A;
input $logic_and$data/demo/demo.v:65$16_B;
input $procdff$47_ARST;
input $procmux$26_S;
wire [31:0] net_0;
wire [8:0] net_1;
wire [8:0] net_2;
wire net_3;
wire [8:0] net_4;
wire [8:0] net_5;
wire [31:0] net_6;
$add $add$data/demo/demo.v:66$17 (
    .A($add$data/demo/demo.v:66$17_A),
    .B(32'b10000000000000000000000000000000),
    .Y(net_0)
);
$logic_and $logic_and$data/demo/demo.v:65$16 (
    .A($logic_and$data/demo/demo.v:65$16_A),
    .B($logic_and$data/demo/demo.v:65$16_B),
    .Y(net_3)
);
$adff $procdff$47 (
    .ARST($procdff$47_ARST),
    .CLK($procdff$47_CLK),
    .D(net_4),
    .Q($procdff$47_Q)
);
$mux $procmux$26 (
    .A($procmux$26_A),
    .B(net_1),
    .S($procmux$26_S),
    .Y(net_5)
);
$mux $procmux$29 (
    .A(net_5),
    .B(net_2),
    .S(net_3),
    .Y(net_4)
);
$sub $sub$data/demo/demo.v:68$20 (
    .A($sub$data/demo/demo.v:68$20_A),
    .B(32'b10000000000000000000000000000000),
    .Y(net_6)
);
endmodule
