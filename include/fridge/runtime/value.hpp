#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace fridge {

enum class ValueType {
    Nothing,
    Number,
    Text,
    Boolean,
    List,
    Map
};

struct Value;
using ListPtr = std::shared_ptr<std::vector<Value>>;
using MapPtr = std::shared_ptr<std::unordered_map<std::string, Value>>;

struct Value {
    using VariantType = std::variant<
        std::monostate, // Nothing
        double,         // Number
        std::string,    // Text
        bool,           // Boolean
        ListPtr,        // List
        MapPtr          // Map
    >;

    VariantType data;

    Value() : data(std::monostate{}) {}
    Value(double n) : data(n) {}
    Value(std::string s) : data(std::move(s)) {}
    Value(const char* s) : data(std::string(s)) {}
    Value(bool b) : data(b) {}
    Value(ListPtr l) : data(std::move(l)) {}
    Value(MapPtr m) : data(std::move(m)) {}

    ValueType type() const;
    bool is_nothing() const { return std::holds_alternative<std::monostate>(data); }
    bool is_number() const { return std::holds_alternative<double>(data); }
    bool is_text() const { return std::holds_alternative<std::string>(data); }
    bool is_boolean() const { return std::holds_alternative<bool>(data); }
    bool is_list() const { return std::holds_alternative<ListPtr>(data); }
    bool is_map() const { return std::holds_alternative<MapPtr>(data); }

    double as_number() const;
    const std::string& as_text() const;
    bool as_boolean() const;
    ListPtr as_list() const;
    MapPtr as_map() const;

    std::string to_string() const;
    bool equals(const Value& other) const;
};

Value make_list(std::vector<Value> elements = {});
Value make_map();

} // namespace fridge
