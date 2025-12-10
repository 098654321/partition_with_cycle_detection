#include "config.hh"
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "../global/debug.hh"


namespace parser {

std::string port_dir_to_string(PortDirection direction) {
  switch (direction) {
    case PortDirection::INPUT: return "input";
    case PortDirection::INOUT: return "inout";
    case PortDirection::OUTPUT: return "output";
    default: throw std::runtime_error("unknown port direction!");
  }
}


std::string Port::to_string() const {
    std::string msg{"{port name = " + _name + "\n"};
      msg += "direction: " + port_dir_to_string(_direction) + "\n";
      msg += "bits: ";
      for (auto& bit: _bits) {
        msg += std::holds_alternative<std::string>(bit) ? std::get<std::string>(bit) : std::to_string(std::get<std::size_t>(bit));
        msg += " ";
      }
      msg += "\n}\n";

      return msg;
}


std::string Cell::to_string() const {
    std::string msg{"{cell name = " + _name + "\n"};
    msg += "hide_name: " + (_hide ? std::to_string(1) : std::to_string(0)) + "\n";
    msg += "type: " + _type + "\n";
    msg += "port direction: \n";
    for (auto& [name, port]: _ports) {
    msg += "    " + name + ": " + port_dir_to_string(port->_direction) + "\n";
    }
    msg += "connections: \n";
    for (auto& [name, port]: _ports) {
        msg += "    " + name + ": ";
        for (auto bit: port->_bits) {
            msg += (std::holds_alternative<std::string>(bit) ? std::get<std::string>(bit) : std::to_string(std::get<std::size_t>(bit)));
            msg += " ";
        }
        msg += "\n";
    }
    msg += "}\n";

    return msg;
}


std::string Module::to_string() const {
    std::string msg{"module name = " + _name + "\n"};
    for (auto& [name, port]: _ports) {
        msg += port->to_string();
    }
    for (auto& [name, cell]: _cells) {
        msg += cell->to_string();
    }
    msg += "\n";

    return msg;
}


/*
******************** HyperGraph *********************
*/


Vertex::Vertex(std::string name, std::size_t v_id, const std::shared_ptr<Cell>& cell)
    : _name{name}, _weight(1.0), _v_id(v_id), _cell(cell)
{
    if (cell == nullptr) {
        throw std::runtime_error("Initialize a vertex with null cell pointer");
    }

    for (const auto& [name, port]: cell->_ports) {
        this->_port_info.emplace(port->_name, port);
    }
}

Vertex::Vertex(std::string name, std::size_t v_id, const std::shared_ptr<Port>& port)
    : _name{name}, _weight(1.0), _v_id(v_id), _cell(nullptr)
{
    if (port == nullptr) {
        throw std::runtime_error("Initialize a vertex with null port pointer");
    }

    this->_port_info.emplace(port->_name, port);
}

std::string Vertex::to_string() const {
    std::string msg{"{vertex id: " + std::to_string(this->_v_id) + ", name: " + this->_name + "\n"};
    msg += "weight: " + std::to_string(this->_weight) + "\n";
    if (this->_cell) {
        msg += "type: a cell\n";
    }
    else {
        msg += "type: a module port\n";
    }
    msg += "port info: \n";
    for (auto& [name, port]: this->_port_info) {
        msg += "    " + name + ": " + port_dir_to_string(port->_direction) + ", bit_vector ";
        for (auto& bit: port->_bits) {
            msg += std::holds_alternative<std::string>(bit) ? std::get<std::string>(bit) : std::to_string(std::get<std::size_t>(bit));
            msg += " ";
        }
        msg += "\n";
    }
    msg += "}\n";
    return msg;
}


Edge::Edge(std::size_t e_id, const std::map<std::size_t, std::set<std::shared_ptr<Port>>>& port_info)
    : _e_id(e_id), _port_info(port_info)
{
    std::size_t min_bitwidth {0};
    for (auto& [v_id, ports]: this->_port_info) {
        for (auto& port: ports) {
            if (min_bitwidth == 0) {
                min_bitwidth = port->_bits.size();
            }
            else {
                min_bitwidth = std::min(min_bitwidth, port->_bits.size());
            }
        }
    }
    this->_weight = min_bitwidth;
}

auto Edge::vertices() const -> std::vector<std::size_t> {
    std::vector<std::size_t> vertices;
    for (auto& [v_id, ports]: this->_port_info) {
        vertices.emplace_back(v_id);
    }
    return vertices;
}

auto Edge::ports() const -> std::vector<std::shared_ptr<Port>> {
    std::vector<std::shared_ptr<Port>> connected_ports;
    for (auto& [v_id, ports]: this->_port_info) {
        connected_ports.insert(connected_ports.end(), ports.begin(), ports.end());
    }
    return connected_ports;
}


std::string Edge::to_string() const {
    std::string msg{"{edge id = " + std::to_string(this->_e_id) + "\n"};
    msg += "weight: " + std::to_string(this->_weight) + "\n";
    msg += "port info: \n";
    for (auto& [v_id, ports]: this->_port_info) {
        for (auto& port: ports) {
            msg += "    vertex: " + std::to_string(v_id) + ", " + port->_name + ": " + port_dir_to_string(port->_direction) + ", bit_vector ";
            for (auto& bit: port->_bits) {
                msg += std::holds_alternative<std::string>(bit) ? std::get<std::string>(bit) : std::to_string(std::get<std::size_t>(bit));
                msg += " ";
            }
            msg += "\n";
        }
    }
    msg += "\n}\n";
    return msg;
}

void Edge::remove_port_in_edge(std::size_t v_id, std::shared_ptr<Port>& port) {
    if (this->_port_info.find(v_id) == this->_port_info.end()) {
        return;
    }
    auto& ports = this->_port_info.at(v_id);
    if (ports.find(port) == ports.end()) {
        return;
    }
    global::log_debug("found port " + port->_name + " in vertex " + std::to_string(v_id) + " in edge " + std::to_string(this->_e_id));
    ports.erase(port);
}

void Edge::add_port_in_edge(std::size_t v_id, std::shared_ptr<Port>& port) {
    if (this->_port_info.find(v_id) == this->_port_info.end()) {
        this->_port_info.emplace(v_id, std::set<std::shared_ptr<Port>>{});
    }
    this->_port_info.at(v_id).emplace(port);
}

void HyperGraph::remove_port_in_edge(std::size_t e_id, std::size_t v_id, std::shared_ptr<Port>& port) {
    if (this->_edges.find(e_id) == this->_edges.end()) {
        global::log_info("Edge " + std::to_string(e_id) + " not found in hypergraph");
        return;
    }
    auto& edge = this->_edges.at(e_id);
    edge.remove_port_in_edge(v_id, port);
}

void HyperGraph::add_port_in_edge(std::size_t e_id, std::size_t v_id, std::shared_ptr<Port>& port) {
    const auto& iter = this->_edges.find(e_id);
    if (iter != this->_edges.end()) {
        const auto& v_port = iter->second.v_port();
        auto port_iter = v_port.find(v_id);
        if (port_iter == v_port.end() && port_iter->second.find(port) != port_iter->second.end()) {
            global::log_info("Port " + port->_name + " already connected to vertex " + std::to_string(v_id) + " in edge " + std::to_string(e_id));
            return;
        }
    }

    auto& edge = iter->second;
    edge.add_port_in_edge(v_id, port);
}

auto HyperGraph::remove_edge(std::size_t e_id) -> void {
    if (this->_edges.find(e_id) == this->_edges.end()) {
        global::log_info("Edge " + std::to_string(e_id) + " not found in hypergraph");
        return;
    }
    this->_edges.erase(e_id);
}

void HyperGraph::remove_vertex(std::size_t v_id) {
    if (this->_vertices.find(v_id) == this->_vertices.end()) {
        global::log_info("Vertex " + std::to_string(v_id) + " not found in hypergraph");
        return;
    }
    this->_vertices.erase(v_id);
    this->_v2e.erase(v_id);
}

void HyperGraph::remove_edge_in_v2e(std::size_t v_id, std::size_t e_id) {
    if (this->_v2e.find(v_id) == this->_v2e.end()) {
        global::log_info("Vertex " + std::to_string(v_id) + " not found in hypergraph");
        return;
    }
    auto& edges = this->_v2e.at(v_id);
    edges.erase(std::remove(edges.begin(), edges.end(), e_id), edges.end());
}

void HyperGraph::remove_all_edge_in_v2e(std::size_t v_id) {
    if (this->_v2e.find(v_id) == this->_v2e.end()) {
        global::log_info("Vertex " + std::to_string(v_id) + " not found in hypergraph");
        return;
    }
    this->_v2e.at(v_id).clear();
    this->_v2e.erase(v_id);
}

void HyperGraph::add_edge_in_v2e(std::size_t v_id, std::size_t e_id) {
    if (this->_v2e.find(v_id) == this->_v2e.end()) {
        global::log_info("Vertex " + std::to_string(v_id) + " not found in hypergraph");
        return;
    }
    auto& edges = this->_v2e.at(v_id);
    edges.emplace_back(e_id);
}

HyperGraph::HyperGraph(const std::vector<Vertex>& vertices, const std::vector<Edge>& edges) {
    // get vertices & edges
    for (const auto& vertex: vertices) {
        this->_vertices.emplace(vertex.v_id(), vertex);
    }
    for (auto& edge: edges) {
        this->_edges.emplace(edge.e_id(), edge);
    }

    std::map<std::size_t, std::shared_ptr<Port>> module_port{};
    std::unordered_map<std::size_t, std::size_t> old2new_vid{};
    for (auto& [v_id, vertex]: this->_vertices) {
        if (vertex.cell() == nullptr) {
            auto port = vertex.port_info();
            if (port.size() > 1) {
                throw std::logic_error("module port should have only one port");
            }
            module_port.emplace(v_id, port.begin()->second);
        }
    }
    for (auto& [v_id, port]: module_port) {
        this->_vertices.erase(v_id);
    }

    std::vector<std::size_t> old_ids;
    old_ids.reserve(this->_vertices.size());
    for (const auto& kv : this->_vertices) old_ids.emplace_back(kv.first);
    std::sort(old_ids.begin(), old_ids.end());

    std::unordered_map<std::size_t, Vertex> new_vertices{};
    std::size_t new_vid{0};
    for (auto old_id : old_ids) {
        auto vertex = this->_vertices.at(old_id);
        vertex.set_vid(new_vid);
        old2new_vid.emplace(old_id, new_vid);
        new_vertices.emplace(new_vid, vertex);
        ++new_vid;
    }
    this->_vertices = std::move(new_vertices);

    for (auto& [e_id, edge] : this->_edges) {
        for (auto& [v_id, port] : module_port) {
            edge.remove_port_in_edge(v_id, port);
        }
        std::map<std::size_t, std::set<std::shared_ptr<Port>>> updated;
        for (auto& [v_id, ports] : edge.v_port()) {
            auto it = old2new_vid.find(v_id);
            if (it != old2new_vid.end()) {
                updated.emplace(it->second, ports);
            }
        }
        edge.set_port_info(updated);
    }

    // create v2e
    for (auto& edge: this->_edges) {
        for (auto& v_id: edge.second.vertices()) {
            auto iter = this->_v2e.emplace(v_id, std::vector<std::size_t>{});
            iter.first->second.emplace_back(edge.second.e_id());
        }
    }

    if(this->_v2e.size() < this->_vertices.size()) {
        auto diff = this->_vertices.size() - this->_v2e.size();
        global::log_debug("Existing " + std::to_string(diff) + " vertices not included in edges!");
        this->to_string();
        throw std::logic_error("unexpected logic exception during creating hypergraph");
    }
}


std::string HyperGraph::to_string() const {
    std::string msg{std::to_string(this->_vertices.size()) + " hypergraph vertices: \n"};
    for (auto& [v_id, vertex]: this->_vertices) {
        msg += vertex.to_string();
    }
    msg += std::to_string(this->_edges.size()) + " hypergraph edges: \n";
    for (auto& [e_id, edge]: this->_edges) {
        msg += edge.to_string();
    }

    return msg;
}

auto HyperGraph::v2e() const -> const std::unordered_map<std::size_t, std::vector<std::size_t>>& {
    return this->_v2e;
}

auto HyperGraph::vertex_weight(std::size_t v_id) const -> double {
    const auto it = this->_vertices.find(v_id);
    if (it == this->_vertices.end()) throw std::out_of_range("vertex not found");
    return it->second.weight();
}

auto HyperGraph::edge_weight(std::size_t e_id) const -> double {
    const auto it = this->_edges.find(e_id);
    if (it == this->_edges.end()) throw std::out_of_range("edge not found");
    return it->second.weight();
}

auto HyperGraph::vertex_name(std::size_t v_id) const -> std::string {
    const auto it = this->_vertices.find(v_id);
    if (it == this->_vertices.end()) throw std::out_of_range("vertex not found");
    return it->second.name();
}


}
