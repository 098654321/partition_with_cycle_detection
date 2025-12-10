#include "reader.hh"
#include "config.hh"
#include "json.h"
#include <cstddef>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <set>
#include <vector>
#include <algorithm>
#include "../global/debug.hh"


namespace parser {

static PortDirection str2dir(const std::string& s) {
    if (s == "input") return PortDirection::INPUT;
    if (s == "output") return PortDirection::OUTPUT;
    return PortDirection::INOUT;
}

static std::vector<std::variant<std::size_t, std::string>> parse_bits(const Json::Value& arr) {
    std::vector<std::variant<std::size_t, std::string>> out;
    out.reserve(arr.size());
    for (const auto& v : arr) {
        if (v.isInt()) {
            out.emplace_back(static_cast<std::size_t>(v.asInt()));
        } else if (v.isUInt()) {
            out.emplace_back(static_cast<std::size_t>(v.asUInt()));
        } else {
            out.emplace_back(v.asString());
        }
    }
    return out;
}

Module Reader::json2module(const std::string& filename) {
    global::log_info("Reading json and build ...");

    std::ifstream in(filename);
    if (!in.good()) {
        throw std::runtime_error(std::string("Failed to open file: ") + filename);
    }
    Json::CharReaderBuilder builder;
    std::string errs;
    Json::Value root;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    bool ok = reader->parse(content.data(), content.data() + content.size(), &root, &errs);
    if (!ok) {
        throw std::runtime_error(std::string("JSON parse failed: ") + errs);
    }

    const Json::Value& modules = root["modules"];
    if (modules.isNull()) {
        throw std::runtime_error("Missing 'modules' in JSON");
    }
    // find top module name; if none, the first one
    std::string target;
    for (const auto& name : modules.getMemberNames()) {
        const auto& attrs = modules[name]["attributes"];
        if (attrs.isMember("top")) {
            const auto s = attrs["top"].asString();
            bool any_one = false;
            for (char c : s) {
                if (c != '0') { any_one = true; break; }
            }
            if (any_one) { target = name; break; }
        }
        if (target.empty()) target = name;
    }
    // clear and populate all modules; ensure top module is first
    _module.clear();

    auto build_module = [&](const std::string& name) {
        Module m;
        m._name = name;
        const Json::Value& modv = modules[name];
        const Json::Value& ports = modv["ports"];
        for (const auto& pname : ports.getMemberNames()) {
            const auto& pval = ports[pname];
            Port p;
            p._name = pname;
            p._direction = str2dir(pval["direction"].asString());
            p._bits = parse_bits(pval["bits"]);
            m._ports.emplace(pname, std::make_shared<Port>(p));
        }
        const Json::Value& cells = modv["cells"];
        for (const auto& cname : cells.getMemberNames()) {
            const auto& cval = cells[cname];
            Cell c;
            c._name = cname;
            c._hide = cval["hide_name"].asInt() != 0;
            c._type = cval["type"].asString();

            std::unordered_map<std::string, Port> c_p{};
            const auto& pdirs = cval["port_directions"];
            const auto& conns = cval["connections"];
            for (const auto& dname : conns.getMemberNames()) {
                c_p.emplace(dname, Port{. _name = dname, ._direction = str2dir(pdirs[dname].asString()), ._bits = parse_bits(conns[dname])});
            }
            for (auto& [dname, port]: c_p) {
                c._ports.emplace(dname, std::make_shared<Port>(port));
            }
            
            m._cells.emplace(cname, std::make_shared<Cell>(c));
        }
        return m;
    };

    // build top module first
    Module top_mod = build_module(target);
    _module.emplace_back(std::move(top_mod));
    // build remaining modules
    for (const auto& name : modules.getMemberNames()) {
        if (name == target) continue;
        _module.emplace_back(build_module(name));
    }

    build_hierarchy();
    return _module.front();
}


// 还没有实现子模块的展开
std::unordered_map<std::string, HyperGraph> Reader::modue2hgraph() {
global::log_info("Building hypergraph ...");

   // for extending cell modules 
    std::set<std::string> module_names {};                                      // all module names
    std::unordered_map<std::string, HyperGraph> name2hg{};                      // for all module hg
    std::unordered_map<std::string, std::set<std::shared_ptr<Port>>> name2ports{};     // for all module ports
    std::unordered_map<std::string, std::set<std::shared_ptr<Cell>>> name2cell{};         // for cell_hg to be extended in module

    // collect all modules
global::log_debug("collecting modules ...");
    for (const auto& mod: this->_module) {
        module_names.emplace(mod._name);
    }

    for (const auto& mod : this->_module) {
        // data structures for creating hypergraph
        std::vector<Vertex> vertices;
        std::vector<Edge> edges;
        std::map<std::vector<std::variant<std::size_t, std::string>>, std::map<std::size_t, std::set<std::shared_ptr<Port>>>> info;  // signal_bit_vector -> <v_id, v_ports>
        auto module_name = mod._name;

        // create vertices using cell & port 
global::log_debug("creating vertices ...");
        std::size_t v_id{0};
        for (auto& [name, cell]: mod._cells) {  // using cell
            vertices.emplace_back(name, v_id++, cell);
            if (module_names.find(cell->_type) != module_names.end()) {                  // is a module object
                auto iter = name2cell.emplace(module_name, std::set<std::shared_ptr<Cell>>{});
                iter.first->second.emplace(cell);
                global::log_debug("cell " + name + " is a module cell");
            }
        }
        for (auto& [name, port]: mod._ports) {  // using port
            vertices.emplace_back(name, v_id++, port);
            auto iter = name2ports.emplace(module_name, std::set<std::shared_ptr<Port>>{});
            iter.first->second.emplace(port);
        }
global::log_debug("totally created " + std::to_string(vertices.size()) + " vertices");

        // create edges
global::log_debug("creating edges ...");
        for (auto& v: vertices) {
            auto vid = v.v_id();

            for (const auto& [name, port]: v.port_info()) {
                const auto& bit_vector = port->_bits;
                bool found = false;

                for (auto& [key, vids]: info) {
                    if (partly_contains_bits(key, bit_vector)) {
                        auto iter = vids.emplace(vid, std::set<std::shared_ptr<Port>>{});
                        iter.first->second.emplace(port);
                        found = true;
                        break;
                    } 
                }
                if (!found) {
                    info.emplace(bit_vector, std::map<std::size_t, std::set<std::shared_ptr<Port>>>{std::make_pair(vid, std::set<std::shared_ptr<Port>>{port})});
                }
            }
        }

        // collect edges
global::log_debug("collecting edges ...");
        std::size_t e_id{0};
        for (const auto& [_, vid_ports]: info) {
            edges.emplace_back(e_id++, vid_ports);
            if (vid_ports.size() <= 1) {
                const auto& [id, ports] = *vid_ports.begin();
                global::log_info(
                "A port are not connected in vertex index" + std::to_string(id)
                );
            }
        }
global::log_debug("totally created " + std::to_string(edges.size()) + " edges");

        // create hypergraph
global::log_debug("creating hypergraph, with module_name: ..." + module_name);
        name2hg.emplace(module_name, HyperGraph(vertices, edges));
    }
    
    // extend cells in upper level hypergraph（not implement）
    // for (const auto& [m, cell]: name2cell) {
    //     const auto& cell_hg = name2hg.at(cell->_type);      // cell hypergraph
    //     const auto& upper_hg = name2hg.at(m);               // upper_level hypergraph
    //     auto cell_id_in_upperhg = std::find_if(upper_hg.vertices().begin(), upper_hg.vertices().end(),\
    //     [&](const auto& v){ return v.name() == cell->_name; }
    //     )->v_id();

    //     // collect vertices in upper_hg 
    //     std::vector<Vertex> new_vertices{};
    //     std::unordered_map<std::size_t, std::size_t> cellhg_old2new_vid;
    //     new_vertices.insert(new_vertices.end(), upper_hg.vertices().begin(), upper_hg.vertices().end());
    //     // collect and update v_id in cell_hg
    //     auto current_v_id = upper_hg.vertices().size();
    //     for (const auto& v: cell_hg.vertices()) {
    //         cellhg_old2new_vid.emplace(v.v_id(), current_v_id);
    //         new_vertices.emplace_back(v.name(), current_v_id++, v.cell());
    //     }
    //     // collect edges in upper_hg
    //     auto new_edges_in_cellhg = std::vector<Edge>{};
    //     for (auto& edge: upper_hg.edges()) {
    //         new_edges_in_cellhg.emplace_back(edge);
    //     }
    //     // update v_id in edges in cell_hg
    //     std::unordered_map<std::size_t, std::size_t> cellhg_old2new_eid;
    //     auto current_e_id = upper_hg.edges().size();
    //     for (auto& edge: cell_hg.edges()) {
    //         auto e_id = current_e_id++;
    //         cellhg_old2new_eid.emplace(edge.e_id(), e_id);
    //         auto port_map = std::map<std::size_t, std::set<std::shared_ptr<Port>>>{};
    //         for (auto& [vid, ports]: edge.v_port()) {
    //             port_map.emplace(cellhg_old2new_vid.at(vid), ports);
    //         }
    //         new_edges_in_cellhg.emplace_back(e_id, port_map);
    //     }
    //     // create new hypergraph
    //     auto new_hg = HyperGraph(new_vertices, new_edges_in_cellhg);

    //     // update info in new_hypergraph
    //     for (const auto& [external_port_name, external_port]: cell->_ports) {
    //         // collect vid in cell_hg connected with external_port
    //         auto ex_port_id_in_cellhg = std::find_if(cell_hg.vertices().begin(), cell_hg.vertices().end(),\
    //         [&](const auto& v){ return v.name() == external_port_name; }
    //         )->v_id();
    //         const auto& e_to_external_port = cell_hg.v2e().at(ex_port_id_in_cellhg);
    //         auto vid_to_external_port = std::vector<std::size_t> {};
    //         for (const auto& e: e_to_external_port) {
    //             const auto& edge = cell_hg.edges().at(e);
    //             for (const auto& [vid, ports]: edge.v_port()) {
    //                 if (vid != ex_port_id_in_cellhg) {
    //                     vid_to_external_port.emplace_back(vid);
    //                     break;
    //                 }
    //             }
    //         }
    //         //update info in new_hg
    //         for (const auto& edge_ids: upper_hg.v2e().at(cell_id_in_upperhg)) {
    //             // edge: an edge in upper_hg, containing cell_id_in_upperhg
    //             const auto& edge = upper_hg.edges().at(edge_ids);
    //             // check if the edge contains the external_port
    //             for (const auto& [vid, ports]: edge.v_port()) {
    //                 std::shared_ptr<Port> found_port = nullptr;
    //                 for(const auto& port: ports) {
    //                     if (port->_name == external_port_name) {
    //                         found_port = port;
    //                         break;
    //                     }
    //                 }
    //                 if (vid == cell_id_in_upperhg && found_port) {
    //                     // the edge contains the external_port, update info of this edge in new_hg
    //                     new_hg.remove_port_in_edge(edge.e_id(), vid, found_port);
    //                     for (const auto& vid_in_cellhg: vid_to_external_port) {
    //                         new_hg.add_port_in_edge(edge.e_id(), cellhg_old2new_vid.at(vid_in_cellhg), found_port);
    //                     }
    //                     for (auto eid_in_cellhg: cell_hg.v2e().at(ex_port_id_in_cellhg)) {
    //                         new_hg.remove_edge(cellhg_old2new_eid.at(eid_in_cellhg));
    //                         new_hg.remove_edge_in_v2e(ex_port_id_in_cellhg, cellhg_old2new_eid.at(eid_in_cellhg));
    //                     }
    //                     new_hg.remove_vertex(cellhg_old2new_vid.at(ex_port_id_in_cellhg));
    //                     new_hg.add_edge_in_v2e(ex_port_id_in_cellhg, edge.e_id());
    //                 }
    //             }
    //         }
    //     }
    //     new_hg.remove_vertex(cell_id_in_upperhg);
    //     new_hg.remove_all_edge_in_v2e(cell_id_in_upperhg);
    //     name2hg.at(m) = new_hg;
    //     name2hg.erase(cell->_type);
    // }

    // clear external_ports in final_hypergraph
    // update id in final_hypergraph

    if (name2hg.size() > 1) {
        global::log_info("Existing isolated module not in top hierarchy!");
    }
    return name2hg;
}


void Reader::test_read() {
    using namespace global;
    global::log_debug("Testing json2module() ...");
    for (const auto& m : this->_module) {
        global::log_debug(m.to_string());
    }
    global::log_debug("Test end");
}


void Reader::test_hgraph(const std::unordered_map<std::string, HyperGraph>& hg) {
    using namespace global;
    global::log_debug("Testing modue2hgraph() ...");
    for (auto& [name, module]: hg) {
        global::log_debug(name + ": ");
        global::log_debug(module.to_string());
    }
    global::log_debug("Test end");
}

void Reader::test_hierarchy() {
    using namespace global;
    global::log_debug("Testing hierarchy ...");
    if (_hier_children.empty() && _module.size() > 0) {
        build_hierarchy();
    }
    std::string roots_msg{"roots: "};
    for (const auto& r : _hier_roots) {
        roots_msg += r + " ";
    }
    global::log_debug(roots_msg);

    for (const auto& m : _module) {
        global::log_debug(std::string("module ") + m._name + ":");
        const auto it = _hier_children.find(m._name);
        if (it == _hier_children.end() || it->second.empty()) {
            global::log_debug("  children: none");
        } else {
            for (const auto& p : it->second) {
                global::log_debug(std::string("  ") + p.first + " -> " + p.second);
            }
        }
        const auto pit = _hier_parents.find(m._name);
        if (pit == _hier_parents.end() || pit->second.empty()) {
            global::log_debug("  parents: none");
        } else {
            std::string parents_msg{"  parents: "};
            for (const auto& par : pit->second) parents_msg += par + " ";
            global::log_debug(parents_msg);
        }
    }
    global::log_debug("Test end");
}

auto Reader::hgraph2hMetis(const HyperGraph& hg, const std::string& filename, std::size_t mode) -> void {
    const auto& v2e = hg.v2e();

    // compact vertex indices: map original v_id -> [1..N]
    std::vector<std::size_t> v_ids;
    v_ids.reserve(v2e.size());
    for (const auto& kv : v2e) v_ids.emplace_back(kv.first);
    std::sort(v_ids.begin(), v_ids.end());
    std::unordered_map<std::size_t, std::size_t> vmap; // original -> compact 1-based
    for (std::size_t i = 0; i < v_ids.size(); ++i) vmap.emplace(v_ids[i], i + 1);

    // invert v2e using compact indices
    std::unordered_map<std::size_t, std::vector<std::size_t>> e2v;
    e2v.reserve(v2e.size());
    for (const auto& [v_id, edges] : v2e) {
        const auto idx = vmap.at(v_id);
        for (const auto& e_id : edges) {
            auto& lst = e2v[e_id];
            lst.emplace_back(idx);
        }
    }

    const std::size_t hyperedge_num = e2v.size();
    const std::size_t node_num = v_ids.size();

    std::vector<std::size_t> edge_ids;
    edge_ids.reserve(e2v.size());
    for (const auto& kv : e2v) edge_ids.emplace_back(kv.first);
    std::sort(edge_ids.begin(), edge_ids.end());

    std::ofstream out(filename);
    if (!out.good()) {
        throw std::runtime_error(std::string("Failed to open hMetis output file: ") + filename);
    }

    out << hyperedge_num << ' ' << node_num << ' ' << mode << '\n';

    // edges section
    const bool edge_w = (mode == 1 || mode == 11);
    for (auto e_id : edge_ids) {
        if (edge_w) {
            out << static_cast<long long>(hg.edge_weight(e_id)) << ' ';
        }
        auto nodes = e2v.at(e_id);
        std::sort(nodes.begin(), nodes.end());
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (i) out << ' ';
            out << nodes[i];
        }
        out << '\n';
    }

    // vertex weights section
    const bool vertex_w = (mode == 10 || mode == 11);
    if (vertex_w) {
        for (auto v_id : v_ids) {
            out << static_cast<long long>(hg.vertex_weight(v_id)) << '\n';
        }
    }

    out.flush();

    // write vertex-id to cell-name mapping in the same directory as filename
    try {
        std::string mapfile = filename + ".vertices.txt";
        std::ofstream mout(mapfile);
        if (mout.good()) {
            for (auto v_id : v_ids) {
                auto idx = vmap.at(v_id);
                auto name = hg.vertex_name(v_id);
                mout << idx << ' ' << name << '\n';
            }
            mout.flush();
        }
    } catch (...) {
        // ignore mapping file errors silently
    }
}

void Reader::test_hmetis_output(const std::unordered_map<std::string, HyperGraph>& hg, const std::string& filename, std::size_t mode) {
    using namespace global;
    global::log_debug("Testing hgraph2hMetis output ...");

    std::string modname = top_module_name();
    const HyperGraph* target = nullptr;
    auto it = hg.find(modname);
    if (it != hg.end()) {
        target = &it->second;
    } else if (!hg.empty()) {
        target = &hg.begin()->second;
        modname = hg.begin()->first;
    }

    if (!target) {
        global::log_info("No hypergraph found to output");
        return;
    }

    hgraph2hMetis(*target, filename, mode);
    global::log_debug(std::string("hMetis file written for module ") + modname + ": " + filename);
}

auto Reader::build_hierarchy() -> void {
    _hier_children.clear();
    _hier_parents.clear();
    _hier_roots.clear();

    std::unordered_set<std::string> names;
    for (const auto& m : _module) {
        names.emplace(m._name);
    }
    for (const auto& m : _module) {
        for (const auto& [cname, cell] : m._cells) {
            const auto& t = cell->_type;
            if (names.find(t) != names.end()) {
                _hier_children[m._name].emplace_back(cname, t);
                _hier_parents[t].insert(m._name);
            }
        }
    }
    for (const auto& n : names) {
        if (_hier_parents[n].empty()) {
            _hier_roots.emplace_back(n);
        }
    }
}

auto Reader::top_module_name() const -> std::string {
    if (!_hier_roots.empty()) return _hier_roots.front();
    if (!_module.empty()) return _module.front()._name;
    return std::string{};
}

auto Reader::hierarchy() const -> const std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>& {
    return _hier_children;
}


// 检测 target 是不是一模一样的包含在 bits 中；使用 KMP 算法检测
bool Reader::partly_contains_bits(
    const std::vector<std::variant<std::size_t, std::string>>& bits,
    const std::vector<std::variant<std::size_t, std::string>>& target
) {
    auto partly_compare = [&](
        const std::vector<std::variant<std::size_t, std::string>>& longer, const std::vector<std::variant<std::size_t, std::string>>& shorter
    ) -> bool {
        // 预处理：构造KMP的next数组
        auto build_next = [&](const std::vector<std::variant<std::size_t, std::string>>& pat) {
            std::vector<int> next(pat.size() + 1, -1);
            int i = 0, j = -1;
            while (i < static_cast<int>(pat.size())) {
                while (j >= 0 && pat[i] != pat[j]) j = next[j];
                ++i; ++j;
                next[i] = j;
            }
            return next;
        };

        // 确保 longer.size() >= shorter.size()
        if (longer.size() < shorter.size()) return false;

        const auto& next = build_next(shorter);
        int i = 0, j = 0;
        while (i < static_cast<int>(longer.size())) {
            while (j >= 0 && longer[i] != shorter[j]) j = next[j];
            ++i; ++j;
            if (j == static_cast<int>(shorter.size())) {
                // 在 longer 中找到完整匹配的连续子串
                return true;
            }
        }
        return false;
    };

    // 暂时不支持 bit vector 里面值和字符串混搭的。如果是字符串常量值，就不是不同 cell 端口之间的连接
    for (auto& v: bits) {
        if (std::holds_alternative<std::string>(v)) {
            return false;
        }
    }
    for (auto& v: target) {
        if (std::holds_alternative<std::string>(v)) {
            return false;
        }
    }

    if (bits.size() < target.size()) {
        auto result = partly_compare(target, bits);
        return result;
    } else {
        auto result = partly_compare(bits, target);
        return result;
    }
}

}
