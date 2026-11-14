#pragma once
#include "fridge/runtime/value.hpp"
#include <string>
#include <unordered_map>

namespace fridge {

class Environment {
public:
    void define(const std::string& name, const Value& val);
    void assign(const std::string& name, const Value& val);
    Value get(const std::string& name) const;
    bool exists(const std::string& name) const;

private:
    std::unordered_map<std::string, Value> values;
};

} // namespace fridge
