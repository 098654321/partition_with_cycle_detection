## Problem C

关注 logic synthesis 流程中的 logic rewriting



## Cases

[The EPFL Combinational Benchmark Suite](https://github.com/lsils/benchmarks.git)

其中的 random_control 数据是 problem C 使用的 case


## 数据集情况

1. BLIF 格式

- BLIF （Berkeley Logic Interchange Format）：面向逻辑综合的文本网表格式，描述组合与简单时序电路。
- 核心构成： .model 、 .inputs 、 .outputs 、 .names （以真值表描述逻辑节点）、 .latch （寄存器/锁存器）、 .end
