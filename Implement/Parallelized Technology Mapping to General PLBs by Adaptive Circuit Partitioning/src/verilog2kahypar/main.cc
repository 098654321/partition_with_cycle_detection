#include "./reader.hh"
#include "../global/debug.hh"
#include <exception>
#include <iostream>
#include <exception>


int main(int argc, char* argv[]) {
    global::init_log("./debug_read.log");
    global::set_log_level(global::LogLevel::DEBUG);

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

    auto module = reader.json2module(filename);
    reader.test_read();
    auto hg = reader.modue2hgraph();
    reader.test_hgraph(hg);
    reader.test_hierarchy();
    reader.test_hmetis_output(hg, "../kahypar/run_hmetis.txt", 11);
    
    return 0;
}
catch(std::exception& e) {
    global::log_exception(e.what());
    return 1;
}
}

