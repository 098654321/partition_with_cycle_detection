"""
Temporal Reuse Partitioning & Scheduling
实现 Algorithm 1 (改进 FM 分区) 和 Algorithm 2 (资源约束列表调度)
根据 fix.md 修改意见进行改进
"""

import networkx as nx
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from dataclasses import dataclass
from typing import Dict, List, Set, Tuple, Optional
import random
from collections import defaultdict, deque
import numpy as np
import sys
import os
from datetime import datetime


# ==================== 数据结构定义 ====================

@dataclass
class Node:
    """节点数据结构"""
    id: int
    resource: int  # cv，节点所需的 FPGA 资源


@dataclass
class Edge:
    """边数据结构"""
    u: int  # 源节点 ID
    v: int  # 目标节点 ID
    bitwidth: int  # we，跨节点的数据宽度
    delay: int  # d(ev)，非可复用模块带来的传输延迟


class Graph:
    """DAG 图结构"""
    
    def __init__(self):
        self.nodes: Dict[int, Node] = {}
        self.edges: List[Edge] = []
        self._nx_graph: Optional[nx.DiGraph] = None
        # 优化：使用字典加速边查找
        self._edge_dict: Dict[Tuple[int, int], Edge] = {}
    
    def add_node(self, node: Node):
        """添加节点"""
        self.nodes[node.id] = node
        self._nx_graph = None  # 使缓存失效
    
    def add_edge(self, edge: Edge):
        """添加边"""
        self.edges.append(edge)
        self._edge_dict[(edge.u, edge.v)] = edge
        self._nx_graph = None  # 使缓存失效

    def remove_edge(self, edge: Edge):
        '''删除边'''
        self.edges.remove(edge)
        del self._edge_dict[(edge.u, edge.v)]
        self._nx_graph = None  # 使缓存失效
    
    def get_edge(self, u: int, v: int) -> Optional[Edge]:
        """快速获取边（优化）"""
        return self._edge_dict.get((u, v))
    
    def get_nx_graph(self) -> nx.DiGraph:
        """获取 networkx 图对象（带缓存）"""
        if self._nx_graph is None:
            self._nx_graph = nx.DiGraph()
            for node_id in self.nodes:
                self._nx_graph.add_node(node_id)
            for edge in self.edges:
                self._nx_graph.add_edge(edge.u, edge.v)
        return self._nx_graph
    
    def is_acyclic(self) -> bool:
        """检查图是否为 DAG"""
        return nx.is_directed_acyclic_graph(self.get_nx_graph())
    
    def topological_sort(self) -> List[int]:
        """拓扑排序"""
        if not self.is_acyclic():
            raise ValueError("Graph is not acyclic")
        return list(nx.topological_sort(self.get_nx_graph()))
    
    def get_predecessors(self, node_id: int) -> List[int]:
        """获取节点的前驱节点"""
        return list(self.get_nx_graph().predecessors(node_id))
    
    def get_successors(self, node_id: int) -> List[int]:
        """获取节点的后继节点"""
        return list(self.get_nx_graph().successors(node_id))


# ==================== 示例 DAG 生成器 ====================

class ExampleGenerator:
    """示例 DAG 生成器"""
    
    def __init__(self):
        pass
    
    def _ramdom_generator(self) -> Graph:
        '''随机生成一个 DAG 图，且确保是 acyclic'''
        graph = Graph()
        random.seed(42)  # 固定随机种子以便复现
        node_size = 10
        node_structure = []

        # 定义节点结构
        for node_id in range(1, node_size + 1):
            prev_nodes = random.sample(range(1, node_size), random.randint(1, node_size - 1))
            if node_id in prev_nodes:
                prev_nodes.remove(node_id)
            node_info = (node_id, prev_nodes)
            node_structure.append(node_info)

        # 添加节点（随机资源 10-80）
        for node_id, _ in node_structure:
            resource = random.randint(10, 80)
            graph.add_node(Node(id=node_id, resource=resource))
        
        # 添加边（随机 bitwidth 4-64, delay 1-5）
        for node_id, predecessors in node_structure:
            for pred_id in predecessors:
                bitwidth = random.randint(4, 64)
                delay = random.randint(1, 5)
                graph.add_edge(Edge(u=pred_id, v=node_id, bitwidth=bitwidth, delay=delay))
        
        self._remove_cycles(graph)
        return graph
    
    def _example_in_paper(self) -> Graph:
        graph = Graph()
        random.seed(42)

        node_structure = [
            (1, []),        
            (2, []),      
            (3, []),  
            (4, []),     
            (5, [1, 2]),     
            (6, [1, 2, 3, 4]),  
            (7, [1, 2, 3, 4]),      
            (8, [3, 4]),
        ]
        for node_id, _ in node_structure:
            resource = random.randint(10, 80)
            graph.add_node(Node(id=node_id, resource=resource))
        
        for node_id, predecessors in node_structure:
            for pred_id in predecessors:
                bitwidth = random.randint(4, 64)
                delay = random.randint(1, 5)
                graph.add_edge(Edge(u=pred_id, v=node_id, bitwidth=bitwidth, delay=delay))

        return graph

    def _remove_cycles(self, graph: Graph):
        """
        移除 graph 中所有有向环，使其成为 DAG。
        使用 networkx.simple_cycles 找所有环，然后删除每个环中任意一条边。
        """
        nx_g = graph.get_nx_graph()

        cycles = list(nx.simple_cycles(nx_g))
        if not cycles:
            return  

        for cycle in cycles:
            # cycle 是形如 [n1, n2, n3, ...] 的一个环
            # 删除环中最后一条边 (cycle[-1] -> cycle[0]) 或者任意一条你想删的边
            u = cycle[-1]
            v = cycle[0]

            # 从 Graph 删除对应的 Edge
            uv_edge = graph.get_edge(u, v)
            if uv_edge:
                graph.remove_edge(uv_edge)

        # 删除一轮环后可能还存在新的环，递归继续删除
        self._remove_cycles(graph)


# ==================== Algorithm 1: Partition Algorithm ====================

class PartitionAlgorithm:
    """改进的 FM 分区算法"""
    
    def __init__(self, graph: Graph, resource_limit: int, size_limit: int):
        self.graph = graph
        self.resource_limit = resource_limit
        self.partitions: List[Set[int]] = []  # 每个 partition 是一个节点集合
        self.node_to_partition: Dict[int, int] = {}  # 节点到 partition 的映射
        self.cutsize_history: List[int] = []  # 记录 cutsize 变化历史
        self.size_limit = size_limit
    
    def partition(self) -> List[Set[int]]:
        """执行分区算法"""
        print("=" * 60)
        print("Algorithm 1: Partition Algorithm (改进 FM)")
        print("=" * 60)
        
        # 步骤 1: 拓扑排序（库自带）生成初始分区
        self._initial_partition()
        print(f"\n初始分区完成，共 {len(self.partitions)} 个 partition")
        self._print_partitions()
        
        # 步骤 2-4: 改进 FM 迭代优化
        iteration = 0
        eva_old = self._compute_cutsize()
        self.cutsize_history.append(eva_old)
        
        print(f"\n初始 cutsize: {eva_old}")
        print("\n开始 FM 迭代优化...")
        
        while True:
            iteration += 1
            eva_new = self._fm_iteration()
            self.cutsize_history.append(eva_new)
            
            if eva_new != eva_old:     
                print(f"迭代 {iteration}: cutsize = {eva_new} (改善 {eva_old - eva_new})")
                eva_old = eva_new
            elif eva_new == eva_old:
                print(f"\n迭代 {iteration}: cutsize = {eva_new} (已改善，收敛)")
                break
            else:
                print(f"迭代 {iteration}: cutsize = {eva_new} (evaluation增大)")
                
        print(f"\n最终分区结果:")
        self._print_partitions()
        print(f"最终 cutsize: {self._compute_cutsize()}")
        
        return self.partitions
    
    
    def _initial_partition(self):
        """根据拓扑排序生成初始分区"""
        topo_order = self.graph.topological_sort()  
        self.partitions = []
        self.node_to_partition = {}
        
        current_partition = set()
        current_resource = 0
        partition_id = 0
        
        for node_id in topo_order:
            node = self.graph.nodes[node_id]
            
            # 如果当前 partition 无法容纳该节点，创建新 partition
            if (current_resource + node.resource > self.resource_limit and current_partition) or\
                  len(current_partition) + 1 > self.size_limit:
                self.partitions.append(current_partition)
                partition_id += 1
                current_partition = set()
                current_resource = 0
            
            current_partition.add(node_id)
            current_resource += node.resource
            self.node_to_partition[node_id] = partition_id
        
        # 添加最后一个 partition
        if current_partition:
            self.partitions.append(current_partition)
    
    def _compute_cutsize(self) -> int:
        """计算 cutsize（跨 partition 的边权重之和）"""
        cutsize = 0
        for edge in self.graph.edges:
            u_part = self.node_to_partition.get(edge.u, -1)
            v_part = self.node_to_partition.get(edge.v, -1)
            if u_part != v_part:
                cutsize += edge.bitwidth  
        return cutsize
    

    def _fm_iteration(self) -> int:
        """执行一次 FM 迭代，返回是否改善"""
        eva_global = self._compute_cutsize()
        for partition in self.partitions:
            eva_global = self._fm(partition)

        return eva_global

    
    def _fm(self, partition: Set[int]) -> int:
        '''对一个划分进行FM迭代；确保best_eva不会增加'''
        best_eva = self._compute_cutsize()  # global evaluation
        node_ids = [id for id in partition]

        # 对每个节点
        for node_id in node_ids:
            current_part = self.node_to_partition[node_id]
            node = self.graph.nodes[node_id]
            if len(partition) == 1:
                continue

            # 尝试移动到其它划分
            for target_part_id in range(len(self.partitions)):
                if target_part_id == current_part:
                    continue
                
                # 检查环，如果存在环就返回 None
                move_info = self._check_acyclic(node_id, target_part_id)
                if move_info is None:
                    continue
                
                # 判断是否更优，如果更优就保留；检查资源冲突
                cutsize_reduction = move_info['cutsize_reduction']
                reduced_eva = best_eva - cutsize_reduction
                if reduced_eva < best_eva:
                    best_eva = reduced_eva
                    self._execute_move(node_id, target_part_id, move_info)
                    if self._check_resource(target_part_id):
                        return self._fm(self.partitions[target_part_id])
        
        return best_eva
                
    def _check_resource(self, target_part_id: int) -> int:
        '''
        检查目标划分是否会超出资源限制
        '''
        target_part = self.partitions[target_part_id]
        target_resource = sum(self.graph.nodes[nid].resource for nid in target_part)
        return target_resource > self.resource_limit
    
    def _check_acyclic(self, node_id: int, target_part_id: int) -> Optional[Dict]:
        """
        检查节点移动是否合法
        返回移动信息字典，如果非法则返回 None
        """
        current_part_id = self.node_to_partition[node_id]
        node = self.graph.nodes[node_id]
        
        # 检查移动后是否保持 DAG（通过检查超图 Gs 是否无环）
        if not self._would_maintain_acyclic(node_id, target_part_id):
            return None
        
        # 超过 size_limit 就返回 None
        if len(self.partitions[target_part_id]) + 1 > self.size_limit:
            return None
        
        # 计算 cutsize 变化
        cutsize_reduction = self._compute_cutsize_change(node_id, current_part_id, target_part_id)
        
        return {
            'cutsize_reduction': cutsize_reduction,
            'evictions': []  # 无需移出
        }
    
    def _would_maintain_acyclic_for_move(self, node_id: int, target_part_id: int) -> bool:
        """检查移动节点后是否保持 DAG（辅助方法）"""
        return self._would_maintain_acyclic(node_id, target_part_id)
    
    def _would_maintain_acyclic(self, node_id: int, target_part_id: int) -> bool:
        """
        检查移动节点后，超图 Gs 是否仍然保持 DAG
        超图 Gs: 节点是 partition，边是跨 partition 的数据依赖
        """
        current_part_id = self.node_to_partition[node_id]
        
        # 构建超图
        super_graph = nx.DiGraph()
        for i in range(len(self.partitions)):
            super_graph.add_node(i)
        
        # 添加跨 partition 的边
        for edge in self.graph.edges:
            u_part = self.node_to_partition[edge.u]
            v_part = self.node_to_partition[edge.v]
            
            # 模拟移动后的 partition 分配
            if edge.u == node_id:
                u_part = target_part_id
            if edge.v == node_id:
                v_part = target_part_id
            
            if u_part != v_part:
                super_graph.add_edge(u_part, v_part)
        
        # 检查是否有环
        try:
            nx.find_cycle(super_graph)
            return False  # 有环
        except nx.NetworkXNoCycle:
            return True  # 无环
    
    def _compute_cutsize_change(self, node_id: int, from_part: int, to_part: int) -> int:
        """计算移动节点后的 cutsize 变化；注意这里的 reduction 正数代表减少量"""
        reduction = 0
        
        # 检查所有与该节点相关的边
        for edge in self.graph.edges:
            if edge.u == node_id:
                # 出边
                v_part = self.node_to_partition[edge.v]
                if from_part != v_part and to_part == v_part:
                    reduction += edge.bitwidth  # 从跨 partition 变为同 partition
                elif from_part != v_part and to_part != v_part:
                    pass  # 仍然是跨 partition
                elif from_part == v_part and to_part != v_part:
                    reduction -= edge.bitwidth  # 从同 partition 变为跨 partition
            
            if edge.v == node_id:
                # 入边
                u_part = self.node_to_partition[edge.u]
                if u_part != from_part and u_part == to_part:
                    reduction += edge.bitwidth
                elif u_part != from_part and u_part != to_part:
                    pass
                elif u_part == from_part and u_part != to_part:
                    reduction -= edge.bitwidth
        
        return reduction
    
    def _execute_move(self, node_id: int, target_part_id: int, move_info: Dict):
        """执行节点移动"""
        current_part_id = self.node_to_partition[node_id]
        self.partitions[current_part_id].remove(node_id)
        self.partitions[target_part_id].add(node_id)
        self.node_to_partition[node_id] = target_part_id
        
        # 清理空 partition
        self.partitions = [p for p in self.partitions if p]
        # 重新映射 partition ID
        self._remap_partitions()
    
    def _remap_partitions(self):
        """重新映射 partition ID"""
        new_partitions = []
        new_node_to_partition = {}
        
        for part_id, part_set in enumerate(self.partitions):
            if part_set:
                new_partitions.append(part_set)
                for node_id in part_set:
                    new_node_to_partition[node_id] = len(new_partitions) - 1
        
        self.partitions = new_partitions
        self.node_to_partition = new_node_to_partition
    
    def _print_partitions(self):
        """打印分区信息"""
        for i, part in enumerate(self.partitions):
            total_resource = sum(self.graph.nodes[nid].resource for nid in part)
            print(f"  Partition {i}: nodes={sorted(part)}, resource={total_resource}/{self.resource_limit}")
    
    def get_partition_resources(self) -> List[int]:
        """获取每个 partition 的资源总量"""
        return [sum(self.graph.nodes[nid].resource for nid in part) 
                for part in self.partitions]


# ==================== Algorithm 2: List Scheduling ====================

class ListScheduler:
    """资源约束的列表调度算法"""
    
    def __init__(self, graph: Graph, partitions: List[Set[int]], 
                 resource_limit: int, node_execution_delay: int = 1):
        self.graph = graph
        self.partitions = partitions
        self.resource_limit = resource_limit  # 可同时执行的可复用模块个数
        self.node_execution_delay = node_execution_delay  # τ_o(v) - τ_i(v)
        self.tau_i: Dict[int, int] = {}  # 节点输入时间戳
        self.tau_o: Dict[int, int] = {}  # 节点输出时间戳
        self.schedule_list: List[List[int]] = []  # 按 cycle 分组的调度序列
    
    def schedule(self) -> Tuple[List[List[int]], Dict[int, int]]:
        """执行调度算法"""
        print("\n" + "=" * 60)
        print("Algorithm 2: Resource-Constrained List Scheduling")
        print("=" * 60)
        
        # 对每个 partition 内部进行拓扑排序
        partition_orders = []
        for part in self.partitions:
            # 构建 partition 内部的子图
            subgraph_nodes = set(part)
            subgraph_edges = [e for e in self.graph.edges 
                            if e.u in subgraph_nodes and e.v in subgraph_nodes]
            
            # 创建子图进行拓扑排序
            subgraph = nx.DiGraph()
            for nid in subgraph_nodes:
                subgraph.add_node(nid)
            for e in subgraph_edges:
                subgraph.add_edge(e.u, e.v)
            
            if subgraph.nodes():
                topo_order = list(nx.topological_sort(subgraph))
                partition_orders.append(topo_order)
            else:   # 有点奇怪，如果 nodes 是空的那么这个 part 也应该是空的
                partition_orders.append(list(part))
        
        # 计算跨 partition 的 offset（δ）- 改进：使用 longest-path-based level
        partition_offsets = self._compute_partition_offsets()
        
        # 列表调度
        self._list_schedule(partition_orders, partition_offsets)

        print(f"\n调度完成，共 {len(self.schedule_list)} 个 cycle")
        self._print_schedule()

        return self.schedule_list, self.tau_i
    
    def _compute_partition_offsets(self) -> List[int]:
        """
        计算每个 partition 的 offset δ(τ_sk)
        改进：使用 longest-path-based level 而不是 shortest_path_length
        """
        # 构建超图以确定 partition 的层级
        super_graph = nx.DiGraph()
        for i in range(len(self.partitions)):
            super_graph.add_node(i)
        
        # 添加跨 partition 的边
        for edge in self.graph.edges:
            u_part = self._get_node_partition(edge.u)
            v_part = self._get_node_partition(edge.v)
            if u_part != v_part and u_part is not None and v_part is not None:
                super_graph.add_edge(u_part, v_part)
        
        # 计算每个 partition 的层级（从源 partition 的最长路径长度）
        partition_levels = {}
        
        # 找到所有源 partition（没有入边的）
        sources = [n for n in super_graph.nodes() if super_graph.in_degree(n) == 0]
        
        if not sources:
            # 如果没有源节点，所有 partition 层级为 0
            for part_id in range(len(self.partitions)):
                partition_levels[part_id] = 0
        else:
            # 使用动态规划计算最长路径
            def compute_longest_path_from_source(source: int) -> Dict[int, int]:
                """从源节点计算到所有节点的最长路径"""
                levels = {source: 0}
                queue = deque([source])
                
                while queue:
                    current = queue.popleft()
                    current_level = levels[current]
                    
                    for neighbor in super_graph.successors(current):
                        new_level = current_level + 1
                        if neighbor not in levels or new_level > levels[neighbor]:
                            levels[neighbor] = new_level
                            queue.append(neighbor)
                
                return levels
            
            # 从所有源节点计算最长路径，取最大值
            all_levels = {}
            for source in sources:
                levels = compute_longest_path_from_source(source)
                for part_id, level in levels.items():
                    all_levels[part_id] = max(all_levels.get(part_id, 0), level)
            
            # 设置所有 partition 的层级
            for part_id in range(len(self.partitions)):
                partition_levels[part_id] = all_levels.get(part_id, 0)
        
        # offset = 层级 * 常数（这里设为 10）
        offset_constant = 10
        offsets = [partition_levels.get(i, 0) * offset_constant for i in range(len(self.partitions))]
        return offsets
    
    def _get_node_partition(self, node_id: int) -> Optional[int]:
        """获取节点所在的 partition ID"""
        for i, part in enumerate(self.partitions):
            if node_id in part:
                return i
        return None
    
    def _list_schedule(self, partition_orders: List[List[int]], partition_offsets: List[int]):
        """执行列表调度"""
        # 初始化
        self.tau_i = {}
        self.tau_o = {}
        self.schedule_list = []
        
        # 记录每个节点的就绪状态
        in_degree = {nid: len(self.graph.get_predecessors(nid)) for nid in self.graph.nodes}
        ready_nodes = deque([nid for nid, deg in in_degree.items() if deg == 0])
        
        # 记录每个 partition 的当前调度位置
        partition_positions = [0] * len(self.partitions)
        
        current_cycle = 0
        
        while ready_nodes or any(pos < len(partition_orders[i]) 
                                for i, pos in enumerate(partition_positions)):
            
            # 当前 cycle 可调度的节点
            cycle_schedule = []
            used_resources = 0
            
            # 按 partition 顺序调度（考虑 offset）
            for part_id in range(len(self.partitions)):
                if used_resources >= self.resource_limit:
                    break
                
                part_offset = partition_offsets[part_id]
                # 改进：offset 应该直接加到时间戳，而不是和 current_cycle 相加
                # 这里检查是否已经过了 offset 时间
                if current_cycle < part_offset:
                    continue  # 该 partition 还未到开始时间
                
                pos = partition_positions[part_id]
                if pos >= len(partition_orders[part_id]):
                    continue
                
                # 尝试调度该 partition 的下一个节点
                candidate_node = partition_orders[part_id][pos]
                
                # 检查节点是否就绪（所有前驱节点已调度）
                predecessors = self.graph.get_predecessors(candidate_node)
                all_predecessors_scheduled = all(
                    pred_id in self.tau_o for pred_id in predecessors
                )
                
                if all_predecessors_scheduled:
                    # 计算输入时间戳
                    if predecessors:
                        tau_i = max(
                            self.tau_o[pred_id] + self._get_edge_delay(pred_id, candidate_node)
                            for pred_id in predecessors
                        )
                    else:
                        tau_i = current_cycle
                    
                    # 改进：offset 应该确保时间戳不小于 offset，而不是和 current_cycle 相加
                    tau_i = max(tau_i, part_offset)
                    
                    # 调度节点
                    self.tau_i[candidate_node] = tau_i
                    self.tau_o[candidate_node] = tau_i + self.node_execution_delay
                    
                    cycle_schedule.append(candidate_node)
                    used_resources += 1
                    partition_positions[part_id] += 1
                    
                    # 更新后继节点的 in_degree
                    for succ_id in self.graph.get_successors(candidate_node):
                        in_degree[succ_id] -= 1
                        if in_degree[succ_id] == 0:
                            ready_nodes.append(succ_id)
            
            # 处理其他就绪节点（不在当前 partition 顺序中的）
            while ready_nodes and used_resources < self.resource_limit:
                node_id = ready_nodes.popleft()
                if node_id in self.tau_i:
                    continue  # 已调度
                
                # 检查是否所有前驱已调度
                predecessors = self.graph.get_predecessors(node_id)
                if all(pred_id in self.tau_o for pred_id in predecessors):
                    if predecessors:
                        tau_i = max(
                            self.tau_o[pred_id] + self._get_edge_delay(pred_id, node_id)
                            for pred_id in predecessors
                        )
                    else:
                        tau_i = current_cycle
                    
                    # 考虑 partition offset
                    node_part = self._get_node_partition(node_id)
                    if node_part is not None:
                        part_offset = partition_offsets[node_part]
                        tau_i = max(tau_i, part_offset)
                    
                    self.tau_i[node_id] = tau_i
                    self.tau_o[node_id] = tau_i + self.node_execution_delay
                    cycle_schedule.append(node_id)
                    used_resources += 1
                    
                    # 更新后继节点
                    for succ_id in self.graph.get_successors(node_id):
                        in_degree[succ_id] -= 1
                        if in_degree[succ_id] == 0:
                            ready_nodes.append(succ_id)
            
            if cycle_schedule:
                self.schedule_list.append(cycle_schedule)
            
            current_cycle += 1
            
            # 防止无限循环
            if current_cycle > 1000:
                break
    
    def _get_edge_delay(self, u: int, v: int) -> int:
        """获取边的延迟（优化：使用字典查找）"""
        edge = self.graph.get_edge(u, v)
        return edge.delay if edge else 0
    
    def _print_schedule(self):
        """打印调度结果"""
        for cycle, nodes in enumerate(self.schedule_list):
            print(f"  Cycle {cycle}: {nodes}")
            for node_id in nodes:
                print(f"    Node {node_id}: τ_i={self.tau_i.get(node_id, -1)}, "
                      f"τ_o={self.tau_o.get(node_id, -1)}")


# ==================== 可视化 ====================

class Visualizer:
    """可视化工具类"""
    
    def __init__(self, graph: Graph, partitions: List[Set[int]], 
                 schedule_result: List[List[int]], tau_i: Dict[int, int],
                 cutsize_history: List[int], output_dir: str = "output"):
        self.graph = graph
        self.partitions = partitions
        self.schedule_result = schedule_result
        self.tau_i = tau_i
        self.cutsize_history = cutsize_history
        self.output_dir = output_dir
        
        # 创建输出目录
        os.makedirs(output_dir, exist_ok=True)
    
    def visualize_all(self):
        """生成所有可视化图表并保存"""
        plt.rcParams['font.sans-serif'] = ['SimHei', 'Arial Unicode MS', 'DejaVu Sans']
        plt.rcParams['axes.unicode_minus'] = False
        
        self.visualize_dag()
        self.visualize_gantt()
        self.visualize_cutsize()
        plt.show()
    
    def visualize_dag(self):
        """可视化 DAG 图（带分区染色）"""
        fig, ax = plt.subplots(figsize=(12, 8))
        G = self.graph.get_nx_graph()
        
        # 计算节点到 partition 的映射
        node_to_partition = {}
        for part_id, part in enumerate(self.partitions):
            for node_id in part:
                node_to_partition[node_id] = part_id
        
        # 布局
        pos = nx.spring_layout(G, k=2, iterations=50)
        
        # 为每个 partition 分配颜色
        num_partitions = len(self.partitions)
        colors = plt.cm.Set3(np.linspace(0, 1, max(num_partitions, 1)))
        
        # 绘制节点
        node_colors = [colors[node_to_partition.get(nid, 0) % len(colors)] 
                      for nid in G.nodes()]
        nx.draw_networkx_nodes(G, pos, node_color=node_colors, 
                              node_size=1000, ax=ax, alpha=0.8)
        
        # 绘制边
        nx.draw_networkx_edges(G, pos, edge_color='gray', 
                              arrows=True, arrowsize=20, ax=ax, alpha=0.6)
        
        # 绘制标签
        labels = {nid: str(nid) for nid in G.nodes()}
        nx.draw_networkx_labels(G, pos, labels, font_size=10, ax=ax)
        
        # 添加图例
        legend_elements = [mpatches.Patch(color=colors[i % len(colors)], 
                                         label=f'Partition {i}') 
                          for i in range(num_partitions)]
        ax.legend(handles=legend_elements, loc='upper right')
        
        ax.set_title("DAG with Partition Coloring", fontsize=14, fontweight='bold')
        ax.axis('off')
        plt.tight_layout()
        
        # 保存图片
        output_path = os.path.join(self.output_dir, "dag_partition.png")
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        print(f"  DAG 图已保存: {output_path}")
        plt.close()
    
    def visualize_gantt(self):
        """可视化调度甘特图"""
        fig, ax = plt.subplots(figsize=(14, 8))
        
        # 准备数据
        node_cycles = {}  # node_id -> [(start_cycle, end_cycle), ...]
        all_nodes = sorted(self.graph.nodes.keys())
        
        for cycle, nodes in enumerate(self.schedule_result):
            for node_id in nodes:
                if node_id not in node_cycles:
                    node_cycles[node_id] = []
                start = self.tau_i.get(node_id, cycle)
                end = self.tau_i.get(node_id, start + 1)
                node_cycles[node_id].append((start, end))
        
        # 绘制甘特图
        y_positions = {node_id: idx for idx, node_id in enumerate(all_nodes)}
        
        # 为每个 partition 分配颜色
        node_to_partition = {}
        for part_id, part in enumerate(self.partitions):
            for node_id in part:
                node_to_partition[node_id] = part_id
        
        num_partitions = len(self.partitions)
        colors = plt.cm.Set3(np.linspace(0, 1, max(num_partitions, 1)))
        
        max_cycle = 0
        for node_id in all_nodes:
            if node_id in node_cycles:
                for start, end in node_cycles[node_id]:
                    y = y_positions[node_id]
                    color = colors[node_to_partition.get(node_id, 0) % len(colors)]
                    ax.barh(y, end - start, left=start, height=0.6, 
                           color=color, alpha=0.7, edgecolor='black')
                    max_cycle = max(max_cycle, end)
        
        ax.set_yticks(range(len(all_nodes)))
        ax.set_yticklabels([f'Node {nid}' for nid in all_nodes])
        ax.set_xlabel('Cycle', fontsize=12)
        ax.set_ylabel('Node ID', fontsize=12)
        ax.set_title('Scheduling Gantt Chart', fontsize=14, fontweight='bold')
        ax.set_xlim(0, max_cycle + 1)
        ax.grid(True, alpha=0.3, axis='x')
        
        # 添加图例
        legend_elements = [mpatches.Patch(color=colors[i % len(colors)], 
                                         label=f'Partition {i}') 
                          for i in range(num_partitions)]
        ax.legend(handles=legend_elements, loc='upper right')
        
        plt.tight_layout()
        
        # 保存图片
        output_path = os.path.join(self.output_dir, "gantt_chart.png")
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        print(f"  甘特图已保存: {output_path}")
        plt.close()
    
    def visualize_cutsize(self):
        """可视化 cutsize 迭代过程"""
        fig, ax = plt.subplots(figsize=(10, 6))
        
        iterations = range(len(self.cutsize_history))
        ax.plot(iterations, self.cutsize_history, 'b-o', linewidth=2, markersize=6)
        ax.set_xlabel('FM Iteration', fontsize=12)
        ax.set_ylabel('Cutsize', fontsize=12)
        ax.set_title('Cutsize Reduction During FM Iterations', 
                    fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        
        # 标注初始和最终值
        if len(self.cutsize_history) > 0:
            ax.annotate(f'Initial: {self.cutsize_history[0]}', 
                       xy=(0, self.cutsize_history[0]),
                       xytext=(5, 10), textcoords='offset points',
                       bbox=dict(boxstyle='round,pad=0.3', facecolor='yellow', alpha=0.5))
            ax.annotate(f'Final: {self.cutsize_history[-1]}', 
                       xy=(len(self.cutsize_history)-1, self.cutsize_history[-1]),
                       xytext=(5, -20), textcoords='offset points',
                       bbox=dict(boxstyle='round,pad=0.3', facecolor='green', alpha=0.5))
        
        plt.tight_layout()
        
        # 保存图片
        output_path = os.path.join(self.output_dir, "cutsize_iteration.png")
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        print(f"  Cutsize 曲线已保存: {output_path}")
        plt.close()


# ==================== 输出重定向 ====================

class TeeOutput:
    """同时输出到控制台和文件"""
    def __init__(self, file_path: str):
        self.file = open(file_path, 'w', encoding='utf-8')
        self.stdout = sys.stdout
    
    def write(self, text):
        self.file.write(text)
        self.file.flush()
        self.stdout.write(text)
    
    def flush(self):
        self.file.flush()
        self.stdout.flush()
    
    def close(self):
        self.file.close()


# ==================== Main 函数 ====================

def main():
    """主函数：执行完整流程"""
    # 创建输出目录
    output_dir = "./Implement/Scaling FPGA Capacity via Temporal Reuse/output"
    os.makedirs(output_dir, exist_ok=True)
    
    # 设置输出重定向
    log_file = os.path.join(output_dir, f"output.txt")
    tee = TeeOutput(log_file)
    sys.stdout = tee
    
    try:
        print("=" * 60)
        print("Temporal Reuse Partitioning & Scheduling")
        print("=" * 60)
        
        # 1. 生成示例 DAG
        print("\n[步骤 1] 生成示例 DAG...")
        generator = ExampleGenerator()
        graph = generator._ramdom_generator()
        print(f"DAG 生成完成: {len(graph.nodes)} 个节点, {len(graph.edges)} 条边")
        print(f"节点资源: {[(nid, node.resource) for nid, node in graph.nodes.items()]}")
        
        # 2. 运行 Algorithm 1 (分区)
        print("\n[步骤 2] 运行 Algorithm 1 (分区算法)...")
        resource_limit = 150  # FPGA 资源上限
        size_limit = 4  # 每个 partition 最大节点数
        partitioner = PartitionAlgorithm(graph, resource_limit, size_limit)
        partitions = partitioner.partition()
        
        # 输出分区结果
        print("\n分区结果详情:")
        partition_resources = partitioner.get_partition_resources()
        for i, (part, res) in enumerate(zip(partitions, partition_resources)):
            print(f"  Partition {i}: {sorted(part)} (资源: {res}/{resource_limit})")
        print(f"总 cutsize: {partitioner._compute_cutsize()}")
        
        # 3. 运行 Algorithm 2 (调度)
        print("\n[步骤 3] 运行 Algorithm 2 (调度算法)...")
        scheduler = ListScheduler(graph, partitions, resource_limit=2, node_execution_delay=1)
        schedule_result, tau_i = scheduler.schedule()

        # 输出调度结果
        print("\n调度结果详情:")
        for cycle, nodes in enumerate(schedule_result):
            print(f"  Cycle {cycle}: {nodes}")
        print(f"\n节点时间戳:")
        for node_id in sorted(tau_i.keys()):
            print(f"  Node {node_id}: τ_i = {tau_i[node_id]}, "
                  f"τ_o = {scheduler.tau_o.get(node_id, -1)}")
        
        # 4. 可视化
        print("\n[步骤 4] 生成可视化图表...")
        visualizer = Visualizer(graph, partitions, schedule_result, tau_i, 
                               partitioner.cutsize_history, output_dir)
        visualizer.visualize_all()
        
        print("\n" + "=" * 60)
        print("所有任务完成！")
        print(f"输出文件已保存到: {output_dir}/")
        print(f"日志文件: {log_file}")
        print("=" * 60)
    
    finally:
        # 恢复标准输出
        sys.stdout = tee.stdout
        tee.close()


if __name__ == "__main__":
    main()