#pragma once
#include "YosysModel.h"
#include "DAG.h"
#include <unordered_map>
#include <vector>

// 从 Yosys 模块构建位到驱动/汇的映射，并生成 DAG

struct PinRef {
    std::string node_id;   // 节点 ID：CELL:<name> 或 PORT:<name>
    std::string pin_label; // 端口与位的展示，如 port[bit]
};

class DAGBuilder {
public:
    explicit DAGBuilder(const YDesign& d) : design_(d) {}
    DAG build_for_top(bool include_top_ports = true);

private:
    const YDesign& design_;
};

