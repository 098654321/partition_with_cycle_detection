## 概览

- 目标： opt_ffinv 把倒相器穿过触发器，使倒相被合并到另一侧的逻辑（尤其是 $lut ）中，减少独立的倒相器、规范控制极性，利于后续映射与优化。
- 入口： OptFfInvPass::execute 遍历选中的模块，对每个模块构造 OptFfInvWorker 并统计本次“推过”的倒相器数量( /yosys/passes/opt/opt_ffinv.cc:235-268 ).
- 适用范围：只处理“宽度为1的内建FF，且无 SR 、有 CLK 、无 ALOAD ”的寄存器( /yosys/passes/opt/opt_ffinv.cc:215-225 ).
核心流程

- 工作者初始化：记录模块、建立索引 ModIndex 、初始化FF初值访问器 FfInitVals ( /yosys/passes/opt/opt_ffinv.cc:29-35 , :204-208 ).
- FF筛选：遍历模块内选中的内建FF，过滤掉带 SR 、无 CLK 、有 ALOAD 、宽度不为1的情况( /yosys/passes/opt/opt_ffinv.cc:215-225 ).
- 两种可穿越模式：
  - Case 1：推过“D侧的倒相器”( /yosys/passes/opt/opt_ffinv.cc:36-118 )
    - 要求：FF的 D 不连到端口输入/输出； D 有且仅有两个端口用户（FF的 D 和另一个），另一个必须是倒相器 $not/$_NOT_ 或特殊的1位buffer-LUT（ WIDTH==1 且 LUT==1 ）；FF的 Q 不连到端口输出；所有 Q 的下游用户必须是倒相器或 $lut ( /yosys/passes/opt/opt_ffinv.cc:41-67 , :70-84 ).
    - 变换：
      - 翻转FF的复位位极性以保持语义（ ff.flip_rst_bits({0}) ），将 sig_d 改为倒相器的输入 A ( /yosys/passes/opt/opt_ffinv.cc:86-88 ).
      - 对以 Q 为输入的 $lut ，找到与 Q 相等的输入位位置，构造 flip_mask 并对LUT真值掩码做按位反转： new_mask[j] = mask[j ^ flip_mask] ( /yosys/passes/opt/opt_ffinv.cc:90-103 ). 如果该LUT等效于简单的1位buffer（ WIDTH==1 && new_mask.as_int()==2 ），直接用 Q 短接其输出并删除该LUT( /yosys/passes/opt/opt_ffinv.cc:104-113 ).
      - 对以 Q 为输入的倒相器，短接其输出到 Q 并删除倒相器( /yosys/passes/opt/opt_ffinv.cc:110-113 ).
      - 重新发射FF单元（更新后的端口/参数），返回成功( /yosys/passes/opt/opt_ffinv.cc:116-118 ).
  - Case 2：推过“Q侧的倒相器”( /yosys/passes/opt/opt_ffinv.cc:120-202 )
    - 要求：FF的 D 不连到端口输入/输出； D 有且仅有两个端口用户（FF的 D 和另一个），另一个必须是倒相器或 $lut ；FF的 Q 不连到端口输出，且 Q 正好有一个额外用户，该用户必须是倒相器或特殊1位buffer-LUT( /yosys/passes/opt/opt_ffinv.cc:126-145 , :147-173 ).
    - 变换：
      - 翻转FF复位位极性；将 sig_q 改为倒相器输出 Y ，并删除该倒相器( /yosys/passes/opt/opt_ffinv.cc:175-178 ).
      - 若 D 侧是 $lut ：对其真值表做全位取反（逐位0↔1），如果成为1位buffer（ WIDTH==1 && new_mask.as_int()==2 ），则绕过它并删除；若是 $not 则直接绕过并删除( /yosys/passes/opt/opt_ffinv.cc:179-198 ).
      - 重新发射FF单元，返回成功( /yosys/passes/opt/opt_ffinv.cc:200-202 ).
语义维护要点

- ff.flip_rst_bits({0}) 用于在把倒相“移过FF”时翻转对应位的复位语义，保持功能等价( /yosys/passes/opt/opt_ffinv.cc:86 , :175 ).
- LUT掩码变换：
  - Case 1在“Q作为某些输入参与的LUT”中对与 Q 相关的输入位集进行“地址位取反”的掩码重映射（ j ^ flip_mask ），相当于把“倒相作用”合并到LUT的真值表中( /yosys/passes/opt/opt_ffinv.cc:90-103 ).
  - Case 2在“D侧LUT”里对输出逐位取反，等效于把Q侧倒相前移到D侧逻辑( /yosys/passes/opt/opt_ffinv.cc:179-189 ).
- “1位buffer-LUT”判定：对于 WIDTH==1 的LUT，掩码为十进制 2 时等价于直通（具体真值编码与Yosys内部掩码编码一致），因此可以绕过并删除( /yosys/passes/opt/opt_ffinv.cc:104-113 , :189-193 ).
与opt目录其他pass的关系

- opt_dff 在触发器层面吸收 mux 、合并 CE/SRST ，而 opt_ffinv 专注于“倒相器搬运与合并到LUT”，两者目标互补；常见顺序是在表达式与多路器优化之后运行，再由 opt_clean 清理被绕过的单元。
- opt_expr 与 opt_demorgan 可能提前消除多余倒相器或将倒相推过布尔归约，减轻 opt_ffinv 后续工作或提高可匹配率。
- opt_lut / opt_lut_ins 可能调整LUT结构，使 opt_ffinv 更容易把倒相合并到LUT掩码中。
- opt_clean 清理被删除/绕过后的残留线网和单元，保证设计收敛。

## 执行时间的主要影响因素

- **候选FF数量**与过滤成本
  - 仅处理“无SR、有CLK、无ALOAD、宽度1”的内建FF；候选越多，遍历越多( /yosys/passes/opt/opt_ffinv.cc:215-225 ).
- 索引查询开销
  - 每个候选需要通过 ModIndex 查询 D/Q 的端口使用者，检查是否正好两个用户、是否为指定类型；这些查询和集合遍历成本随**连接数量**增长( /yosys/passes/opt/opt_ffinv.cc:41-67 , :70-84 , :132-145 , :149-173 ).
- LUT掩码处理复杂度
  - Case 1对以 Q 为输入的LUT进行掩码重映射，复杂度与LUT输入位数的指数相关：构造新掩码需要遍历 2^n 表项（ new_mask_builder.push_back(mask[j ^ flip_mask]) ），尽管常见场景中 n 很小。Case 2的全位取反是线性于掩码位数( /yosys/passes/opt/opt_ffinv.cc:98-103 , :181-189 ).
- SigMap 与连线更新
  - 比较 sig_a[i] 是否等于 ff.sig_q[0] 要走索引的 sigmap 统一化，更新FF端口、连接LUT或倒相器输出都需要创建/删除连接；大规模模块会积累这类固定开销( /yosys/passes/opt/opt_ffinv.cc:93-97 , :111-113 , :192-198 , :200 ).
- 选择范围与日志
  - 仅对已选择的模块和单元运行，合理缩小选择范围能显著降低时间。日志输出影响较小。
使用建议

- 典型序列：
  - yosys -p "read_verilog design.v; proc; opt; opt_expr; opt_lut; opt_ffinv; opt_clean; stat"
- 提升命中与效率：
  - 在 opt_expr/opt_demorgan 后运行，减少孤立倒相器，提高合并到LUT的机会。
  - 选择性地在含大量“1位FF + 较小LUT”的模块上运行，避免在宽LUT上频繁进行 2^n 掩码操作。
  - 结合 opt_clean 确保被绕过的 $not/$lut 及时清理，减少后续pass负担。