#pragma once
#include <string>

namespace fridge {

enum class ValueType {
    STRING,
    NUMBER,
    BOOLEAN,
    NOTHING
};

struct Value {
    ValueType type = ValueType::NOTHING;
    std::string strVal;
    double numVal = 0.0;
    bool boolVal = false;

    static Value fromString(const std::string& s);
    static Value fromNumber(double n);
    static Value fromBool(bool b);
    static Value nothing();

    std::string toString() const;
    bool isTruthy() const;
};

} // namespace fridge
