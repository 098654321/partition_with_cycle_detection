#pragma once
#include "Json.h"
#include <string>
#include <vector>
#include <unordered_map>

// Yosys JSON 模型：模块、端口、单元与连接

struct YPort {
    std::string name;
    std::string direction; // "input" / "output" / "inout"
    std::vector<int> bits; // 位 ID
};

struct YCell {
    std::string name;
    std::string type;
    std::unordered_map<std::string, std::vector<int>> connections; // 端口到位ID列表
    std::unordered_map<std::string, std::string> port_directions;  // 端口方向
};

struct YModule {
    std::string name;
    std::unordered_map<std::string, YPort> ports;
    std::unordered_map<std::string, YCell> cells;
};

struct YDesign {
    std::unordered_map<std::string, YModule> modules;
    std::string top;
};

// 从 JSON DOM 构建 YDesign
class YosysJsonReader {
public:
    explicit YosysJsonReader(const json::Value& root) : root_(root) {}
    YDesign read();

private:
    const json::Value& root_;
    static std::vector<int> to_int_list(const json::Value& v);
};

