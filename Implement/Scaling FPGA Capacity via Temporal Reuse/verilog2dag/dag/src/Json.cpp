#include "Json.h"
#include <stdexcept>
#include <cctype>

namespace json {

void Parser::skip_ws() {
    while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) pos_++;
}

bool Parser::consume(char c) {
    if (pos_ < text_.size() && text_[pos_] == c) { pos_++; return true; }
    return false;
}

bool Parser::peek(char c) const { return pos_ < text_.size() && text_[pos_] == c; }

char Parser::current() const { return pos_ < text_.size() ? text_[pos_] : '\0'; }

Value Parser::parse() { return parse_value(); }

Value Parser::parse_value() {
    skip_ws();
    char c = current();
    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == '"') return parse_string();
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
    if (text_.compare(pos_, 4, "true") == 0) return parse_literal("true", Value{Type::Bool});
    if (text_.compare(pos_, 5, "false") == 0) return parse_literal("false", Value{Type::Bool});
    if (text_.compare(pos_, 4, "null") == 0) return parse_literal("null", Value{Type::Null});
    throw std::runtime_error("Invalid JSON value at pos " + std::to_string(pos_));
}

Value Parser::parse_literal(const std::string& lit, Value v) {
    if (text_.compare(pos_, lit.size(), lit) != 0)
        throw std::runtime_error("Expected literal '" + lit + "' at pos " + std::to_string(pos_));
    pos_ += lit.size();
    if (lit == "true") { v.type = Type::Bool; v.b = true; }
    else if (lit == "false") { v.type = Type::Bool; v.b = false; }
    else { v.type = Type::Null; }
    return v;
}

Value Parser::parse_object() {
    Value v; v.type = Type::Object;
    consume('{'); skip_ws();
    if (peek('}')) { consume('}'); return v; }
    while (true) {
        skip_ws();
        std::string key = parse_string_raw();
        skip_ws();
        if (!consume(':')) throw std::runtime_error("Expected ':' in object at pos " + std::to_string(pos_));
        Value val = parse_value();
        v.obj.emplace(std::move(key), std::move(val));
        skip_ws();
        if (consume('}')) break;
        if (!consume(',')) throw std::runtime_error("Expected ',' in object at pos " + std::to_string(pos_));
    }
    return v;
}

Value Parser::parse_array() {
    Value v; v.type = Type::Array;
    consume('['); skip_ws();
    if (peek(']')) { consume(']'); return v; }
    while (true) {
        Value elem = parse_value();
        v.arr.emplace_back(std::move(elem));
        skip_ws();
        if (consume(']')) break;
        if (!consume(',')) throw std::runtime_error("Expected ',' in array at pos " + std::to_string(pos_));
    }
    return v;
}

std::string Parser::parse_string_raw() {
    if (!consume('"')) throw std::runtime_error("Expected '\"' at pos " + std::to_string(pos_));
    std::string out;
    while (pos_ < text_.size()) {
        char c = text_[pos_++];
        if (c == '"') break;
        if (c == '\\') {
            if (pos_ >= text_.size()) throw std::runtime_error("Invalid escape at end of string");
            char e = text_[pos_++];
            switch (e) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    // 简化处理：跳过4位十六进制，不做真正的 Unicode 解码
                    if (pos_ + 4 > text_.size()) throw std::runtime_error("Invalid unicode escape");
                    pos_ += 4; // 跳过
                    // 用占位符表示
                    out.push_back('?');
                    break;
                }
                default: throw std::runtime_error("Invalid escape sequence");
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
}

Value Parser::parse_string() {
    Value v; v.type = Type::String; v.s = parse_string_raw();
    return v;
}

Value Parser::parse_number() {
    size_t start = pos_;
    if (peek('-')) pos_++;
    while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) pos_++;
    bool is_float = false;
    if (pos_ < text_.size() && text_[pos_] == '.') {
        is_float = true; pos_++;
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) pos_++;
    }
    if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
        is_float = true; pos_++;
        if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) pos_++;
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) pos_++;
    }
    std::string num = text_.substr(start, pos_ - start);
    Value v;
    if (!is_float) {
        v.type = Type::NumberInt;
        v.i = std::stoll(num);
    } else {
        v.type = Type::NumberFloat;
        v.f = std::stod(num);
    }
    return v;
}

} // namespace json

