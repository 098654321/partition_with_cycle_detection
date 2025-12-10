#ifndef READER_HH
#define READER_HH


#include "config.hh"
#include <cstddef>
#include <unordered_map>
#include <unordered_set>


namespace parser {

class Reader {
public:
    Reader(): _module{} {}

public:
    auto json2module(const std::string& filename) -> Module;
    auto modue2hgraph() -> std::unordered_map<std::string, HyperGraph>;
    auto hgraph2hMetis(const HyperGraph& hg, const std::string& filename, std::size_t mode) -> void; 

    auto build_hierarchy() -> void;
    auto top_module_name() const -> std::string;
    auto hierarchy() const -> const std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>&;

public:
    void test_read();
    void test_hgraph(const std::unordered_map<std::string, HyperGraph>& hg);
    void test_hierarchy();
    void test_hmetis_output(const std::unordered_map<std::string, HyperGraph>& hg, const std::string& filename, std::size_t mode);

private:
    auto partly_contains_bits(
        const std::vector<std::variant<std::size_t, std::string>>&, const std::vector<std::variant<std::size_t, std::string>>&
    ) -> bool;
    
private:
    std::vector<Module> _module;
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> _hier_children;
    std::unordered_map<std::string, std::unordered_set<std::string>> _hier_parents;
    std::vector<std::string> _hier_roots;
};

}


#endif  // READER_HH
