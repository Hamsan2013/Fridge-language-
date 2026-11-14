#include "fridge/runtime/value.hpp"
#include <cmath>
#include <cstdlib>

namespace fridge {

Value Value::fromString(const std::string& s) {
    if (s == "true") return fromBool(true);
    if (s == "false") return fromBool(false);
    if (s == "nothing") return nothing();

    char* end = nullptr;
    double num = std::strtod(s.c_str(), &end);
    if (end != s.c_str() && *end == '\0') {
        return fromNumber(num);
    }

    Value v;
    v.type = ValueType::STRING;
    v.strVal = s;
    return v;
}

Value Value::fromNumber(double n) {
    Value v;
    v.type = ValueType::NUMBER;
    v.numVal = n;
    if (std::floor(n) == n) {
        v.strVal = std::to_string(static_cast<long long>(n));
    } else {
        v.strVal = std::to_string(n);
    }
    return v;
}

Value Value::fromBool(bool b) {
    Value v;
    v.type = ValueType::BOOLEAN;
    v.boolVal = b;
    v.strVal = b ? "true" : "false";
    return v;
}

Value Value::nothing() {
    Value v;
    v.type = ValueType::NOTHING;
    v.strVal = "nothing";
    return v;
}

std::string Value::toString() const {
    return strVal;
}

bool Value::isTruthy() const {
    if (type == ValueType::NOTHING) return false;
    if (type == ValueType::BOOLEAN) return boolVal;
    if (type == ValueType::NUMBER) return numVal != 0.0;
    return !strVal.empty() && strVal != "false" && strVal != "0";
}

} // namespace fridge
