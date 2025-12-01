#include "DAGBuilder.h"
#include <unordered_map>
#include <string>

static bool is_output_dir(const std::string& d) { return d == "output"; }
static bool is_input_dir(const std::string& d) { return d == "input"; }

DAG DAGBuilder::build_for_top(bool include_top_ports) {
    DAG g;
    auto it = design_.modules.find(design_.top);
    if (it == design_.modules.end()) return g;
    const YModule& m = it->second;

    // 位到驱动/汇的列表
    std::unordered_map<int, std::vector<PinRef>> drivers;
    std::unordered_map<int, std::vector<PinRef>> sinks;

    // 顶层端口作为节点（可选）
    if (include_top_ports) {
        for (const auto& pkv : m.ports) {
            const YPort& p = pkv.second;
            std::string nid = std::string("PORT:") + p.name;
            g.add_node(nid, p.name);
            for (size_t i = 0; i < p.bits.size(); ++i) {
                int bit = p.bits[i];
                std::string plabel = p.name + "[" + std::to_string(i) + "]";
                if (is_output_dir(p.direction)) drivers[bit].push_back({nid, plabel});
                if (is_input_dir(p.direction)) sinks[bit].push_back({nid, plabel});
            }
        }
    }

    // 单元节点与端口方向解析
    for (const auto& ckv : m.cells) {
        const YCell& c = ckv.second;
        std::string nid = std::string("CELL:") + c.name;
        g.add_node(nid, c.name + " (" + c.type + ")");
        for (const auto& conn : c.connections) {
            const std::string& port = conn.first;
            const auto& bits = conn.second;
            std::string dir;
            auto pd = c.port_directions.find(port);
            if (pd != c.port_directions.end()) dir = pd->second;
            for (size_t i = 0; i < bits.size(); ++i) {
                int bit = bits[i];
                std::string plabel = c.name + ":" + port + "[" + std::to_string(i) + "]";
                if (is_output_dir(dir)) drivers[bit].push_back({nid, plabel});
                else if (is_input_dir(dir)) sinks[bit].push_back({nid, plabel});
                // 其他方向（inout）暂作为汇
                else sinks[bit].push_back({nid, plabel});
            }
        }
    }

    // 生成边：驱动 -> 汇
    for (const auto& kv : drivers) {
        int bit = kv.first;
        const auto& ds = kv.second;
        const auto it2 = sinks.find(bit);
        if (it2 == sinks.end()) continue;
        const auto& ss = it2->second;
        for (const auto& dref : ds) {
            for (const auto& sref : ss) {
                g.add_edge(dref.node_id, sref.node_id, std::to_string(bit));
            }
        }
    }

    return g;
}

