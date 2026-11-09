#include "fridge/runtime/environment.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace fridge {

std::string Environment::normalize(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return str;
}

Environment::Environment(std::shared_ptr<Environment> parent)
    : parent_(std::move(parent)) {}

void Environment::define(const std::string& name, Value val) {
    bindings_[normalize(name)] = std::move(val);
}

void Environment::assign(const std::string& name, Value val) {
    std::string key = normalize(name);
    auto it = bindings_.find(key);
    if (it != bindings_.end()) {
        it->second = std::move(val);
        return;
    }
    if (parent_) {
        parent_->assign(name, std::move(val));
        return;
    }
    throw std::runtime_error("FRG-NAME-001: Assignment to undeclared variable '" + name + "'. Use 'remember' first.");
}

Value Environment::get(const std::string& name) const {
    std::string key = normalize(name);
    auto it = bindings_.find(key);
    if (it != bindings_.end()) {
        return it->second;
    }
    if (parent_) {
        return parent_->get(name);
    }
    throw std::runtime_error("FRG-NAME-002: Unknown variable '" + name + "'. Verify spelling.");
}

bool Environment::exists(const std::string& name) const {
    std::string key = normalize(name);
    if (bindings_.find(key) != bindings_.end()) return true;
    if (parent_) return parent_->exists(name);
    return false;
}

} // namespace fridge
