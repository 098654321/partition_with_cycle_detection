#include "Json.h"
#include "YosysModel.h"
#include "DAGBuilder.h"
#include <fstream>
#include <iostream>
#include <sstream>

// 入口：读取 Yosys JSON，构建模块数据流 DAG，输出 Graphviz DOT

static std::string read_file(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream oss; oss << ifs.rdbuf();
    return oss.str();
}

int main(int argc, char** argv) {
    try {
        std::string in = argc > 1 ? argv[1] : "../hierarchy_voter.json";
        std::string out = argc > 2 ? argv[2] : "dag_voter.dot";

        std::string text = read_file(in);
        json::Parser parser(text);
        json::Value root = parser.parse();

        YosysJsonReader reader(root);
        YDesign design = reader.read();

        DAGBuilder builder(design);
        DAG g = builder.build_for_top(true);

        std::ofstream ofs(out);
        ofs << g.to_dot();
        ofs.close();

        std::cout << "Top module: " << design.top << "\n";
        std::cout << "Nodes: " << g.nodes().size() << ", Edges: " << g.edges().size() << "\n";
        std::cout << "DOT written to: " << out << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

