#pragma once
#include "fridge/runtime/value.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace fridge {

class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> parent = nullptr);

    void define(const std::string& name, Value val);
    void assign(const std::string& name, Value val);
    Value get(const std::string& name) const;
    bool exists(const std::string& name) const;

private:
    std::unordered_map<std::string, Value> bindings_;
    std::shared_ptr<Environment> parent_;
    static std::string normalize(std::string str);
};

} // namespace fridge
