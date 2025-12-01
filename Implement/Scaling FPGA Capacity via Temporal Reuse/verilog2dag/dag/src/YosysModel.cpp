#include "YosysModel.h"
#include <stdexcept>

static const json::Value* expect_object_key(const json::Value& obj, const char* key) {
    const json::Value* v = obj.get(key);
    if (!v) throw std::runtime_error(std::string("Missing key: ") + key);
    return v;
}

std::vector<int> YosysJsonReader::to_int_list(const json::Value& v) {
    std::vector<int> out;
    if (!v.is_array()) return out;
    out.reserve(v.arr.size());
    for (const auto& e : v.arr) {
        if (e.is_int()) out.push_back(static_cast<int>(e.i));
        else if (e.is_float()) out.push_back(static_cast<int>(e.f));
    }
    return out;
}

YDesign YosysJsonReader::read() {
    YDesign d;
    if (!root_.is_object()) throw std::runtime_error("Root JSON must be an object");
    const json::Value* modules = root_.get("modules");
    if (!modules || !modules->is_object()) throw std::runtime_error("'modules' must be an object");

    for (const auto& kv : modules->obj) {
        YModule m; m.name = kv.first;
        const json::Value& mobj = kv.second;
        // ports
        const json::Value* ports = mobj.get("ports");
        if (ports && ports->is_object()) {
            for (const auto& pkv : ports->obj) {
                YPort p; p.name = pkv.first;
                const json::Value& pobj = pkv.second;
                const json::Value* dir = pobj.get("direction");
                if (dir && dir->is_string()) p.direction = dir->s;
                const json::Value* bits = pobj.get("bits");
                if (bits) p.bits = to_int_list(*bits);
                m.ports.emplace(p.name, std::move(p));
            }
        }

        // cells
        const json::Value* cells = mobj.get("cells");
        if (cells && cells->is_object()) {
            for (const auto& ckv : cells->obj) {
                YCell c; c.name = ckv.first;
                const json::Value& cobj = ckv.second;
                const json::Value* type = cobj.get("type");
                if (type && type->is_string()) c.type = type->s;
                const json::Value* conns = cobj.get("connections");
                if (conns && conns->is_object()) {
                    for (const auto& xkv : conns->obj) {
                        c.connections.emplace(xkv.first, to_int_list(xkv.second));
                    }
                }
                const json::Value* pdirs = cobj.get("port_directions");
                if (pdirs && pdirs->is_object()) {
                    for (const auto& xkv : pdirs->obj) {
                        if (xkv.second.is_string()) c.port_directions.emplace(xkv.first, xkv.second.s);
                    }
                }
                m.cells.emplace(c.name, std::move(c));
            }
        }
        d.modules.emplace(m.name, std::move(m));
        const json::Value* attrs = mobj.get("attributes");
        if (attrs && attrs->is_object()) {
            const json::Value* top = attrs->get("top");
            if (top) d.top = kv.first;
        }
    }
    if (d.top.empty() && !d.modules.empty()) d.top = d.modules.begin()->first;
    return d;
}

