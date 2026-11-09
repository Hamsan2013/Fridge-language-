#include "fridge/core/lexer.hpp"
#include "fridge/core/diagnostic.hpp"
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace fridge {

std::string_view token_type_name(TokenType type) {
    switch (type) {
        case TokenType::Identifier: return "Identifier";
        case TokenType::NumberLiteral: return "NumberLiteral";
        case TokenType::StringLiteral: return "StringLiteral";
        case TokenType::BooleanLiteral: return "BooleanLiteral";
        case TokenType::NothingLiteral: return "NothingLiteral";
        case TokenType::Newline: return "Newline";
        case TokenType::EndOfFile: return "EndOfFile";
        default: return "Keyword/Operator";
    }
}

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"remember", TokenType::Remember},
    {"as", TokenType::As},
    {"set", TokenType::Set},
    {"to", TokenType::To},
    {"becomes", TokenType::Becomes},
    {"show", TokenType::Show},
    {"ask", TokenType::Ask},
    {"into", TokenType::Into},
    {"if", TokenType::If},
    {"otherwise", TokenType::Otherwise},
    {"end", TokenType::End},
    {"repeat", TokenType::Repeat},
    {"times", TokenType::Times},
    {"while", TokenType::While},
    {"for", TokenType::For},
    {"each", TokenType::Each},
    {"in", TokenType::In},
    {"create", TokenType::Create},
    {"function", TokenType::Function},
    {"called", TokenType::Called},
    {"taking", TokenType::Taking},
    {"and", TokenType::And},
    {"or", TokenType::Or},
    {"not", TokenType::Not},
    {"returning", TokenType::Returning},
    {"return", TokenType::Return},
    {"call", TokenType::Call},
    {"with", TokenType::With},
    {"listen", TokenType::Listen},
    {"when", TokenType::When},
    {"child", TokenType::Child},
    {"add", TokenType::Add},
    {"subtract", TokenType::Subtract},
    {"multiply", TokenType::Multiply},
    {"divide", TokenType::Divide},
    {"by", TokenType::By},
    {"from", TokenType::From},
    {"list", TokenType::List},
    {"map", TokenType::Map},
    {"containing", TokenType::Containing},
    {"first", TokenType::First},
    {"last", TokenType::Last},
    {"item", TokenType::Item},
    {"of", TokenType::Of},
    {"wait", TokenType::Wait},
    {"seconds", TokenType::Seconds},
    {"milliseconds", TokenType::Milliseconds},
    {"save", TokenType::Save},
    {"read", TokenType::Read},
    {"file", TokenType::File},
    {"random", TokenType::Random},
    {"between", TokenType::Between},
    {"break", TokenType::Break},
    {"continue", TokenType::Continue},
    {"true", TokenType::BooleanLiteral},
    {"false", TokenType::BooleanLiteral},
    {"nothing", TokenType::NothingLiteral}
};

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

char Lexer::peek() const { return is_at_end() ? '\0' : source_[pos_]; }

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') {
        line_++;
        col_ = 1;
    } else {
        col_++;
    }
    return c;
}

bool Lexer::is_at_end() const { return pos_ >= source_.size(); }

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!is_at_end()) {
        char c = peek();

        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
            continue;
        }

        SourceLocation loc{"source.frg", line_, col_};

        if (c == '\n') {
            advance();
            tokens.push_back({TokenType::Newline, "\n", loc});
            continue;
        }

        // Single-line Comments: // or "note"
        if (c == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
            while (!is_at_end() && peek() != '\n') advance();
            continue;
        }

        // Block Comments: /* ... */
        if (c == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '*') {
            advance(); advance();
            while (!is_at_end()) {
                if (peek() == '*' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
            continue;
        }

        // Delimiters
        if (c == '{') { advance(); tokens.push_back({TokenType::LeftBrace, "{", loc}); continue; }
        if (c == '}') { advance(); tokens.push_back({TokenType::RightBrace, "}", loc}); continue; }
        if (c == '(') { advance(); tokens.push_back({TokenType::LeftParen, "(", loc}); continue; }
        if (c == ')') { advance(); tokens.push_back({TokenType::RightParen, ")", loc}); continue; }
        if (c == ',') { advance(); tokens.push_back({TokenType::Comma, ",", loc}); continue; }
        if (c == ':') { advance(); tokens.push_back({TokenType::Colon, ":", loc}); continue; }

        // Strings
        if (c == '"') {
            advance();
            std::string str;
            while (!is_at_end() && peek() != '"') {
                if (peek() == '\\' && pos_ + 1 < source_.size()) {
                    advance();
                    char esc = advance();
                    if (esc == 'n') str += '\n';
                    else if (esc == 't') str += '\t';
                    else str += esc;
                } else {
                    str += advance();
                }
            }
            if (is_at_end()) {
                DiagnosticEngine::instance().error("FRG-LEX-001", "Unterminated string literal.", loc, "Close the string with a double quote (\").");
                break;
            }
            advance(); // consume closing quote
            tokens.push_back({TokenType::StringLiteral, str, loc});
            continue;
        }

        // Numbers
        if (std::isdigit(static_cast<unsigned char>(c))) {
            std::string num;
            while (!is_at_end() && (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '.')) {
                num += advance();
            }
            tokens.push_back({TokenType::NumberLiteral, num, loc});
            continue;
        }

        // Identifiers and Keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string word;
            while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
                word += advance();
            }

            std::string norm = word;
            std::transform(norm.begin(), norm.end(), norm.begin(), [](unsigned char ch){ return static_cast<char>(std::tolower(ch)); });

            if (norm == "note") {
                while (!is_at_end() && peek() != '\n') advance();
                continue;
            }

            auto it = KEYWORDS.find(norm);
            if (it != KEYWORDS.end()) {
                tokens.push_back({it->second, word, loc});
            } else {
                tokens.push_back({TokenType::Identifier, word, loc});
            }
            continue;
        }

        // Unrecognized characters
        advance();
        DiagnosticEngine::instance().error("FRG-LEX-002", std::string("Unexpected character '") + c + "'.", loc);
    }

    tokens.push_back({TokenType::EndOfFile, "", {"source.frg", line_, col_}});
    return tokens;
}

} // namespace fridge
