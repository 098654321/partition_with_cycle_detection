## 概览

- muxpack 是一个优化 pass，用来把按层级串接的 $mux / $pmux 链打包成一个单一的 $pmux ，减少层级和器件数量，利于后续映射与优化。入口类是 MuxpackPass ，遍历选中的模块并对每个模块运行 MuxpackWorker ，最后打印统计信息( /yosys/passes/opt/muxpack.cc:344 、 /yosys/passes/opt/muxpack.cc:364 ).
- 该优化是“保守”的：只有在能够证明这些选择信号在同一数据源上互斥时才会打包。互斥性由 ExclusiveDatabase 建立和查询( /yosys/passes/opt/muxpack.cc:27-114 )，它识别由 $eq 、 $logic_not 、 $reduce_or 形成的比较/归约结构，判断选择信号是否对同一非常量信号的不同常量值比较，从而互斥。

## 关键数据结构

- ExclusiveDatabase ( /yosys/passes/opt/muxpack.cc:27-114 )
  - 建库阶段扫描模块内的 $eq 、 $logic_not 、 $reduce_or ：
    - $eq ：提取一端非常量、一端常量的比较，记录输出 Y 位对应的“非常量源”和“常量值”( /yosys/passes/opt/muxpack.cc:40-64 ).
    - $logic_not ：等价于“输入为零”的比较，记录“非常量源”和常量零( /yosys/passes/opt/muxpack.cc:50-54 ).
    - $reduce_or ：如果其所有输入都来自同一“非常量源”的不同常量比较，则归并为“该源与集合中任一常量相等”( /yosys/passes/opt/muxpack.cc:66-89 ).
  - query(sig) ：对给定选择信号 sig 的各比特，检查它们都对应同一“非常量源”，且关联的常量集合没有重复，成立则认为互斥( /yosys/passes/opt/muxpack.cc:92-113 ).
- 链构建所用索引( /yosys/passes/opt/muxpack.cc:126-133 ):
  - sig_chain_next : 某信号作为 A/B 的使用者是哪一个 $mux/$pmux （用于沿 Y→A/B 方向向前拓展链）( /yosys/passes/opt/muxpack.cc:134-181 ).
  - sig_chain_prev : 某信号作为 Y 的驱动者是哪一个 $mux/$pmux （用于沿 A/B→Y 方向回溯）( /yosys/passes/opt/muxpack.cc:144-174 ).
  - sigbit_with_non_chain_users : 记录被非（p）mux单元或端口使用的比特，避免误把共享信号并进链( /yosys/passes/opt/muxpack.cc:136-143 , /yosys/passes/opt/muxpack.cc:176-180 ).
  - candidate_cells 、 chain_start_cells : 候选（p）mux及判定为链起点的单元集合( /yosys/passes/opt/muxpack.cc:129-131 , /yosys/passes/opt/muxpack.cc:183-222 ).

## 算法流程

- 索引构建： make_sig_chain_next_prev() ( /yosys/passes/opt/muxpack.cc:134-181 )
  - 遍历模块，标记端口输出或 keep 属性的线网位为“有非链用户”，并收集所有非（p）mux单元的输入比特。
  - 对每个 $mux/$pmux 候选，记录其 A 、（如是 $mux 则还有） B 的使用映射到 sig_chain_next ，以及其输出 Y 的驱动映射到 sig_chain_prev 。出现重复使用同一驱动信号的情况就把该信号比特标记为“有非链用户”，以阻止打包错误共享。
- 链起点判定： find_chain_start_cells() ( /yosys/passes/opt/muxpack.cc:183-222 )
  - 对每个候选（p）mux：
    - $mux ：要求 A 与 B 中恰有一个由另一（p）mux的 Y 驱动；否则就是链的起点( /yosys/passes/opt/muxpack.cc:190-197 ).
    - $pmux ：要求 A 由另一（p）mux的 Y 驱动( /yosys/passes/opt/muxpack.cc:198-201 ).
    - 若被判断为“可能延链”，还需满足：
      - 用作延链输入的信号比特不被非链用户使用( /yosys/passes/opt/muxpack.cc:204-207 ).
      - 将本单元的选择端 S 与前驱的选择端 S 拼接后，能在 ExclusiveDatabase 中验证互斥( /yosys/passes/opt/muxpack.cc:209-215 ).
    - 任何条件不满足就将该单元标记为链起点( /yosys/passes/opt/muxpack.cc:219-221 ).
- 链构建： create_chain(start_cell) 沿 Y→A/B 向前拓展，直到没有后继或遇到另一个起点( /yosys/passes/opt/muxpack.cc:224-244 ).
- 链打包： process_chain(chain) ( /yosys/passes/opt/muxpack.cc:246-298 )
  - 将一整段链打包成一个 $pmux ：
    - 首单元类型改为 $pmux ，其 B 和 S 端扩展为后续各级的相应端连接。若出现链中某级是反向（即该级的 A 接前一级 Y ），则把其“另一路”作为候选，并对其 S 做逻辑取反( /yosys/passes/opt/muxpack.cc:272-288 ).
    - 设置 S_WIDTH 参数为新的 S 宽度，并把首单元的 Y 端指向链末单元的 Y ( /yosys/passes/opt/muxpack.cc:291-295 ).
    - 后续级别标记删除( /yosys/passes/opt/muxpack.cc:288 , /yosys/passes/opt/muxpack.cc:300-310 ).
  - 统计计数： mux_count 增加链级数， pmux_count 增加1( /yosys/passes/opt/muxpack.cc:269-271 ).
- 驱动： MuxpackPass::execute 遍历所有被选择的模块，实例化 MuxpackWorker 执行流程并汇总统计( /yosys/passes/opt/muxpack.cc:358-365 ).

## 互斥性判定的意义与覆盖面

- 目标是识别“同一数据源 X 与不同常量值比较”的选择信号组合（如 S_i = (X == c_i) 或 S = reduce_or{(X==c1), (X==c2), ...} ），这种结构保证同一个周期内不会同时激活多个分支，从而安全地把串接的（p）mux打包成一个多路选择器。
- 通过支持 $logic_not 把“ X==0 ”纳入比较，通过 $reduce_or 把“ X属于{c1,c2,...} ”纳入集合比较，从而覆盖常见的 case / if-else 综合产物( /yosys/passes/opt/muxpack.cc:50-54 , /yosys/passes/opt/muxpack.cc:66-89 ).

## 与opt目录其他pass的关系

- opt_muxtree 会在更广的“mux树”层面进行可观测性、端口激活分析，剪枝不可达端口并收缩树结构。其比特使用跟踪与“非mux用户”识别的思路与 muxpack 相似（如对端口输出/ keep 线网的保留处理），但目标不同： muxpack 以“打包”减少链级， opt_muxtree 以“删减/替换常量/直连”精简逻辑。参见 /yosys/passes/opt/opt_muxtree.cc:160-164 , :205-219 , :280-317 .
- pmux2shiftx 可能把某些 $pmux 模式转换为 $shiftx ，进一步利于技术映射，常在 muxpack 之后受益( /yosys/passes/opt/pmux2shiftx.cc , 目录中可见文件名)。
- opt_share 、 opt_expr 、 opt_reduce 等会重写表达式、共享逻辑与宽度归约，为 ExclusiveDatabase 提供更规整的比较/归约模式，提升 muxpack 的适配率。

## 执行时间的主要影响因素

- 模块规模与单元数量
  - 初始化遍历所有单元与线网是线性的； ExclusiveDatabase 、索引构建、起点判定均按模块规模线性扩展。**大量单元**会直接增加运行时间( /yosys/passes/opt/muxpack.cc:39-64 , :134-181 , :183-222 ).
- 比较结构的复杂度
  - $reduce_or 的**输入宽度**越大，归并时需要逐比特查找并验证来源一致与累积常量集合，成本随输入位数线性增加( /yosys/passes/opt/muxpack.cc:66-89 ).
  - query(s_sig) 对选择信号逐比特检查映射与常量集合去重，**选择端越宽开销越大**( /yosys/passes/opt/muxpack.cc:92-113 ).
- SigSpec / SigMap 规范化成本
  - 频繁对端口进行 sigmap() 统一化、 SigSpec 拼接、比较会产生显著但必要的常量时间开销；在大规模宽总线和深链场景中，这类操作的累计成本不可忽略( /yosys/passes/opt/muxpack.cc:141-149 , :211-213 , :233-239 , :273-287 ).
- 字典/集合操作的规模
  - dict<SigSpec, Cell*> 与 pool<SigBit> 的插入、查找近似常数期望时间，但键值是位向量/比特，数量和唯一性直接影响哈希表尺寸与缓存局部性，规模越大总体成本越高( /yosys/passes/opt/muxpack.cc:126-133 , :154-170 ).
- 候选链长度和数量
  - 链越长，打包时端口拼接、删除单元、参数更新等工作随链长线性扩展；大量短链的管理开销也会积累( /yosys/passes/opt/muxpack.cc:246-298 , :300-310 ).
- 非链用户与 keep /端口输出
  - 这些使用会把潜在候选排除或切断链，使算法更少深入，但需要在索引阶段遍历并标记，整体仍随规模增长( /yosys/passes/opt/muxpack.cc:136-143 , :176-180 ).
- 与其他pass的交互顺序
  - 若在 opt_reduce / opt_expr 之后运行，比较结构更规整， ExclusiveDatabase 匹配更快；若在 share 或 muxtree 之后，候选数量减少也会降低 muxpack 花费。

## 使用建议

- 典型流程在命令行中运行（macOS）：
  - yosys -p "read_verilog design.v; proc; opt; muxpack; opt_muxtree; pmux2shiftx; opt_clean; stat" .
- 为加速大规模设计：
  - 优先在子模块上选择运行，或在前置 opt_* 清理后运行 muxpack 。
  - 尽量让比较逻辑保持 $eq/$reduce_or/$logic_not 的规范形态，减少复杂、难以识别的控制结构。
  - 避免滥用 keep 属性和将链信号暴露为顶层端口，否则会阻止打包。