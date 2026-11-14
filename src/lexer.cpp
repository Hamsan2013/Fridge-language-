#include "fridge/core/lexer.hpp"
#include <cctype>
#include <unordered_map>

namespace fridge {

Lexer::Lexer(const std::string& source, DiagnosticEngine& diag)
    : src(source), diagnostics(diag) {}

bool Lexer::isAtEnd() const {
    return pos >= src.size();
}

char Lexer::peek() const {
    return isAtEnd() ? '\0' : src[pos];
}

char Lexer::peekNext() const {
    return (pos + 1 >= src.size()) ? '\0' : src[pos + 1];
}

char Lexer::advance() {
    char c = src[pos++];
    if (c == '\n') {
        line++;
        col = 1;
    } else {
        col++;
    }
    return c;
}

std::string Lexer::toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peekNext() == '/') {
            while (!isAtEnd() && peek() != '\n') advance();
        } else if (c == '/' && peekNext() == '*') {
            advance(); advance();
            while (!isAtEnd() && !(peek() == '*' && peekNext() == '/')) advance();
            if (!isAtEnd()) { advance(); advance(); }
        } else {
            break;
        }
    }
}

Token Lexer::readString() {
    int startLine = line;
    int startCol = col;
    std::string val;
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\' && peekNext() != '\0') advance();
        val += advance();
    }
    if (isAtEnd()) {
        diagnostics.report(Severity::ERROR, startLine, startCol, "Unclosed string literal.");
    } else {
        advance(); // closing '"'
    }
    return { TokenType::STRING, val, startLine, startCol };
}

Token Lexer::readNumber() {
    int startLine = line;
    int startCol = col;
    std::string val;
    if (peek() == '-') val += advance();
    while (!isAtEnd() && (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '.')) {
        val += advance();
    }
    return { TokenType::NUMBER, val, startLine, startCol };
}

Token Lexer::readIdentifierOrKeyword() {
    int startLine = line;
    int startCol = col;
    std::string val;
    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        val += advance();
    }

    std::string lower = toLower(val);
    if (lower == "note") {
        while (!isAtEnd() && peek() != '\n') advance();
        return { TokenType::IDENTIFIER, "", startLine, startCol };
    }

    static const std::unordered_map<std::string, TokenType> keywords = {
        {"remember",  TokenType::REMEMBER},
        {"rem",       TokenType::REMEMBER},
        {"as",        TokenType::AS},
        {"set",       TokenType::SET},
        {"to",        TokenType::TO},
        {"show",      TokenType::SHOW},
        {"ask",       TokenType::ASK},
        {"joined",    TokenType::JOINED},
        {"with",      TokenType::WITH},
        {"if",        TokenType::IF},
        {"is",        TokenType::IS},
        {"otherwise", TokenType::OTHERWISE},
        {"repeat",    TokenType::REPEAT},
        {"times",     TokenType::TIMES},
        {"while",     TokenType::WHILE},
        {"function",  TokenType::FUNCTION},
        {"fnc",       TokenType::FUNCTION},
        {"called",    TokenType::CALLED},
        {"returning", TokenType::RETURNING},
        {"end",       TokenType::END},
        {"attempt",   TokenType::ATTEMPT},
        {"error",     TokenType::ERROR},
        {"stop",      TokenType::STOP},
        {"exit",      TokenType::EXIT},
        {"delete",    TokenType::DELETE_KW},
        {"del",       TokenType::DELETE_KW},
        {"create",    TokenType::CREATE},
        {"read",      TokenType::READ},
        {"add",       TokenType::ADD},
        {"file",      TokenType::FILE_KW},
        {"into",      TokenType::INTO},
        {"from",      TokenType::FROM},
        {"load",      TokenType::LOAD},
        {"model",     TokenType::MODEL},
        {"obj",       TokenType::OBJ},
        {"sprite",    TokenType::SPRITE},
        {"draw",      TokenType::DRAW},
        {"rectangle", TokenType::RECTANGLE},
        {"circle",    TokenType::CIRCLE},
        {"at",        TokenType::AT},
        {"width",     TokenType::WIDTH},
        {"height",    TokenType::HEIGHT},
        {"radius",    TokenType::RADIUS},
        {"color",     TokenType::COLOR},
        {"true",      TokenType::BOOLEAN},
        {"false",     TokenType::BOOLEAN},
        {"nothing",   TokenType::NOTHING}
    };

    auto it = keywords.find(lower);
    if (it != keywords.end()) {
        return { it->second, val, startLine, startCol };
    }
    return { TokenType::IDENTIFIER, val, startLine, startCol };
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;

        int startLine = line;
        int startCol = col;
        char c = peek();

        if (c == '{') { advance(); tokens.push_back({ TokenType::LBRACE, "{", startLine, startCol }); }
        else if (c == '}') { advance(); tokens.push_back({ TokenType::RBRACE, "}", startLine, startCol }); }
        else if (c == '(') { advance(); tokens.push_back({ TokenType::LPAREN, "(", startLine, startCol }); }
        else if (c == ')') { advance(); tokens.push_back({ TokenType::RPAREN, ")", startLine, startCol }); }
        else if (c == ',') { advance(); tokens.push_back({ TokenType::COMMA, ",", startLine, startCol }); }
        else if (c == '+') { advance(); tokens.push_back({ TokenType::PLUS, "+", startLine, startCol }); }
        else if (c == '*') { advance(); tokens.push_back({ TokenType::STAR, "*", startLine, startCol }); }
        else if (c == '/') { advance(); tokens.push_back({ TokenType::SLASH, "/", startLine, startCol }); }
        else if (c == '-') {
            if (std::isdigit(static_cast<unsigned char>(peekNext()))) {
                tokens.push_back(readNumber());
            } else {
                advance();
                tokens.push_back({ TokenType::MINUS, "-", startLine, startCol });
            }
        }
        else if (c == '"') {
            advance();
            tokens.push_back(readString());
        }
        else if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(readNumber());
        }
        else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            Token t = readIdentifierOrKeyword();
            if (!t.lexeme.empty()) tokens.push_back(t);
        }
        else {
            advance();
        }
    }
    tokens.push_back({ TokenType::END_OF_FILE, "", line, col });
    return tokens;
}

} // namespace fridge
