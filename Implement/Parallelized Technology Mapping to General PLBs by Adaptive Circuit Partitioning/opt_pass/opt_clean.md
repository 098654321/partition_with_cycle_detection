## 概览

opt_clean用于清理设计中未使用的单元和线网，规范信号代表、合并连接、移除冗余的init属性，并标注未被使用的位。它常在其他优化 pass 后执行，用于“擦屁股”，让设计更干净、语义更明确。
两个入口 pass：OptCleanPass（详细日志，适用于完整选择模块）和CleanPass（相同功能但更安静，并且会在命令链分隔符;;或; ;;之间自动插入运行，参见帮助文本/yosys/passes/opt/opt_clean.cc:731-745）。主流程在execute中对所有选定模块调用rmunused_module并汇总统计(/yosys/passes/opt/opt_clean.cc:680-727, /yosys/passes/opt/opt_clean.cc:747-792).

## 保留策略

keep_cache定义保留规则：模块/单元如果有keep属性，或是断言/形式验证类单元，或$specify*、$print/$check、$scopeinfo等应保留；层级实例也会回溯查询其原型模块是否包含需要保留的元素(/yosys/passes/opt/opt_clean.cc:34-101).
在“清理未使用单元”阶段，初始队列会包含被保留的单元以及驱动端口输出或keep线网的驱动者(/yosys/passes/opt/opt_clean.cc:154-169).

## 未使用单元清理

函数：rmunused_module_cells(/yosys/passes/opt/opt_clean.cc:107-238)
建立映射：wire2driver记录每个比特的驱动单元；收集“原始连线”供日志定位；记录内存的读写/初始化单元以便一起保留(/yosys/passes/opt/opt_clean.cc:118-135, :140-153).
初始队列：保留单元进入队列，其他单元标为未使用；端口输出与keep线网的驱动者也入队(/yosys/passes/opt/opt_clean.cc:154-169).
迭代标记：从队列出发，收集其输入所需的驱动者与相关内存控制单元，逐步从unused转移到队列，直到稳定(/yosys/passes/opt/opt_clean.cc:171-201).
删除：剩余unused按名字排序依次移除；若是内置寄存器（FF），同步移除对应Q的初始化信息以维护一致性(/yosys/passes/opt/opt_clean.cc:203-213). 未被引用的memories也删除(/yosys/passes/opt/opt_clean.cc:215-221).
驱动冲突日志：如果一个比特有常量驱动与单元驱动的冲突且该比特被外部使用，发出警告(/yosys/passes/opt/opt_clean.cc:223-237).

## 信号代表与线网清理

函数：rmunused_module_signals(/yosys/passes/opt/opt_clean.cc:301-514)
代表选择基础：收集“寄存器输出信号”和“已连接信号”，并标记“直接驱动的线网”影响代表选择(/yosys/passes/opt/opt_clean.cc:305-338).
代表选择策略：逐比特比较候选代表，优先无端口输入→避免双向端口→公开名且优先寄存器输出/直接驱动/有连接→端口输出→公开名→非平凡属性少→名称排序；最终用SigMap把每个比特映射到更优代表(/yosys/passes/opt/opt_clean.cc:250-287, :340-349).
清空旧连接，统一所有单元连接至代表信号，同时记录使用情况（含不统计驱动的一份，用于unused_bits）(/yosys/passes/opt/opt_clean.cc:351-371).
端口与keep线网加入“使用集合”；提取并暂存各线网的init值到位级字典，再把位组内的init回写到每个线网(/yosys/passes/opt/opt_clean.cc:373-415).
删除与别名化：
对每条线网，若非端口、非keep、无init且不在“原始使用集合”中，则删除；否则如果它的比特与代表比特不同，生成新的连接把该线网别名到代表信号，并合并init到需要的位(/yosys/passes/opt/opt_clean.cc:417-474).
为非驱动使用缺失的位设置unused_bits属性，帮助后续分析(/yosys/passes/opt/opt_clean.cc:475-493).
执行删除并计数，必要时输出统计日志(/yosys/passes/opt/opt_clean.cc:496-514).
该函数可能重复运行直到稳定（返回true表示发生删除），由rmunused_module驱动循环(/yosys/passes/opt/opt_clean.cc:655-659).

## 冗余初始化清理

函数：rmunused_module_init(/yosys/passes/opt/opt_clean.cc:516-595)
收集所有FF的Q端位与其初始值（如果线网位上有init），建立位→初值映射(/yosys/passes/opt/opt_clean.cc:520-545).
对每条线网：如果其init与映射后的代表比特的初值一致（或者映射到常量且等于常量值），说明该线网的init冗余，移除它(/yosys/passes/opt/opt_clean.cc:547-589).
如果发生修改，设置设计“做了事情”的标记以触发后续清理(/yosys/passes/opt/opt_clean.cc:591-595).
顶层封装与其它清理

函数：rmunused_module(/yosys/passes/opt/opt_clean.cc:597-659)
先把简单的缓冲/连接类单元物化为直接连接并删除，包括$pos/$_BUF_/$buf、$connect、$input_port(/yosys/passes/opt/opt_clean.cc:602-651).
然后按顺序执行：删除未使用单元→重复删除未使用线网直到稳定→可选删除冗余init→再重复一次删除线网直到稳定(/yosys/passes/opt/opt_clean.cc:654-659).

## 执行时间的主要影响因素

设计规模与位宽
线性扫描所有模块、单元、线网与位；wire2driver、used_signals、init_bits等数据结构大小随总位数增长。总时间大致近似于**O(单元数 + 线网数 + 总位数)**。
驱动-使用图构建与BFS标记
建立wire2driver需要遍历所有单元的输出连接，并进行SigMap归一化；随后从队列进行多轮传播，将驱动者从unused转移到保留集合，复杂度与驱动边数量成正比(/yosys/passes/opt/opt_clean.cc:140-153, :171-201).
信号代表比较与SigMap归一化
compare_signals对每比特进行多条件比较：端口方向、公开名、是否寄存器输出、是否直接驱动、是否在连接中、属性数量与字典序等；代表选择遍历所有线网的所有比特，**宽总线耗时明显**(/yosys/passes/opt/opt_clean.cc:250-287, :342-349).
assign_map.apply对每个单元连接和线网进行归一化，次数和**总位数**相关(/yosys/passes/opt/opt_clean.cc:365-371, :379-395).
初始化属性处理
将init按位提取、回写，并在别名化过程中处理位级替换，**宽度越大开销越高**(/yosys/passes/opt/opt_clean.cc:373-415, :456-473).
冗余init检测需映射位与FFQ的初始化一致性，需要遍历所有FF与对应线网位(/yosys/passes/opt/opt_clean.cc:520-545, :547-589).
循环至稳定的次数
rmunused_module_signals通常需要多轮才能达成固定点（删除→代表变化→新的删除机会）。迭代次数与连线别名化复杂度相关，通常较低，但对于深层别名链可能增加总时间(/yosys/passes/opt/opt_clean.cc:655-659).
属性与日志
unused_bits生成和驱动冲突日志的收集/过滤会遍历位集合，若大量线网存在部分未用位或冲突，日志准备开销增大(/yosys/passes/opt/opt_clean.cc:475-493, :233-237).
模式与开关
-purge模式会允许删除带“公共名”的内部线网，提高删除率但也增加代表与删除判断的工作量(/yosys/passes/opt/opt_clean.cc:676-691, :440-447).
ys_debug()与verbose影响日志输出与一些分支判断，主要影响打印而非算法核心。
与opt目录其他pass的关系

opt_expr、opt_reduce、opt_merge等会重写表达式、合并等价逻辑，通常增加opt_clean的清理机会（更多未用线网/单元）。
muxpack与opt_muxtree减少多路器链级与端口，随后opt_clean能删除断开或重命名后的冗余信号与别名连接。
pmux2shiftx、wreduce等改变结构和位宽，opt_clean随之更新代表与连接，进一步收敛设计。
clean命令在命令序列中自动插入，有助于在多步优化间保持设计整洁与一致性(/yosys/passes/opt/opt_clean.cc:740-745).
使用建议

常用流程（macOS 命令示例）：
yosys -p "read_verilog design.v; proc; opt; opt_expr; opt_reduce; opt_clean; stat"
大型设计建议分模块选择运行以控制时间：select <module>; opt_clean
加速要点：
尽量减少不必要的keep属性与公开名的临时线网，避免阻止清理。
在宽总线场景下，先进行wreduce降低位宽，再运行opt_clean。
在进行大量结构性转换后（如muxpack、share），立刻运行opt_clean以收敛别名与删除未用线网，减少后续 pass 的负担。