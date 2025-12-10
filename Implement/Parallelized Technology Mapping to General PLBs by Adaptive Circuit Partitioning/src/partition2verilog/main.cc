#include <iostream>
#include <string>
#include "writer.hh"
#include "../global/debug.hh"

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << "usage: partition2verilog <yosys_json> <vertices_txt> <part_file> <out_dir>\n";
    return 1;
  }
  std::string json_path = argv[1];
  std::string vertices_txt = argv[2];
  std::string part_file = argv[3];
  std::string out_dir = argv[4];
  global::init_log("debug_write.log", false);
  global::set_log_level(global::LogLevel::DEBUG);
  global::log_info("partition2verilog start");
  Writer w;
  try {
    int rc = w.run(json_path, vertices_txt, part_file, out_dir);
    global::log_info("partition2verilog done");
    return rc;
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    global::log_exception(e.what());
    return 2;
  }
}
