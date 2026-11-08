#pragma once
#include <string>

namespace fridge {

enum class TokenType {
    Word,
    Number,
    Text,
    Newline,
    LBrace,
    RBrace,
    LParen,
    RParen,
    Comma,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

} // namespace fridge
