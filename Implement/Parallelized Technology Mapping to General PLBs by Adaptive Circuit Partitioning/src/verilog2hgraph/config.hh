#ifndef CONFIG_HH
#define CONFIG_HH

#include <cstddef>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <variant>
#include <memory>
#include <set>


namespace parser {

/*
************************** Json file info **************************
*/

enum class PortDirection {
    INPUT,
    OUTPUT,
    INOUT
};

std::string port_dir_to_string(PortDirection direction);

struct Port {
    std::string _name;
    PortDirection _direction;
    std::vector<std::variant<std::size_t, std::string>> _bits;

    std::string to_string() const;
};


struct Cell {
    std::string _name;
    bool _hide;
    std::string _type;
    std::map<std::string, std::shared_ptr<Port>> _ports;        // port_name, Port*

    std::string to_string() const;
};


struct Module {
    std::string _name;
    std::map<std::string, std::shared_ptr<Port>> _ports;
    std::map<std::string, std::shared_ptr<Cell>> _cells;

    // 加一个数据结构，描述每个 module 的层次，以及每个 module 包含了哪些 cell

    std::string to_string() const;
};


/*
************************** HyperGraph **************************
*/

class Vertex {
public:
    Vertex(std::string, std::size_t, const std::shared_ptr<Cell>&);
    Vertex(std::string, std::size_t, const std::shared_ptr<Port>&);
    ~Vertex() = default;

public:
    auto name() const -> std::string {return this->_name;}
    auto weight() const -> double {return this->_weight;}
    auto v_id() const -> std::size_t {return this->_v_id;}
    auto cell() const -> std::shared_ptr<Cell> {return this->_cell;}
    auto to_string() const -> std::string;
    auto port_info() const -> std::map<std::string, std::shared_ptr<Port>> {return this->_port_info;}

private:
    std::string _name;
    double _weight;
    std::size_t _v_id;
    std::shared_ptr<Cell> _cell;    // if nullptr, then vertex is a port
    std::map<std::string, std::shared_ptr<Port>> _port_info;
};


class Edge {
public:
    Edge(std::size_t, const std::map<std::size_t, std::set<std::shared_ptr<Port>>>&);
    ~Edge() = default;

public:
    auto weight() const -> double {return this->_weight;} 
    auto e_id() const -> std::size_t {return this->_e_id;}
    auto v_port() const -> std::map<std::size_t, std::set<std::shared_ptr<Port>>> {return this->_port_info;}
    auto to_string() const -> std::string;
    auto vertices() const -> std::vector<std::size_t>;
    auto ports() const -> std::vector<std::shared_ptr<Port>>;

    auto remove_port_in_edge(std::size_t v_id, std::shared_ptr<Port>& port) -> void;
    auto add_port_in_edge(std::size_t v_id, std::shared_ptr<Port>& port) -> void;

private:
    std::size_t _e_id;
    double _weight;                                             // maximum bitwidth on connections
    std::map<std::size_t, std::set<std::shared_ptr<Port>>> _port_info;    // ports in this edge, not all the ports in vertex
};


class HyperGraph {
public:
    HyperGraph(const std::vector<Vertex>&, const std::vector<Edge>&);
    ~HyperGraph() = default;

public:
    auto to_string() const -> std::string;
    auto vertices() const -> const std::vector<Vertex>&;
    auto edges() const -> const std::vector<Edge>&;
    auto v2e() const -> const std::unordered_map<std::size_t, std::vector<std::size_t>>&;

    auto remove_port_in_edge(std::size_t e_id, std::size_t v_id, std::shared_ptr<Port>& port) -> void;
    auto add_port_in_edge(std::size_t e_id, std::size_t v_id, std::shared_ptr<Port>& port) -> void;
    auto remove_edge(std::size_t e_id) -> void;
    auto remove_vertex(std::size_t v_id) -> void;
    auto remove_edge_in_v2e(std::size_t v_id, std::size_t e_id) -> void;
    auto remove_all_edge_in_v2e(std::size_t v_id) -> void;
    auto add_edge_in_v2e(std::size_t v_id, std::size_t e_id) -> void;
    
private:
    std::unordered_map<std::size_t, Vertex> _vertices;
    std::unordered_map<std::size_t, Edge> _edges;
    std::unordered_map<std::size_t, std::vector<std::size_t>> _v2e;  // map vertex id to edge id
};


}

#endif  // CONFIG_HH

