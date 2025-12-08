#ifndef READER_HH
#define READER_HH


#include "config.hh"


namespace parser {

class Reader {
public:
    Reader(): _module{} {}

public:
    auto json2module(const std::string& filename) -> Module;
    auto modue2hgraph() -> std::unordered_map<std::string, HyperGraph>;
    auto hgraph2hMetis(const HyperGraph& hg, const std::string& filename) -> void; 

    // 加一个函数，根据 module2hgraph返回的内容提取最顶层的 module 

public:
    void test_read();
    void test_hgraph(const std::unordered_map<std::string, HyperGraph>& hg);

private:
    auto partly_contains_bits(
        const std::vector<std::variant<std::size_t, std::string>>&, const std::vector<std::variant<std::size_t, std::string>>&
    ) -> bool;

private:
    std::vector<Module> _module;
};

}


#endif  // READER_HH
