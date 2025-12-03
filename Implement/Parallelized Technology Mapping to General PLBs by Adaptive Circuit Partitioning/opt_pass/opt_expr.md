## 概览

- 目标： opt_expr 在表达式级别做“常量折叠、模式识别、布尔/算术精简、门级重写”，把能直接求值或可替换的运算用更简单的结构代替，或直接用常量驱动，从而减少电路复杂度并为后续优化打下规范化基础。
- 入口：按模块拓扑顺序遍历可求值单元，逐类规则尝试替换或重写，过程中维护 SigMap 统一信号代表与 invert_map 帮助消除“多余倒相”( /yosys/passes/opt/opt_expr.cc:493-520 , :396-410 ).
未驱动信号处理

- 函数： replace_undriven 扫描模块，找出“使用过但无人驱动”的比特，将其赋值为常量（优先使用线网上的 init 位作为常量）然后更新或删除 init 属性( /yosys/passes/opt/opt_expr.cc:34-117 ).
- 规则要点：
  - 建立“驱动集合”“使用集合”“全体集合”后， all_signals - driven_signals 即未驱动；对其中“内部临时（以 $ 开头）”信号再用“使用集合”筛掉无用片段( /yosys/passes/opt/opt_expr.cc:68-79 ).
  - 将未驱动片段连为常量值，记录触及到的 init 线网，随后把 init 中已等价为常量连线的位清空，若整条线 init 全为 x 则删除( /yosys/passes/opt/opt_expr.cc:80-116 ).
倒相与极性归一

- 倒相映射：收集 $_NOT_/$not/$logic_not 和“1/0输入的 $mux ”形成的等价倒相映射，用于后续消除双倒相和翻转选择端( /yosys/passes/opt/opt_expr.cc:396-410 ).
- 极性处理：
  - 在触发器/锁存器/内存读写等单元上，若其时钟/复位/使能的信号等价于某输出的倒相，则翻转端口极性参数或交换单元类型的“带N/带P”变体，使电路更规整( /yosys/passes/opt/opt_expr.cc:416-491 , :319-347 ).
  - 例如 $_DFF_N_↔$_DFF_P_ 、 $_SR_N?_/$_SR_P?_ 等映射族( /yosys/passes/opt/opt_expr.cc:444-491 ).
拓扑序与迭代规则

- 对可求值单元构建有向图，按“输出到输入”的依赖拓扑排序，便于先简化源再简化用户；若存在组合环或常量导致无法严格拓扑，则给出提示但仍继续( /yosys/passes/opt/opt_expr.cc:493-520 ).
布尔与归约精简

- and/or/reduce_and/reduce_or 常量规则：输入含 0 （或存在互为倒相的两输入），输出恒 0 ；含 1 则对 or 恒 1 ；否则若仅一端为“非常量”，直接缓冲该端( /yosys/passes/opt/opt_expr.cc:526-586 ).
- xor/xnor ：
  - 单比特相等或含 x/z （在 keepdc=false ）时输出为常量；单边为常量时将其转为“倒相或直通”，并保持 xnor 与“ $_XOR_ + $_NOT_ ”一致性( /yosys/passes/opt/opt_expr.cc:588-637 ).
- 一元归约的缓冲与改写：单比特 reduce_xnor → $not ，其他一元归约/取负在输入为1位时直接缓冲( /yosys/passes/opt/opt_expr.cc:640-654 ).
- 细粒度归组（启用 do_fine ）：
  - 将位级输入按“常量/非常量/两侧常量”分组，生成多个更小的门或直接连线，支持 $not/$pos/$and/$or/$xor/$xnor 及逻辑型 $logic_* 和 $reduce_* ，并删除“中性位”（如 reduce_and 的 1 、 logic_or 的 0 ）( /yosys/passes/opt/opt_expr.cc:135-305 , :804-848 ).
  - 特例： $reduce_and/$reduce_or 在输入全 0/1/x 时将 A 直接改为常量以缩短宽度( /yosys/passes/opt/opt_expr.cc:850-874 ).
多路器相关

- 去除选择端倒相：若 S 是某输出的倒相，交换 A/B 并替换 S 为倒相源( /yosys/passes/opt/opt_expr.cc:1089-1098 ).
- 常量模式：
  - $_MUX_/$mux 若 S/B/A 匹配常量三元组，替换为直连、倒相、 x 等；部分模式把 mux 变成 $_NOT_ ( /yosys/passes/opt/opt_expr.cc:1161-1189 ).
  - mux_bool 将 A=0,B=1 替成 S ；或 A=1,B=0 替成倒相( $not/$_NOT_ )； A=0 或 B=1 时可转成 and/or （在“吞X”时）( /yosys/passes/opt/opt_expr.cc:1464-1529 ).
  - mux_undef 删除 $mux/$pmux 中完全 undef 的选择或分支，若只剩一个分支则直连；当 A=0,B=1 则转成 S 并改参数( /yosys/passes/opt/opt_expr.cc:1531-1580 ).
  - 宽多路器 $_MUX4_/8/16_ ：若所有数据输入均 undef 则直连 A ( /yosys/passes/opt/opt_expr.cc:1582-1594 ).
算术与比较精简

- 单元常量折叠宏：大部分算术/逻辑/移位比较在两端均为常量时直接调用 RTLIL::const_* 计算并替换单元( /yosys/passes/opt/opt_expr.cc:1596-1702 ).
- 位宽与LSB剥离：
  - add/sub ：根据低位的常量模式剥离若干LSB，缩短端口与输出宽度( /yosys/passes/opt/opt_expr.cc:928-967 ).
  - alu ：在确定 CI/BI 为常量时按低位模式剥离( /yosys/passes/opt/opt_expr.cc:970-1027 ).
- 常量移位： A 按 B 常量位移重写为固定布线；忽略越界位并处理算术右移的符号扩展( /yosys/passes/opt/opt_expr.cc:1307-1349 ).
- 乘法优化：
  - 乘以零→零驱动；乘以 onehot →左移；前导零剥离降低乘法器宽度( /yosys/passes/opt/opt_expr.cc:1745-1843 ).
- 除法/取模优化：
  - 除以零→ undef ；除/取模 onehot →右移/掩码；有些情况加上“余数非零且结果为负”的修正项( /yosys/passes/opt/opt_expr.cc:1846-1942 ).
- 比较优化：
  - eq/ne/eqx/nex ：按位删除成对冗余输入，若发现“不可相等”矛盾则直接输出常量( /yosys/passes/opt/opt_expr.cc:2039-2111 ).
  - cmpzero ：比较零转成 $logic_not/$reduce_bool 并去掉另一端( /yosys/passes/opt/opt_expr.cc:1288-1305 ).
  - 简化 lt/le/ge/gt ：对常量上界/下界，替换为恒值或用 reduce_or/not 实现区间判定；对 onehot 常量用位段化简( /yosys/passes/opt/opt_expr.cc:2114-2170 , :2172-2217 以及后续片段).
“吞X”与身份化规则

- consume_x 打开时，某些 and/or/xor/mux 在遇到 x 时会“吞掉”到确定值，更激进地替换（比如 mux AND/OR 变体）( /yosys/passes/opt/opt_expr.cc:1119-1122 , :1138-1141 , :1155-1159 , :1491-1529 ).
- 身份元素替换：在 add/sub/or/xor/mul/div/shift 族中识别“0/1或±1/移位0”等身份元素，直接把整个单元替为 $pos/$neg 或端口直通，并修正 $alu 的伴随端口( /yosys/passes/opt/opt_expr.cc:1351-1461 ).

## 执行时间的主要影响因素

- 单元与位宽规模
  - 遍历所有可求值单元并对端口进行 SigMap 归一化，规则大多按位扫描与分组，成本与**“单元数 × 端口位宽”**近似线性。
- 拓扑排序与依赖图
  - 构建 cells 图及 outbit_to_cell 字典需要遍历所有连接；存在环时排序失败但继续处理，可能导致规则应用顺序不利，需要更多迭代( /yosys/passes/opt/opt_expr.cc:493-520 ).
- 倒相与极性处理
  - invert_map 构建一次，但后续频繁查询；大量触发器/内存端口的极性翻转与类型切换会增加端口检查与 SigMap 调用成本( /yosys/passes/opt/opt_expr.cc:416-491 ).
- 常量折叠与宏展开
  - 宏 FOLD_* 在两端常量时直接调用 RTLIL::const_* ，快速且廉价；但要先做 SigMap.apply ，大规模设计中归一化开销累计( /yosys/passes/opt/opt_expr.cc:1596-1702 ).
- 细粒度分组与重写
  - group_cell_inputs 按位分桶、重布线并新建小单元；在宽总线下分组和信号构建显著增加工作量( /yosys/passes/opt/opt_expr.cc:135-305 ).
- 算术特化
  - 乘法、除法、ALU拆分等规则涉及多端口同步变更、添加新单元或连接；规则命中越多、位宽越大，改写成本越高( /yosys/passes/opt/opt_expr.cc:1745-1843 , :1846-1942 , :1946-2036 ).
- keepdc/mux_undef/mux_bool/consume_x 开关
  - 这些选项会启用更激进或更保守的规则；开启“吞X”和“mux布尔化”增加匹配次数与改写操作，影响总耗时。
与opt目录其他pass的关系

- opt_reduce 在布尔/算术层面做进一步归约，常与 opt_expr 配合先后运行，相互提供更规范的结构。
- opt_demorgan 把倒相推过归约运算，减少倒相器数量； opt_expr 可随后识别更多常量与缓冲模式。
- muxpack / opt_muxtree 依赖更规范的 $mux/$pmux 结构； opt_expr 的 mux 简化能提升它们的命中率。
- opt_dff 在触发器层面吸收 mux 与合并控制； opt_expr 提前把“D端的简单模式”简化，有助于 opt_dff 抓取。
- opt_clean 在 opt_expr 之后清理被替换/断开的单元和线网，收敛设计。
使用建议

- 常用序列（示例）： proc; opt; opt_expr; opt_reduce; opt_clean; stat
- 大设计加速：
  - 先运行 wreduce 降低宽度；选择性地启用 do_fine 与 consume_x ，在控制X传播的场景用 -keepdc 保守处理。
  - 通过 select 限定优化范围到包含大量组合逻辑的模块，减少全局遍历时间。
- 与多路器优化协作：在 muxpack / opt_muxtree 之前先用 opt_expr 规整 $mux/$pmux ，提升打包与树剪枝效率。
如需，我可以结合你的工程实际模块的统计结果，建议 -mux_undef/-mux_bool/-keepdc 等参数组合与顺序，平衡优化收益与运行时间