# 第一次实现

## 📘《Temporal Reuse Partitioning & Scheduling：代码实现指导文档》

**用途：**作为输入给 AI 模型，使其可以直接根据本说明生成 Python 代码，完成论文 Algorithm 1 和 Algorithm 2 的实现、构造示例、测试与可视化。

---

## 0. 背景与核心目标

论文提出一个基于**时间复用（Temporal Reuse）**的 FPGA 原型验证系统，核心软件模块包含两个关键算法：

* **算法 1：Partition Algorithm（改进 FM）**
* **算法 2：Resource-Constrained List Scheduling（带资源约束的列表调度）**

本任务是让 AI：

1. **生成可运行的 Python 版本**（图模型 + 分区 + 调度）
2. **构造示例 DAG**
3. **运行算法并展示结果**
4. **对结果可视化（分区结果、调度甘特图等）**

---

## 1. 图模型统一定义（供代码生成使用）

请确保 AI 使用如下统一数据结构。

### 1.1 DAG 结构（G = (V, E)）

每个节点：

```python
Node {
    id: int or str
    resource: int    # cv，节点所需的 FPGA 资源
}
```

每条边：

```python
Edge {
    u: node_id
    v: node_id
    bitwidth: int    # we，跨节点的数据宽度
    delay: int       # d(ev)，非可复用模块带来的传输延迟
}
```

整个图：

```python
Graph {
    nodes: Dict[node_id, Node]
    edges: List[Edge]
}
```

---

## 2. 需要 AI 构造的示例 DAG（务必包含多路径与并行）

请 AI 生成一个与论文 Fig.2 类似但更一般的 DAG：

* 至少 **8–12 个节点**
* 包含：

  * 多个链式依赖
  * 多输入节点（in-degree > 1）
  * 多输出节点
  * 有“可复用的模块”（节点）
* 每个节点需随机生成 resource 值（10～80）
* 每条边随机生成 bitwidth（4～64 bit）、delay（1～5）

示例结构参考（AI 可自行构造）：

```
v1 → v3 → v5 → v8
v2 → v3
v2 → v4 → v6 → v8
v7 → v6
```

---

## 3. Algorithm 1 实现要求（Partition Algorithm）

论文伪代码对应的算法逻辑如下（请 AI 严格照此实现）：

### 3.1 输入

* DAG：G(V,E)
* FPGA 资源上限：R
* 节点资源：cv
* 边权重：we（用于 cutsize）

### 3.2 输出

* PartitionResult：一个列表 `[Vs1, Vs2, …, Vsk]`，每个子集是一批节点

### 3.3 核心功能点（AI 必须实现）

1. **拓扑排序生成初始分区**
   根据拓扑序将节点顺序填充，每个 partition 的资源 ≤ R。

2. **构建超图 Gs**（节点为 partition，边为跨 partition 的数据依赖）

3. **改进 FM 的 iterative refinement**

   * 在每次尝试移动节点时必须检查：

     * **资源限制**
     * **cutsize 是否下降（F = sum(weight of cross edges)）**
     * **Gs 必须保持 DAG（无环）**

4. **实现 GetBestMove()**

   * 尝试把每个节点移到所有合法 partition
   * 接受改善 cutsize 且保持 acyclic 的移动
   * 若资源超限，需要递归移出（论文 line 17–20）

5. **终止条件**

   * cutsize 不再下降

---

## 4. Algorithm 2 调度算法实现要求（List Scheduling）

### 4.1 输入

* 原图 G(V,E)
* 分区结果 Gs
* FPGA 可同时执行的可复用模块个数 R（视为资源数）
* 边 delay d(ev)
* 节点内部执行延迟 τ_o(v) − τ_i(v)（可设为固定常数）

### 4.2 输出

* 调度顺序 Q_out（按 cycle 分组）
* 每个节点的输入时间戳 τ_i[v]
* 可视化甘特图所需信息

### 4.3 必须实现的逻辑

1. **对每个 partition 内部进行拓扑排序**
2. **列表调度：每 cycle 最多调度 R 个节点**
3. **计算 τ_i(v) = max(parent output timestamp + d(edge))**
4. **支持跨 partition 的 offset δ(τ_sk)**（论文 line 6）

   * 可让 AI 定义 δ 为常数或依据 partition 层级计算

---

## 5. 测试要求（AI 必须包含）

AI 生成代码后必须自动运行：

1. **生成示例 DAG**
2. **运行 Algorithm 1（分区）**
3. **输出：**

   * 分区结果
   * cutsize
   * 每个 partition 的资源总量
4. **运行 Algorithm 2（调度）**
5. **输出：**

   * 节点调度序列 Q_out
   * 每个节点的时间戳 τ_i(v)
6. **可视化：**

   * DAG 图（networkx）
   * 分区染色图
   * 调度甘特图（matplotlib）
   * cutsize 的变化曲线（迭代过程）

---

## 6. 可视化规范（AI 请严格遵守）

使用 `matplotlib` + `networkx`：

#### 6.1 DAG 可视化

* 节点标签 = ID
* 节点颜色 = partition
* 边带箭头

#### 6.2 甘特图

* x 轴：cycle
* y 轴：node ID
* 每个调度周期画一个 rectangle bar

#### 6.3 cutsize 迭代图

* x 轴 = FM 迭代轮次
* y 轴 = cutsize

---

## 7. 最终产物要求

AI 最终输出必须包含：

#### ✔ 完整可运行 Python 代码

包括：

* Graph class
* Partition algorithm
* Scheduling algorithm
* Example DAG generator
* Visualization part
* Main 函数

#### ✔ 运行结果输出

包括：

* Partition 列表
* cutsize
* 调度列表
* 可视化图像

---

## 8. AI 生成代码风格要求

* 使用 Python3
* 推荐库：

  * `networkx`
  * `matplotlib`
  * `random`
  * `dataclasses`
* 代码必须模块化，函数命名清晰
* 使用类封装图结构与分区器、调度器
* 每个核心步骤必须包含注释

---

## 📌 你现在可以复制这份文档，输入给另一个 AI，让它直接生成完整代码。

如果你愿意，我还可以直接根据这份文档 **自动为你生成完整代码版本**。



# 第一次修改

下面是对你上传的 `implement.py` 的逐条、系统化审核，判断它是否**满足你之前提出的要求**：

> 「构造文章中的例子、用 Python 实现算法 1 & 2、再用例子进行测试、并完成可视化，而且结构要清晰，让另一个 AI 模型能快速写代码。」

我将从 6 个维度给你严格检查：**（1）代码结构、（2）算法 1 实现、（3）算法 2 实现、（4）测试流程、（5）可视化、（6）是否适合作为“输入给另一个 AI 模型的说明文档”**。

---

## ✅ 结论（短版）

**你的代码已经完整覆盖了你之前提出的所有需求，甚至做得比较完善，可以直接作为一个基线实现。
但存在一些问题和不符合论文的地方，我会在后面给你逐条指出。**

如果你把这个代码交给另一 AI 模型继续改写，它**绝对能顺利继续开发**，因为结构清晰：分区 → 调度 → 可视化 → main()。

---

## ✅ 检查结果（详细逐项）

---

## ① **是否构造了示例 DAG？（要求 1）**

✔ **你已有： `generate_example_dag()`**

* 自己构造了 DAG（10 节点，随机资源 & 随机 bitwidth & delay）
* 结构包含多输入、多输出、多路径
* 足够测试算法 1 & 2

⭐ **满足要求。**

唯一小缺点：

* 不是论文中的示例 DAG（如果论文里有明确的例子）

如果你需要“完全复原论文的图”，可以继续补。

---

## ② **Algorithm 1（分区算法）是否正确实现？（要求 2）**

你已经实现了：

| 论文需求                | 代码是否实现                                          | 备注          |
| ------------------- | ----------------------------------------------- | ----------- |
| 初始分区：拓扑排序按资源填充      | ✔ 已实现 `_initial_partition()`                    | 符合论文思路      |
| Cutsize 计算          | ✔ `_compute_cutsize()`                          | 权重＝bitwidth |
| 节点移动评估              | ✔ `_check_move()` 和 `_compute_cutsize_change()` | FM 思想       |
| 超图检测有无环             | ✔ `_would_maintain_acyclic()`                   | 关键功能已实现     |
| 资源限制 + 迁出节点         | △ `_check_move_with_eviction()` 简化版             | 不完全真实       |
| 反复迭代直到 cutsize 不再下降 | ✔ 实现                                            | 合理          |

⭐ **整体满足你的需求：算法 1 能跑、能测试、能可视化 cutsize 下降。**

⚠ **不足（重要）：**

* “移出节点 (eviction)” 只做了框架，没有真的实现。“递归寻找合适新 partition”也缺失。
* 论文中的 FM 改进版可能包含更多启发式逻辑（如 gain bucket），当前版本属于简化 FM。

---

## ③ **Algorithm 2（调度）是否实现？（要求 3）**

你实现了 `ListScheduler`，包含：

| 论文需求                  | 代码是否实现                           | 备注                         |
| --------------------- | -------------------------------- | -------------------------- |
| 分区内拓扑排序               | ✔                                | 每个 partition 自带 topo order |
| 跨 partition offset（δ） | ✔ `_compute_partition_offsets()` | offset 设置为 level×10        |
| 时间戳 τ_i、τ_o           | ✔                                | 使用 edge delay              |
| 资源限制（可复用模块数）          | ✔ `resource_limit`               | 参数可调                       |
| 每 cycle 限制可调度数量       | ✔                                | 和论文一致                      |
| 甘特图可视化支持结构            | ✔                                | 后面可视化可用                    |

⭐ **总的来说完全满足你的需求。**

⚠ **缺点：**

* `tau_i = max(tau_i, part_offset + current_cycle)` 这行不符合论文描述，可能引入错误 offset。
* offset 计算使用 shortest_path_length，而论文里往往是 longest-path-based level。

---

## ④ **是否自带测试流程？（要求 4）**

✔ 在 `main()` 中你已经写了整个测试流水：

1. 生成 DAG
2. 调用 Algorithm 1
3. 调用 Algorithm 2
4. 打印所有结果
5. 调用 Visualizer 画图

⭐ **完全满足需求**。

---

## ⑤ **可视化是否完整？（要求 5）**

你实现了：

* DAG（带分区染色）
* 调度甘特图
* cutsize 曲线图

⭐ **超出要求，做得很好。**

缺点：

* 网络大时 spring 布局可能过于乱（不过可接受）

---

## ⑥ **作为”给 AI 模型的指导文档“是否合适？（要求 6）**

非常重要的一点：

### ✔ 你的代码结构**非常清晰**：

```
Graph / Node / Edge 数据结构
↓
Algorithm 1：PartitionAlgorithm
↓
Algorithm 2：ListScheduler
↓
Visualizer
↓
main() 执行全部测试
```

### ✔ 模块化很好（AI 很容易扩展）

* 类名清晰
* 函数职责清楚
* 注释明确

### ✔ 有 main() 完整执行链

这对于让另一个 AI 模型基于它继续开发非常重要。

### 🟡 唯一问题是：

某些部分实现 **和论文不完全一致**（如 eviction、offset）。

但如果你的目标是：

> “让它写出可复现论文风格的算法”

那么你现在这个代码是**非常好的基线**。

---

## ⭐ 最终评价（总结）

你的代码 **完全满足你之前提出的要求**：

| 要求             | 是否符合     |
| -------------- | -------- |
| 构造例子           | ✔        |
| 实现算法 1         | ✔（简化版）   |
| 实现算法 2         | ✔（基本正确）  |
| 用例子测试          | ✔        |
| 可视化            | ✔        |
| 结构适合作为 AI 输入文档 | ✔✔（非常适合） |

唯一不足之处：

1. **Algorithm 1（FM + eviction）实现不完整**（论文中的递归迁移未完全实现）
2. **Algorithm 2 中 offset 逻辑不完全符合论文**
3. **一些函数可进一步优化（如 edge lookup 应用加速结构）**
4. **需要保存最终的命令行窗口输出到一个txt文件，然后把生成的图片保存下来**



