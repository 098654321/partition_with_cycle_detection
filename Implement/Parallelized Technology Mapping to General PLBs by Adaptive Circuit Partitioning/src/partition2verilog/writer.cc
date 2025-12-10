#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../json/json.h"
#include "writer.hh"
#include "../global/debug.hh"

std::string Writer::normalize_bits(const std::vector<int>& bits) {
  std::ostringstream os;
  for (size_t i = 0; i < bits.size(); ++i) {
    if (i) os << ',';
    os << bits[i];
  }
  return os.str();
}

ModuleInfo Writer::parse_json(const std::string& json_path) {
  std::ifstream in(json_path);
  if (!in.good()) throw std::runtime_error("cannot open json: " + json_path);
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  Json::CharReaderBuilder builder;
  std::string errs;
  Json::Value root;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  bool ok = reader->parse(content.data(), content.data() + content.size(), &root, &errs);
  if (!ok) throw std::runtime_error("json parse failed: " + errs);

  const auto& modules = root["modules"];
  std::string top;
  for (const auto& name : modules.getMemberNames()) {
    const auto& attrs = modules[name]["attributes"];
    if (attrs.isMember("top")) {
      const auto s = attrs["top"].asString();
      bool any_one = false;
      for (char c : s) { if (c != '0') { any_one = true; break; } }
      if (any_one) { top = name; break; }
    }
    if (top.empty()) top = name;
  }

  ModuleInfo M; M.name = top;
  const auto& mod = modules[top];
  global::log_info(std::string("top=") + M.name);

  const auto& ports = mod["ports"];
  for (const auto& pname : ports.getMemberNames()) {
    const auto& pval = ports[pname];
    PortInfo p;
    p.name = pname;
    p.direction = pval["direction"].asString();
    for (const auto& v : pval["bits"]) {
      if (v.isInt()) p.bits.push_back(v.asInt());
      else if (v.isUInt()) p.bits.push_back((int)v.asUInt());
    }
    M.ports.push_back(std::move(p));
  }
  global::log_info(std::string("ports=") + std::to_string(M.ports.size()));

  const auto& cells = mod["cells"];
  for (const auto& cname : cells.getMemberNames()) {
    const auto& cval = cells[cname];
    CellInfo C; C.name = cname; C.type = cval["type"].asString();
    const auto& dirs = cval["port_directions"];
    const auto& conns = cval["connections"];
    for (const auto& dname : conns.getMemberNames()) {
      PortInfo p; p.name = dname; p.direction = dirs[dname].asString();
      int w = 0;
      for (const auto& v : conns[dname]) {
        if (v.isInt()) { p.bits.push_back(v.asInt()); ++w; }
        else if (v.isUInt()) { p.bits.push_back((int)v.asUInt()); ++w; }
        else if (v.isString()) { const auto s = v.asString(); char c = (s.empty() ? '0' : s[0]); p.const_bits.push_back(c); ++w; }
      }
      p.width = w;
      C.ports.push_back(std::move(p));
    }
    M.cells.push_back(std::move(C));
  }
  global::log_info(std::string("cells=") + std::to_string(M.cells.size()));
  return M;
}

std::unordered_map<int, std::string> Writer::read_vertices_map(const std::string& path) {
  std::unordered_map<int, std::string> id2cell;
  std::ifstream in(path);
  if (!in.good()) throw std::runtime_error("cannot open vertices map: " + path);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    std::istringstream iss(line);
    int vid; std::string cell;
    if (!(iss >> vid >> cell)) continue;
    id2cell[vid] = cell;
  }
  global::log_info(std::string("vertices_map entries=") + std::to_string(id2cell.size()));
  return id2cell;
}

std::unordered_map<std::string, int> Writer::build_cell_part_map(const std::string& part_file, const std::unordered_map<int, std::string>& id2cell) {
  std::unordered_map<std::string, int> cell2part;
  std::ifstream in(part_file);
  if (!in.good()) throw std::runtime_error("cannot open partition file: " + part_file);
  std::string line; int idx = 1;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    int p = std::stoi(line);
    auto it = id2cell.find(idx);
    if (it != id2cell.end()) cell2part[it->second] = p;
    ++idx;
  }
  global::log_info(std::string("part_map entries=") + std::to_string(cell2part.size()));
  return cell2part;
}

void Writer::generate_modules(const std::string& out_dir) {
  for (auto& kvp : parts_) {
    int p = kvp.first;
    auto& PD = kvp.second;
    std::ostringstream os;
    std::string modname = M_.name + std::string("_part") + std::to_string(p);
    struct PortDecl { std::string name; std::string dir; int width; };
    std::vector<PortDecl> portdecls;
    std::unordered_map<std::string, std::string> internal_alias; // netkey -> internal wire name
    std::unordered_map<std::string, std::string> conn_alias; // netkey|cell|port -> module port name
    int net_idx = 0;
    for (const auto& nk : PD.nets) {
      const std::string& netkey = nk.first;
      const auto& u = nk.second;
      size_t total_conns = net2conns_[netkey].size();
      size_t local_conns = u.cells.size();
      bool is_cut = !(local_conns == total_conns);
      global::log_debug(std::string("net ") + netkey + (is_cut ? " cut" : " internal"));
      if (is_cut) {
        const auto& conns = net2conns_[netkey];
        for (const auto& c : conns) {
          if (c.cell == "$top") continue;
          auto itp = cell2part_.find(c.cell);
          int cp = (itp != cell2part_.end()) ? itp->second : 0;
          if (cp != p) continue;
          std::string pname = c.cell + std::string("_") + c.port;
          portdecls.push_back({pname, c.dir, c.width});
          conn_alias[netkey + std::string("|") + c.cell + std::string("|") + c.port] = pname;
          global::log_debug(std::string("  port ") + pname + std::string(" width=") + std::to_string(c.width) + std::string(" dir=") + c.dir);
        }
      } else {
        std::string nname = std::string("net_") + std::to_string(net_idx++);
        internal_alias[netkey] = nname;
        global::log_debug(std::string("  internal wire ") + nname + std::string(" width=") + std::to_string(u.width));
      }
    }

    os << "module " << modname << " (\n";
    for (size_t i = 0; i < portdecls.size(); ++i) {
      os << "    " << portdecls[i].name;
      if (i + 1 < portdecls.size()) os << ",";
      os << "\n";
    }
    os << ");\n";

    for (const auto& pd : portdecls) {
      if (pd.width > 1) os << pd.dir << " [" << (pd.width - 1) << ":0] " << pd.name << ";\n";
      else os << pd.dir << " " << pd.name << ";\n";
    }

    for (const auto& nk : PD.nets) {
      const auto& netkey = nk.first; const auto& u = nk.second;
      auto it = internal_alias.find(netkey);
      if (it != internal_alias.end()) {
        const auto& nname = it->second;
        if (u.width > 1) os << "wire [" << (u.width - 1) << ":0] " << nname << ";\n";
        else os << "wire " << nname << ";\n";
      }
    }

    for (const auto& C : PD.cells) {
      os << C.type << " " << C.name << " (\n";
      for (size_t i = 0; i < C.ports.size(); ++i) {
        const auto& P = C.ports[i];
        if (P.bits.empty() && !P.const_bits.empty()) {
          if (P.width <= 1) { char b = P.const_bits[0]; os << "    ." << P.name << "(1'b" << b << ")"; }
          else { std::string pat; pat.reserve(P.width); for (char c : P.const_bits) pat.push_back(c); os << "    ." << P.name << "(" << P.width << "'b" << pat << ")"; }
        } else {
          std::string key = normalize_bits(P.bits);
          auto ca = conn_alias.find(key + std::string("|") + C.name + std::string("|") + P.name);
          if (ca != conn_alias.end()) { os << "    ." << P.name << "(" << ca->second << ")"; }
          else { auto ia = internal_alias.find(key); if (ia == internal_alias.end()) continue; os << "    ." << P.name << "(" << ia->second << ")"; }
        }
        if (i + 1 < C.ports.size()) os << ",";
        os << "\n";
      }
      os << ");\n";
    }

    os << "endmodule\n";

    std::string outpath = out_dir + "/" + modname + ".v";
    std::ofstream outf(outpath);
    if (!outf.good()) {
      throw std::runtime_error(std::string("cannot write: ") + outpath);
    }
    outf << os.str();
    outf.flush();
    global::log_info(std::string("write ") + outpath + std::string(" ports=") + std::to_string(portdecls.size()) + std::string(" cells=") + std::to_string(PD.cells.size()));
  }
}

int Writer::run(const std::string& json_path,
                const std::string& vertices_txt,
                const std::string& part_file,
                const std::string& out_dir) {
  global::log_info(std::string("json=") + json_path);
  global::log_info(std::string("vertices=") + vertices_txt);
  global::log_info(std::string("part=") + part_file);
  global::log_info(std::string("out_dir=") + out_dir);
  M_ = parse_json(json_path);
  id2cell_ = read_vertices_map(vertices_txt);
  cell2part_ = build_cell_part_map(part_file, id2cell_);

  for (const auto& C : M_.cells) {
    int p = 0;
    auto itp = cell2part_.find(C.name);
    if (itp != cell2part_.end()) p = itp->second;
    parts_[p].cells.push_back(C);
  }
  global::log_info(std::string("parts=") + std::to_string(parts_.size()));

  for (const auto& C : M_.cells) {
    for (const auto& P : C.ports) {
      if (P.bits.empty()) continue;
      std::string key = normalize_bits(P.bits);
      net2conns_[key].push_back({C.name, P.name, P.direction, (int)P.bits.size()});
    }
  }
  for (const auto& P : M_.ports) {
    if (P.bits.empty()) continue;
    std::string key = normalize_bits(P.bits);
    net2conns_[key].push_back({"$top", P.name, P.direction, (int)P.bits.size()});
  }
  global::log_info(std::string("nets=") + std::to_string(net2conns_.size()));

  for (const auto& kv : net2conns_) {
    const std::string& netkey = kv.first;
    const auto& conns = kv.second;
    int width = conns.empty() ? 1 : conns.front().width;
    std::unordered_map<int, NetUse> uses;
    for (const auto& c : conns) {
      if (c.cell == "$top") continue;
      int p = 0;
      auto itp = cell2part_.find(c.cell);
      if (itp != cell2part_.end()) p = itp->second;
      auto& u = uses[p];
      u.width = width;
      u.cells.insert(c.cell);
      if (c.dir == "input") u.inside_inputs.insert(c.cell);
      else if (c.dir == "output") u.inside_outputs.insert(c.cell);
      else { u.inside_inputs.insert(c.cell); u.inside_outputs.insert(c.cell); }
    }
    for (auto& pu : uses) {
      int p = pu.first; auto& u = pu.second;
      parts_[p].nets[netkey] = u;
    }
  }

  generate_modules(out_dir);
  return 0;
}
