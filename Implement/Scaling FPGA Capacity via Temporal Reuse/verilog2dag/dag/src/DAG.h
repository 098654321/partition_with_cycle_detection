#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// 通用 DAG 结构，节点代表模块或端口，边代表数据流

struct DAGNode {
    std::string id;    // 唯一标识，如 CELL:u1 或 PORT:A[0]
    std::string label; // 展示名称
};

struct DAGEdge {
    std::string src;
    std::string dst;
    std::string via; // 通过的位/网络 ID 字符串
};

class DAG {
public:
    const DAGNode& add_node(const std::string& id, const std::string& label);
    void add_edge(const std::string& src, const std::string& dst, const std::string& via);
    bool has_node(const std::string& id) const;

    const std::unordered_map<std::string, DAGNode>& nodes() const { return nodes_; }
    const std::vector<DAGEdge>& edges() const { return edges_; }

    std::string to_dot() const;

private:
    std::unordered_map<std::string, DAGNode> nodes_;
    std::vector<DAGEdge> edges_;
    std::unordered_set<std::string> edge_set_; // 去重：src|dst|via
};

