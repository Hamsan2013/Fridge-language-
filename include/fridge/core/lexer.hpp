#pragma once
#include "fridge/core/token.hpp"
#include "fridge/core/diagnostic.hpp"
#include <string>
#include <vector>

namespace fridge {

class Lexer {
public:
    Lexer(const std::string& source, DiagnosticEngine& diag);
    std::vector<Token> tokenize();

private:
    std::string src;
    DiagnosticEngine& diagnostics;
    size_t pos = 0;
    int line = 1;
    int col = 1;

    bool isAtEnd() const;
    char peek() const;
    char peekNext() const;
    char advance();
    void skipWhitespaceAndComments();
    Token readString();
    Token readNumber();
    Token readIdentifierOrKeyword();
    static std::string toLower(const std::string& s);
};

} // namespace fridge
