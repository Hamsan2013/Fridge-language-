#pragma once
#include "fridge/core/token.hpp"
#include <string>
#include <vector>

namespace fridge {

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string source_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    char peek() const;
    char advance();
    bool is_at_end() const;
};

} // namespace fridge
