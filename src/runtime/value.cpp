#include "fridge/runtime/value.hpp"
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace fridge {

ValueType Value::type() const {
    switch (data.index()) {
        case 0: return ValueType::Nothing;
        case 1: return ValueType::Number;
        case 2: return ValueType::Text;
        case 3: return ValueType::Boolean;
        case 4: return ValueType::List;
        case 5: return ValueType::Map;
        default: return ValueType::Nothing;
    }
}

double Value::as_number() const {
    if (is_number()) return std::get<double>(data);
    if (is_text()) {
        try { return std::stod(std::get<std::string>(data)); }
        catch (...) { throw std::runtime_error("FRG-TYPE-001: Cannot convert text '" + std::get<std::string>(data) + "' to number."); }
    }
    if (is_boolean()) return std::get<bool>(data) ? 1.0 : 0.0;
    throw std::runtime_error("FRG-TYPE-002: Expected numeric value.");
}

const std::string& Value::as_text() const {
    if (is_text()) return std::get<std::string>(data);
    throw std::runtime_error("FRG-TYPE-003: Expected text string value.");
}

bool Value::as_boolean() const {
    if (is_boolean()) return std::get<bool>(data);
    if (is_number()) return std::get<double>(data) != 0.0;
    if (is_text()) return !std::get<std::string>(data).empty();
    if (is_nothing()) return false;
    return true;
}

ListPtr Value::as_list() const {
    if (is_list()) return std::get<ListPtr>(data);
    throw std::runtime_error("FRG-TYPE-004: Expected list reference.");
}

MapPtr Value::as_map() const {
    if (is_map()) return std::get<MapPtr>(data);
    throw std::runtime_error("FRG-TYPE-005: Expected map reference.");
}

std::string Value::to_string() const {
    if (is_nothing()) return "nothing";
    if (is_boolean()) return std::get<bool>(data) ? "true" : "false";
    if (is_number()) {
        double d = std::get<double>(data);
        if (std::floor(d) == d && !std::isinf(d)) {
            return std::to_string(static_cast<long long>(d));
        }
        std::ostringstream ss;
        ss << d;
        return ss.str();
    }
    if (is_text()) return std::get<std::string>(data);
    if (is_list()) {
        auto list = as_list();
        std::string s = "[";
        for (size_t i = 0; i < list->size(); ++i) {
            s += (*list)[i].to_string();
            if (i + 1 < list->size()) s += ", ";
        }
        s += "]";
        return s;
    }
    if (is_map()) return "<map>";
    return "unknown";
}

bool Value::equals(const Value& other) const {
    if (type() != other.type()) return false;
    if (is_nothing()) return true;
    if (is_boolean()) return as_boolean() == other.as_boolean();
    if (is_number()) return std::fabs(as_number() - other.as_number()) < 1e-9;
    if (is_text()) return as_text() == other.as_text();
    return false;
}

Value make_list(std::vector<Value> elements) {
    return Value(std::make_shared<std::vector<Value>>(std::move(elements)));
}

Value make_map() {
    return Value(std::make_shared<std::unordered_map<std::string, Value>>());
}

} // namespace fridge
