#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PortInfo {
  std::string name;
  std::string direction;
  std::vector<int> bits;
  int width = 0;
  std::vector<char> const_bits;
};

struct CellInfo {
  std::string name;
  std::string type;
  std::vector<PortInfo> ports;
};

struct ModuleInfo {
  std::string name;
  std::vector<CellInfo> cells;
  std::vector<PortInfo> ports;
};

struct NetUse {
  int width = 0;
  std::unordered_set<std::string> inside_inputs;
  std::unordered_set<std::string> inside_outputs;
  std::unordered_set<std::string> cells;
};

struct PartDesign {
  std::vector<CellInfo> cells;
  std::unordered_map<std::string, NetUse> nets;
};

class Writer {
public:
  int run(const std::string& json_path,
          const std::string& vertices_txt,
          const std::string& part_file,
          const std::string& out_dir);

private:
  static std::string normalize_bits(const std::vector<int>& bits);
  static ModuleInfo parse_json(const std::string& json_path);
  static std::unordered_map<int, std::string> read_vertices_map(const std::string& path);
  static std::unordered_map<std::string, int> build_cell_part_map(const std::string& part_file,
                                                                  const std::unordered_map<int, std::string>& id2cell);
  void generate_modules(const std::string& out_dir);

  ModuleInfo M_;
  std::unordered_map<int, std::string> id2cell_;
  std::unordered_map<std::string, int> cell2part_;
  std::unordered_map<int, PartDesign> parts_;
  struct Conn { std::string cell; std::string port; std::string dir; int width; };
  std::unordered_map<std::string, std::vector<Conn>> net2conns_;
};
