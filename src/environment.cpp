#include "fridge/runtime/environment.hpp"

namespace fridge {

void Environment::define(const std::string& name, const Value& val) {
    values[name] = val;
}

void Environment::assign(const std::string& name, const Value& val) {
    values[name] = val;
}

Value Environment::get(const std::string& name) const {
    auto it = values.find(name);
    if (it != values.end()) {
        return it->second;
    }
    return Value::fromString(name);
}

bool Environment::exists(const std::string& name) const {
    return values.find(name) != values.end();
}

} // namespace fridge
