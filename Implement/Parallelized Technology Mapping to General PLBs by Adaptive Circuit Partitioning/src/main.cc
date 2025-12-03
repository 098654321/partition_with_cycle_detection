#include "./verilog2hgraph/reader.hh"
#include "./global/debug.hh"
#include <exception>
#include <iostream>


int main(int argc, char* argv[]) {
    global::init_log("./debug.log");
    global::set_log_level(global::LogLevel::INFO);

try{
    // 检查参数数量
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        std::cerr << "Example: " << argv[0] << " config.json" << std::endl;
        return 1;
    }
    
    // 第一个参数 argv[0] 是程序名，第二个 argv[1] 是文件名
    std::string filename = argv[1];

    auto reader = parser::Reader();
    reader.json2module(filename);
    reader.test_read();
    
    return 0;
}
catch(std::exception& e) {
    global::log_exception(e.what());
    return 1;
}
}

