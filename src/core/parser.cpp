#include "fridge/core/parser.hpp"
#include <algorithm>
#include <cctype>

namespace fridge {

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

std::vector<Statement> Parser::parse() {
    std::vector<Statement> statements;
    std::vector<Token> current_line;

    for (const auto& tok : tokens_) {
        if (tok.type == TokenType::Newline || tok.type == TokenType::EndOfFile) {
            if (!current_line.empty()) {
                Statement stmt;
                stmt.action = to_lower(current_line.front().lexeme);
                stmt.line = current_line.front().line;
                stmt.tokens = current_line;
                statements.push_back(std::move(stmt));
                current_line.clear();
            }
        } else {
            current_line.push_back(tok);
        }
    }

    return statements;
}

} // namespace fridge
