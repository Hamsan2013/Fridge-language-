#pragma once
#include "fridge/core/parser.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace fridge {

class Interpreter {
public:
    Interpreter();
    int execute(const std::vector<Statement>& program);

    void set_var(const std::string& name, const std::string& val);
    std::string get_var(const std::string& name) const;
    bool has_var(const std::string& name) const;

    std::string evaluate_tokens(const std::vector<Token>& tokens, size_t start_idx) const;

private:
    std::unordered_map<std::string, std::string> variables_;
};

} // namespace fridge
