#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

// 简易 JSON DOM 与解析器，支持对象、数组、字符串、数值、布尔与 null

namespace json {

enum class Type { Null, Bool, NumberInt, NumberFloat, String, Array, Object };

struct Value {
    Type type{Type::Null};
    bool b{};
    long long i{};
    double f{};
    std::string s;
    std::vector<Value> arr;
    std::unordered_map<std::string, Value> obj;

    bool is_object() const { return type == Type::Object; }
    bool is_array() const { return type == Type::Array; }
    bool is_string() const { return type == Type::String; }
    bool is_bool() const { return type == Type::Bool; }
    bool is_int() const { return type == Type::NumberInt; }
    bool is_float() const { return type == Type::NumberFloat; }

    const Value* get(const std::string& key) const {
        auto it = obj.find(key);
        if (it == obj.end()) return nullptr;
        return &it->second;
    }
};

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}
    Value parse();

private:
    const std::string& text_;
    size_t pos_ = 0;

    void skip_ws();
    bool consume(char c);
    bool peek(char c) const;
    char current() const;

    Value parse_value();
    Value parse_object();
    Value parse_array();
    Value parse_string();
    Value parse_number();
    Value parse_literal(const std::string& lit, Value v);

    std::string parse_string_raw();
};

} // namespace json

