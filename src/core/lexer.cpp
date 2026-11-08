#include "fridge/core/lexer.hpp"
#include <cctype>

namespace fridge {

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

char Lexer::peek() const {
    return is_at_end() ? '\0' : source_[pos_];
}

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

bool Lexer::is_at_end() const {
    return pos_ >= source_.size();
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!is_at_end()) {
        char c = peek();

        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
            continue;
        }

        if (c == '\n') {
            int l = line_, col = col_;
            advance();
            tokens.push_back({TokenType::Newline, "\n", l, col});
            continue;
        }

        // Line comments: //
        if (c == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
            while (!is_at_end() && peek() != '\n') advance();
            continue;
        }

        // Block comments: /* ... */
        if (c == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '*') {
            advance(); advance();
            while (!is_at_end() && !(peek() == '*' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/')) {
                advance();
            }
            if (!is_at_end()) { advance(); advance(); }
            continue;
        }

        int start_line = line_;
        int start_col = col_;

        if (c == '{') { advance(); tokens.push_back({TokenType::LBrace, "{", start_line, start_col}); continue; }
        if (c == '}') { advance(); tokens.push_back({TokenType::RBrace, "}", start_line, start_col}); continue; }
        if (c == '(') { advance(); tokens.push_back({TokenType::LParen, "(", start_line, start_col}); continue; }
        if (c == ')') { advance(); tokens.push_back({TokenType::RParen, ")", start_line, start_col}); continue; }
        if (c == ',') { advance(); tokens.push_back({TokenType::Comma, ",", start_line, start_col}); continue; }

        if (c == '"') {
            advance(); // skip opening quote
            std::string str;
            while (!is_at_end() && peek() != '"') {
                if (peek() == '\\' && pos_ + 1 < source_.size()) {
                    advance();
                }
                str += advance();
            }
            if (!is_at_end()) advance(); // closing quote
            tokens.push_back({TokenType::Text, str, start_line, start_col});
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c)) || (c == '-' && pos_ + 1 < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_ + 1])))) {
            std::string num;
            num += advance();
            while (!is_at_end() && (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '.')) {
                num += advance();
            }
            tokens.push_back({TokenType::Number, num, start_line, start_col});
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string word;
            while (!is_at_end() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
                word += advance();
            }
            if (word == "note") {
                while (!is_at_end() && peek() != '\n') advance();
                continue;
            }
            tokens.push_back({TokenType::Word, word, start_line, start_col});
            continue;
        }

        advance();
    }

    tokens.push_back({TokenType::EndOfFile, "", line_, col_});
    return tokens;
}

} // namespace fridge
