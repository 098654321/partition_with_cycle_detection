概览

- 目标： opt_demorgan 通过德摩根变换在 $reduce_* 归约单元上“推送”倒相，使整体倒相数量更少、结构更规整，便于后续优化与映射。
- 范围：当前仅处理 $reduce_and 与 $reduce_or ，不处理 reduce_xor （代码中有TODO说明）( /Users/jiaheng/FDU_files/Tao_group/partition_with_cycle_detection/yosys/passes/opt/opt_demorgan.cc:35-39 ).
核心思路

- 观察归约输入的每一位是否由 $_NOT_ 的 Y 端驱动，统计被倒相的输入位数量 num_inverted ( /Users/jiaheng/.../opt_demorgan.cc:47-67 ).
- 若“被倒相位”占到至少一半（否则收益不明显），则执行德摩根变换：
  - 对每一位：
    - 未倒相的位：增加一个 $_NOT_ 作为输入倒相( /Users/jiaheng/.../opt_demorgan.cc:99-106 ).
    - 已倒相的位：旁路该倒相（直接用倒相器的 A 端原信号作为归约输入），不立即删除倒相器，以防还有其他用户( /Users/jiaheng/.../opt_demorgan.cc:107-111 ).
  - 将输入信号集 A 替换为经过上述处理后的 insig ( /Users/jiaheng/.../opt_demorgan.cc:157-159 ).
  - 翻转归约类型（ reduce_and ↔ reduce_or ）( /Users/jiaheng/.../opt_demorgan.cc:161-165 ).
  - 在输出一侧补上一个倒相器，使语义保持不变( /Users/jiaheng/.../opt_demorgan.cc:167-171 ).
- 额外“整洁化”：若输入位只是同一总线的乱序采样且恰好每一位出现一次，则重排为总线位序，避免归约上的奇怪顺序( /Users/jiaheng/.../opt_demorgan.cc:113-155 ).
代码流程与定位

- 入口 pass： OptDemorganPass::execute 遍历被选择模块与单元，构建模块索引 ModIndex 后调用 demorgan_worker ( /Users/jiaheng/.../opt_demorgan.cc:186-203 ).
- 单元级处理： demorgan_worker(index, cell, cells_changed) ( /Users/jiaheng/.../opt_demorgan.cc:27-172 )
  - 过滤：只处理 $reduce_and/$reduce_or ( /Users/jiaheng/.../opt_demorgan.cc:38-39 ).
  - 统一信号：用 SigMap 获取并简化输入 A ( /Users/jiaheng/.../opt_demorgan.cc:41-46 ).
  - 检测倒相：通过 index.query_ports(bit) 查询该位的驱动端口集合，判断是否由 $_NOT_ 的 Y 端输出驱动( /Users/jiaheng/.../opt_demorgan.cc:54-67 ).
  - 阈值判断：若倒相位少于一半则不做变换( /Users/jiaheng/.../opt_demorgan.cc:69-74 ).
  - 变换与旁路：按位添加/旁路倒相器，替换输入端口 A ，翻转归约类型，并在输出侧补倒相( /Users/jiaheng/.../opt_demorgan.cc:76-171 ).
  - 计数与日志：累加 cells_changed ，按条件输出日志( /Users/jiaheng/.../opt_demorgan.cc:76-81 , :201-203 ).
为什么能“减少门数”

- 初始状态：有 k 个输入位各自前置了倒相器， N 为输入位数。
- 变换后：所有输入位都视为“倒相后的值”参与归约，归约类型翻转，并在输出侧补一个倒相器。对原来未倒相的 N-k 位增加倒相器；对已倒相的 k 位旁路原倒相器。
- 后续 opt_clean 若发现旁路后的倒相器不再被任何用户使用，会删除它们。最终倒相器数量趋向于 (N-k) + 1 （输入补的加一个输出），与原来的 k 相比，当 k > (N+1)/2 时总体倒相数量减少；代码采用“至少一半”作为触发阈值（ num_inverted*2 >= N ），在偶数边界时可能会持平或略增，但通常配合后续优化（如合并相邻逻辑）仍有利于规整和综合。

## 执行时间的主要影响因素

- 归约单元数量与输入宽度
  - 该 pass 遍历所有被选择单元，且每个 $reduce_* 会按位查询驱动，时间与“归约单元数量 × 每个归约的位宽”线性相关( /Users/jiaheng/.../opt_demorgan.cc:194-199 , :47-67 , :82-111 ).
- 端口索引查询成本
  - ModIndex::query_ports(bit) 会返回驱动该位的端口集合。若网络中**共享驱动或扇出较高**，每位的查询集合更大，遍历判断是否 $_NOT_/Y 的成本上升。
  - 构建 ModIndex 的初始化也随模块规模（节点/边数）增加而增加（在 execute 中为每模块构建一次）。
- SigMap 规范化与信号重排
  - SigMap 用于统一/折叠别名，应用与比较在每次取端口时都会发生；输入位排序与整洁化逻辑包含 dict 统计与 SigSpec.sort() ，在**宽总线**归约时可能显著( /Users/jiaheng/.../opt_demorgan.cc:41-46 , :113-155 ).
- 结构变更与对象创建
  - 当触发变换时，会创建新线网、插入 $_NOT_ 倒相器；数量与“未倒相输入位数”成正比，并固定增加一个输出倒相器( /Users/jiaheng/.../opt_demorgan.cc:99-106 , :167-171 ).
  - 这些插入操作的开销在本 pass 内线性，但会增加后续优化 pass 的工作量（例如 opt_clean 删除旁路后的倒相器）。
- 选择范围与日志
  - 仅对“被选择”的模块与单元运行，合理选择能显著缩短时间；详细日志输出本身不会影响算法复杂度但会有I/O开销( /Users/jiaheng/.../opt_demorgan.cc:194-199 , :188-193 ).
与opt目录其他pass的关系

- opt_clean 会删除因本变换而旁路的 $_NOT_ ，以及未再使用的线网与单元，巩固变换收益（见 opt_clean 说明）。
- opt_reduce 、 opt_expr 在布尔代数与归约规则上进一步简化，通常在 opt_demorgan 后有更多可折叠机会。
- muxpack / opt_muxtree 与多路器结构相关，德摩根后的倒相集中也可能让选择控制更规整，利于树剪枝和打包。
- Pass 顺序建议：把 opt_demorgan 放在表达式归约前后都可，但与 opt_clean 搭配较重要，这样能及时移除旁路后遗留的倒相器和临时线网。
使用建议

- 常用命令序列（macOS）：
  - yosys -p "read_verilog design.v; proc; opt; opt_demorgan; opt_reduce; opt_clean; stat"
- 控制开销：
  - 用 select 限制在含大量 $reduce_and/$reduce_or 且倒相分布不均的模块上运行。
  - 宽总线下优先用 opt_reduce/wreduce 降低位宽再做德摩根，减少按位扫描与插入倒相的成本。
- 观察收益：
  - 结合 log 输出“Pushed inverters through %u reductions”与后续 opt_clean 的“Removed %d unused cells and %d unused wires”，即可评估本变换的触发次数与最终净化效果。