#pragma once
#include <string>
#include <string_view>

namespace fridge {

struct SourceLocation {
    std::string filename = "source.frg";
    int line = 1;
    int column = 1;
};

enum class TokenType {
    // Literals & Identifiers
    Identifier,
    NumberLiteral,
    StringLiteral,
    BooleanLiteral,
    NothingLiteral,

    // Keywords
    Remember, As, Set, To, Becomes, Show, Ask, Into,
    If, Otherwise, End, Repeat, Times, While, For, Each, In,
    Create, Function, Called, Taking, And, Returning, Return, Call, With,
    Listen, When, Child, Add, Subtract, Multiply, Divide, By, From,
    List, Map, Containing, First, Last, Item, Of, NumberOf,
    Wait, Seconds, Milliseconds, Save, Read, File, Random, Between,
    Break, Continue,

    // Operators
    Plus, Minus, MultipliedBy, DividedBy, Modulo,
    Is, IsNot, GreaterThan, LessThan, AtLeast, AtMost, EqualTo,
    JoinedWith, Not, Or,

    // Delimiters
    Newline,
    LeftBrace, RightBrace,
    LeftParen, RightParen,
    Comma,
    Colon,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;
};

std::string_view token_type_name(TokenType type);

} // namespace fridge
