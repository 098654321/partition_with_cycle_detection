## 概览

- 目标： opt_dff 对内建触发器/锁存器进行结构归一与化简，包括合并同步复位与使能、移除常不激活的控制、将常激活控制转换为组合电路、识别并替换常量寄存位，以及在可能情况下把寄存器删掉改为常量驱动。
- 入口： OptDffPass::execute 解析选项后，对每个已选择模块创建 OptDffWorker 并调用两阶段优化：结构优化 run() 与常量位优化 run_constbits() ，如有变化，设置 opt.did_something 标记( /yosys/passes/opt/opt_dff.cc:948-996 ).
关键数据结构

- OptDffWorker 构造时建立：
  - bitusers : 每个位的用户计数；只有当某比特只有一个用户（FF或其前级）时才敢把前级 $mux/$pmux “吸入”FF( /yosys/passes/opt/opt_dff.cc:64-96 ).
  - bit2mux : 某位是否由 $mux/$pmux/$_MUX_ 驱动，记录驱动单元与对应比特索引( /yosys/passes/opt/opt_dff.cc:77-83 ).
  - dff_cells : 模块内已选择且为内建FF的单元队列( /yosys/passes/opt/opt_dff.cc:92-96 ).
- FfData ：把一个FF拆解为特征字段（宽度、D/Q、时钟、同步/异步复位、使能、异步加载、极性、初始化值等），并支持切片与重新发射( /yosys/passes/opt/opt_dff.cc:324-371 , :785-789 ).
- 模式提取与合成：
  - find_muxtree_feedback_patterns ：在 D 端沿 $mux 树回溯，识别“从 Q 反馈到D的路径”以及控制位取值模式，用于生成使能等控制( /yosys/passes/opt/opt_dff.cc:108-171 ).
  - simplify_patterns ：约简出互补/冗余的模式集合，降低控制逻辑复杂度( /yosys/passes/opt/opt_dff.cc:173-229 ).
  - make_patterns_logic ：将模式与控制集合合成最终的 CE 控制逻辑，必要时用 simplemap 将内建单元映射为基本门( /yosys/passes/opt/opt_dff.cc:231-281 ).
  - combine_resets ：将多个同步复位控制汇合为一个等效的复位控制信号（根据极性使用 reduce_or 或 reduce_and ）( /yosys/passes/opt/opt_dff.cc:283-315 ).
结构优化流程

- 主循环：弹出一个FF，构建 FfData ，按顺序尝试各种化简与转换( /yosys/passes/opt/opt_dff.cc:317-333 ).
- 常激活/常不激活控制处理：
  - 同步/异步清零/置位：常激活则把 Q 连到常量或组合电路、并删除FF；常不激活则移除该控制或转为异步复位等等价结构( /yosys/passes/opt/opt_dff.cc:333-436 , :500-516 , :518-535 ).
  - 使能 CE ：常不激活则在保持行为正确的前提下移除D路径或转接 SRST 等；常激活则直接移除 CE ( /yosys/passes/opt/opt_dff.cc:537-569 ).
  - 常量时钟：如初始化值可定义，则移除D路径并删除时钟/使能等；否则保持未定义初始化并把D回接Q( /yosys/passes/opt/opt_dff.cc:571-586 ).
  - D=Q或AD=Q：移除相应路径，使FF退化( /yosys/passes/opt/opt_dff.cc:588-627 , :629-634 ).
- 合并同步复位 SDFF ：
  - 在 D 端吸收以常量为一支的 $mux （重置值在 A/B ，选择 S ），将若干比特分组，合成统一的 SRST 控制并重建FF；剩余比特切片保留( /yosys/passes/opt/opt_dff.cc:636-719 ).
- 合并使能 DFFE ：
  - 在 D 端吸收以 Q 为一支的 $mux （另一支为更新值，选择 S ），再结合从 Q 反馈的 mux 树模式；合成统一 CE 并重建FF；剩余比特切片保留( /yosys/passes/opt/opt_dff.cc:720-782 ).
- 变更生效：如有更改，调用 ff.emit() 重建；如全部比特已处理，删除原单元( /yosys/passes/opt/opt_dff.cc:785-789 , :710-717 ).
常量位优化

- run_constbits() 用SAT证明某寄存位在所有可达赋值下保持初始值不变，从而用常量替代该位( /yosys/passes/opt/opt_dff.cc:794-909 ).
  - 组合常量合并：把 init 与各类复位值、 SR 控制影响结合，若仍明确为0/1则继续( /yosys/passes/opt/opt_dff.cc:809-823 ).
  - 对 CLK 或 ALOAD 路径，若 D/AD 是常量直接合并；否则在开启 -sat 时用 QuickConeSat 构造 IFF(Q, init) ∧ ¬IFF(D, init) 约束寻找反例，若无反例则可替换为常量( /yosys/passes/opt/opt_dff.cc:825-851 , :859-879 ).
  - 删除整位或切片保留未删除位，最后发射新FF并移除旧单元( /yosys/passes/opt/opt_dff.cc:881-907 ).
选项与行为

- -nosdff ：禁止识别/合并同步复位( /yosys/passes/opt/opt_dff.cc:958-966 , :636-719 ).
- -nodffe ：禁止识别/合并使能( /yosys/passes/opt/opt_dff.cc:960-966 , :720-782 ).
- -simple-dffe ：只对“明显”情况做使能识别，不进行 mux 树反馈模式的复杂搜索( /yosys/passes/opt/opt_dff.cc:968-975 , :746-749 ).
- -sat ：启用SAT进行常量位证明( /yosys/passes/opt/opt_dff.cc:976-979 , :794-909 ).
- -keepdc ：保持X的不定行为，不进行可能改变X传播的优化，如 combine_const 会保守地保留X，减少替换机会( /yosys/passes/opt/opt_dff.cc:972-975 , :98-106 ).

## 执行时间的主要影响因素

- FF数量与位宽
  - 所有逻辑均按FF及其位进行；结构优化对每位做 bit2mux 吸收尝试；分组与切片会遍历位集，时间近似线性于“FF位总数”( /yosys/passes/opt/opt_dff.cc:636-686 , :720-756 ).
- 前级 mux 树复杂度
  - find_muxtree_feedback_patterns 沿 D 的 $mux 图递归探索，视选择位组合枚举路径并回溯到 Q ，在复杂/深的 mux 树上可能指数增长； simplify_patterns 对模式集合做互补/冗余消除，复杂度与模式数量的平方相关( /yosys/passes/opt/opt_dff.cc:108-171 , :173-212 , :213-229 ).
- bitusers 与 bit2mux 图构建
  - 初始扫描**所有单元连接、端口**， SigMap 归一化，每比特记录用户与驱动，时间线性于“单元连接比特总数”；哈希映射的键为位对象，规模大时查找与插入成本累积( /yosys/passes/opt/opt_dff.cc:64-96 ).
- 逻辑合成与映射
  - make_patterns_logic 与 combine_resets 可能创建 ne 、 reduce_and/or 、 Not 等单元，并在细粒度模式下调用 simplemap 把内建单元立即映射为基本门，创建/删除的频率与分组数量成正比( /yosys/passes/opt/opt_dff.cc:231-281 , :283-315 ).
- SigMap 与连接更新
  - 每次读取端口都要 sigmap ，切片与重建FF要更新连接与参数；大量位宽与频繁切片时成本明显( /yosys/passes/opt/opt_dff.cc:649-673 , :758-781 , :785-789 ).
- SAT证明
  - 每寄存位一次SAT（若启用 -sat 且驱动非常量），复杂度与位的驱动锥大小相关； QuickConeSat 会抽取局部锥体，仍可能较重。大设计下建议谨慎使用或限制选择范围( /yosys/passes/opt/opt_dff.cc:794-909 ).
- 日志与切片次数
  - 结构优化中按组生成新FF并回入队列，会增加循环次数；每次日志输出也有I/O开销，但相较算法代价通常次要。
与opt目录其他pass的关系

- opt_clean 会移除被吸收后留下的未使用 $mux 与信号，进一步收敛；也会处理冗余 init （与 FfInitVals 保持一致）。
- opt_reduce 、 opt_expr 简化布尔/算术结构，让 mux 树与控制信号更规范，有利于 opt_dff 识别与合并。
- muxpack / opt_muxtree 在多路器层级上优化；之后 opt_dff 更容易把简单的 D 端 $mux 吸入FF。
- wreduce 减少宽度，降低 opt_dff 在宽总线上的工作量与搜索空间。
- 建议顺序： proc; opt; opt_reduce/expr; muxpack/opt_muxtree; opt_dff; opt_clean 。
使用建议

- 常用命令（macOS）：
  - yosys -p "read_verilog design.v; proc; opt; opt_reduce; opt_dff -simple-dffe; opt_clean; stat"
  - 启用SAT时： yosys -p "read_verilog design.v; proc; opt; opt_dff -sat; opt_clean; stat"
- 控制开销：
  - 用 select 限定到包含大量FF且 D 端前级 mux 结构简单的模块。
  - 在大型设计中谨慎使用 -sat ，或先用 opt_reduce 、 muxpack 降低锥体复杂度。
  - 当不希望改变X传播时启用 -keepdc ，但会减少优化机会并提高总体运行时间。
- 观察输出：
  - 日志中包含每类转换的说明与具体单元；配合后续 opt_clean 统计“Removed %d unused cells and %d unused wires”可评估净收益与热点。