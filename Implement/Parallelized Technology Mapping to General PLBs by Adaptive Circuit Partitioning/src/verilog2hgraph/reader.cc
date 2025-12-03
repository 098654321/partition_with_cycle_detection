#include "reader.hh"
#include "json.h"
#include <fstream>
#include "../global/debug.hh"


namespace parser {

static PortDirection str2dir(const std::string& s) {
    if (s == "input") return PortDirection::INPUT;
    if (s == "output") return PortDirection::OUTPUT;
    return PortDirection::INOUT;
}

static std::vector<std::variant<int, std::string>> parse_bits(const Json::Value& arr) {
    std::vector<std::variant<int, std::string>> out;
    out.reserve(arr.size());
    for (const auto& v : arr) {
        if (v.isInt()) {
            out.emplace_back(v.asInt());
        } else if (v.isUInt()) {
            out.emplace_back(static_cast<int>(v.asUInt()));
        } else {
            out.emplace_back(v.asString());
        }
    }
    return out;
}

Module Reader::json2module(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.good()) {
        throw std::runtime_error(std::string("Failed to open file: ") + filename);
    }
    Json::CharReaderBuilder builder;
    std::string errs;
    Json::Value root;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    bool ok = reader->parse(content.data(), content.data() + content.size(), &root, &errs);
    if (!ok) {
        throw std::runtime_error(std::string("JSON parse failed: ") + errs);
    }

    const Json::Value& modules = root["modules"];
    if (modules.isNull()) {
        throw std::runtime_error("Missing 'modules' in JSON");
    }
    std::string target;
    for (const auto& name : modules.getMemberNames()) {
        const auto& attrs = modules[name]["attributes"];
        if (attrs.isMember("top")) {
            const auto s = attrs["top"].asString();
            bool any_one = false;
            for (char c : s) {
                if (c != '0') { any_one = true; break; }
            }
            if (any_one) { target = name; break; }
        }
        if (target.empty()) target = name;
    }

    const Json::Value& mod = modules[target];
    _module._name = target;

    const Json::Value& ports = mod["ports"];
    for (const auto& pname : ports.getMemberNames()) {
        const auto& pval = ports[pname];
        Port p;
        p._name = pname;
        p._direction = str2dir(pval["direction"].asString());
        p._bits = parse_bits(pval["bits"]);
        _module._ports.emplace(pname, std::move(p));
    }

    const Json::Value& cells = mod["cells"];
    for (const auto& cname : cells.getMemberNames()) {
        const auto& cval = cells[cname];
        Cell c;
        c._name = cname;
        c._hide = cval["hide_name"].asInt() != 0;
        c._type = cval["type"].asString();
        const auto& pdirs = cval["port_directions"];
        for (const auto& dname : pdirs.getMemberNames()) {
            c._port_directions.emplace(dname, str2dir(pdirs[dname].asString()));
        }
        const auto& conns = cval["connections"];
        for (const auto& k : conns.getMemberNames()) {
            c._connections.emplace(k, parse_bits(conns[k]));
        }
        _module._cells.emplace(cname, std::move(c));
    }

    return _module;
}


void Reader::test_read() {
    using namespace global;
    std::string debug_file{"./debug.log"};
    global::log_info("Testing json2module() ...");
    global::log_info(this->_module.to_string());
    global::log_info("Test end");
}


}
