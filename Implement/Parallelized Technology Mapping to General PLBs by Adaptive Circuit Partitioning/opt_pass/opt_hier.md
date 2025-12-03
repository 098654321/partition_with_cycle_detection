## 概览

- opt_hier 做“跨层次边界优化”：把“未使用的端口位、常量端口位、需要绑在一起的端口位”的信息在父/子模块之间双向传播，便于在层次化设计不打扁的情况下进行进一步清理和常量折叠。
- 该 pass要求顶层模块被选择，否则报错( /Users/jiaheng/FDU_files/Tao_group/partition_with_cycle_detection/yosys/passes/opt/opt_hier.cc:432 )；它会改变非顶层模块的端口语义（帮助信息已警告这一点， /Users/jiaheng/.../opt_hier.cc:418 ）。
数据结构

- ModuleIndex ：为“从模块向外传播”建立索引( /Users/jiaheng/.../opt_hier.cc:27-88 )
  - 记录“使用过的比特” used （端口输出、进程、已知/未知类型单元的输入， /Users/jiaheng/.../opt_hier.cc:50-64 ）。
  - 记录“输出端口上的常量驱动” constant_outputs （通过 module->connections_ 识别， /Users/jiaheng/.../opt_hier.cc:66-80 ）。
  - 记录“需要绑在一起的输出端口位集合” tie_together_outputs （同一右值驱动多个输出端口位， /Users/jiaheng/.../opt_hier.cc:82-88 ）。
- UsageData ：为“向模块内传播”收集使用信息( /Users/jiaheng/.../opt_hier.cc:200-238 )
  - 初始化“所有输入端口位候选常量”为 Sx ，并收集所有输入/输出集合( /Users/jiaheng/.../opt_hier.cc:217-237 )。
  - 累积“某模块哪些输出端口位被父模块使用” used_outputs 、输入端口位的常量候选 constant_inputs 、需要绑在一起的输入位组 tie_together_inputs ( /Users/jiaheng/.../opt_hier.cc:239-283 )。
向外传播（父模块侧应用）

- 函数： ModuleIndex::apply_changes(parent_index, instantiation) ( /Users/jiaheng/.../opt_hier.cc:90-197 )
  - 忽略黑盒子子模块( /Users/jiaheng/.../opt_hier.cc:93-96 )和双向端口（难以保证健壮处理， /Users/jiaheng/.../opt_hier.cc:112-111 ）。
  - 对子模块的输入端口：若该端口位在子模块内未被使用，则把父模块中该实例连接处的该位改为 Sx ，并统计断开数量( /Users/jiaheng/.../opt_hier.cc:115-121 , :147-151 )。
  - 对子模块的输出端口：若子模块该输出位在内部是常量，且父模块确实使用了该位，则在父模块里把实例连接替换为该常量，并用一个临时线网替换实例端口位以保持端口连接完整( /Users/jiaheng/.../opt_hier.cc:124-145 , :152-155 )。
  - 传播“输出端口绑在一起”：若父模块通过该实例同时使用了同一子模块输出组内的多个端口位，则在父模块内把除第一个之外的位都连接到第一个位，实现绑在一起；同时在实例侧用临时线网替代被“切断”的端口位( /Users/jiaheng/.../opt_hier.cc:158-180 , :182-195 )。
向内传播（子模块侧应用）

- 函数： UsageData::refine(instance, parent_index) ( /Users/jiaheng/.../opt_hier.cc:284-312 )
  - 从父模块的实例连接中提取该子模块端口的实际连接，标注哪些输出端口位在父模块里被使用( /Users/jiaheng/.../opt_hier.cc:301-304 , :239-245 )。
  - 输入端口位的常量候选：若父模块连接的是确定常量且兼容当前候选，则更新为该常量；若冲突则移除候选( /Users/jiaheng/.../opt_hier.cc:246-262 )。
  - 输入端口绑在一起：按父模块连接（相同右值）的分组更新需要绑在一起的输入位组( /Users/jiaheng/.../opt_hier.cc:264-283 )。
- 函数： UsageData::apply_changes(index) ( /Users/jiaheng/.../opt_hier.cc:313-403 )
  - 忽略黑盒子( /Users/jiaheng/.../opt_hier.cc:316-320 )。
  - 断开未使用的输出端口位：生成替换映射，把这些输出位重写为新的临时线网，并在子模块内部把它们连接为 Sx ；全模块重写以保证引用处一致( /Users/jiaheng/.../opt_hier.cc:321-354 )。
  - 连接常量输入：把累积的 constant_inputs 应用到模块内部所有信号，替换为常量；统计并打印替换( /Users/jiaheng/.../opt_hier.cc:355-373 )。
  - 传播“输入端口绑在一起”：仅考虑父索引 index.used 标注为“被使用”的输入端口位，把同一组中的后续位重写为第一个位；对全模块信号进行重写( /Users/jiaheng/.../opt_hier.cc:374-403 )。
Pass执行流程

- 构建所有模块的 ModuleIndex ( /Users/jiaheng/.../opt_hier.cc:434-439 )。
- 为所有“被选择的非顶层模块”构建 UsageData ( /Users/jiaheng/.../opt_hier.cc:441-447 )。
- 在整个设计上遍历实例，先“收集/细化”每个 UsageData （根据父模块里该模块的实例连接），让子模块端知道外部使用情况( /Users/jiaheng/.../opt_hier.cc:449-456 )。
- 遍历被选择模块：
  - 若该模块有 UsageData ，先在该模块内部应用“向内传播”的改变( /Users/jiaheng/.../opt_hier.cc:462-466 )。
  - 对该模块里的每个实例，应用其子模块的 ModuleIndex::apply_changes 做“向外传播”（常量替换、断开未使用输入、绑在一起）( /Users/jiaheng/.../opt_hier.cc:467-473 )。
- 若有变更，设置 opt.did_something 标记( /Users/jiaheng/.../opt_hier.cc:474-476 )。
典型效果

- 父模块侧：不再把常量通过实例向下传再返回；直接在父模块处用常量替换实例输出连接。未被子模块使用的输入端位清空为 Sx ，减少无效驱动。对相同源驱动的多个子模块输出位，父模块里直接绑在一起。
- 子模块侧：未被父模块使用的输出端位断开，不参与内部逻辑传播；输入端位如果在所有实例中都连接到某常量，则该端位在子模块内部直接替换为常量；需要绑在一起的输入端位在子模块内部重写为同一信号。

## 执行时间的主要影响因素

- 模块/实例规模
  - 为每个模块构建 ModuleIndex 要遍历线网、进程、单元与连接；为每个被选择的非顶层模块构建 UsageData 并在整个设计中“细化”，**成本与模块数、实例数成线性关系**( /Users/jiaheng/.../opt_hier.cc:434-456 )。
- 位宽与连接密度
  - connections_ 按位扫描以识别端口常量与分组； rewrite_sigspecs 会对全模块的所有信号进行替换，复杂度与**信号位总数线性增长且常数因子较高**( /Users/jiaheng/.../opt_hier.cc:66-80 , :333-349 , :364-373 , :389-397 )。
- 使用集检查与 SigMap
  - 频繁进行 used.check(sigmap(bit)) 集合查询；大设计中 SigMap 归一化与集合查找开销累积( /Users/jiaheng/.../opt_hier.cc:45-49 , :239-245 , :124-133 , :381-387 )。
- 绑在一起的类集合构建
  - 构造 tie_together_outputs / tie_together_inputs 类并筛选，仅当组大小>1才产生工作；这些分组和后续重写会按“参与组的位数”线性扩展( /Users/jiaheng/.../opt_hier.cc:82-88 , :236-237 , :374-387 )。
- 黑盒与双向端口
  - 黑盒与双向端口被跳过，减少不确定性但也减少可优化的范围（少量早退检查成本， /Users/jiaheng/.../opt_hier.cc:93-96 , :111 , :297 ）。
与opt目录其他pass的关系

- opt_clean 非常关键： opt_hier 重写后会产生更多未用线网与临时线网，需用 opt_clean 清理掉，保证设计收敛与统计优化收益。
- opt_expr / opt_reduce ：经由 opt_hier 常量传播后，组合逻辑里更容易发生常量折叠与表达式精简，提升命中率。
- muxpack / opt_muxtree ：跨层次把无效选择/常量数据前推后，可减少多路器树的复杂度，提升这两个 pass 的效果。
- opt_dff ：输入端常量化与输出端断开能让 opt_dff 更容易识别“常不激活控制/常量位”，从而合并或删除寄存器。
使用建议

- 推荐顺序： proc; opt; opt_hier; opt_expr; opt_reduce; opt_clean; stat 。在需要跨层次信息流的层次化设计上尤其有效；打平（ flatten ）后该 pass 价值降低。
- 选择范围控制：
  - 必须选择顶层；非顶层模块尽量只选择有明确父实例使用关系的模块，减少全局重写成本。
  - 黑盒模块不会应用变化；对IP核或不希望修改的层次可标注为黑盒，避免跨边界传播。
- 注意端口语义变化：
  - 非顶层模块的端口被替换为常量或“绑在一起”，后续综合/仿真应以修改后的端口语义为准。若不希望这类更改，避免选择该模块运行 opt_hier 。