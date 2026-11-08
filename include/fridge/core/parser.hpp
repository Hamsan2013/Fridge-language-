#pragma once
#include "fridge/core/token.hpp"
#include <string>
#include <vector>

namespace fridge {

struct Statement {
    std::string action;
    std::vector<Token> tokens;
    int line = 1;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::vector<Statement> parse();
    bool has_errors() const { return !errors_.empty(); }
    const std::vector<std::string>& errors() const { return errors_; }

private:
    std::vector<Token> tokens_;
    std::vector<std::string> errors_;
};

} // namespace fridge
