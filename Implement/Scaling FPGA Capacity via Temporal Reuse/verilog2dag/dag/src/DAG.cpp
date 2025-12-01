#include "DAG.h"
#include <sstream>

const DAGNode& DAG::add_node(const std::string& id, const std::string& label) {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        DAGNode n{ id, label };
        auto res = nodes_.emplace(id, std::move(n));
        return res.first->second;
    }
    return it->second;
}

bool DAG::has_node(const std::string& id) const { return nodes_.find(id) != nodes_.end(); }

void DAG::add_edge(const std::string& src, const std::string& dst, const std::string& via) {
    if (src == dst) return; // 避免自环
    std::string key = src + "|" + dst + "|" + via;
    if (edge_set_.insert(key).second) {
        edges_.push_back({src, dst, via});
    }
}

std::string DAG::to_dot() const {
    std::ostringstream oss;
    oss << "digraph G {\n";
    for (const auto& kv : nodes_) {
        const DAGNode& n = kv.second;
        oss << "  \"" << n.id << "\" [label=\"" << n.label << "\"];\n";
    }
    for (const auto& e : edges_) {
        oss << "  \"" << e.src << "\" -> \"" << e.dst << "\" [label=\"" << e.via << "\"];\n";
    }
    oss << "}\n";
    return oss.str();
}

