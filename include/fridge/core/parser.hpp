#pragma once
#include "fridge/core/ast.hpp"
#include <vector>

namespace fridge {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::vector<std::unique_ptr<Stmt>> parse_program();

private:
    std::vector<Token> tokens_;
    size_t current_ = 0;

    const Token& peek() const;
    const Token& previous() const;
    bool is_at_end() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& err_code, const std::string& err_msg, const std::string& suggestion = "");
    void skip_newlines();

    // Expression Grammar (Pratt-style Precedence Levels)
    std::unique_ptr<Expr> parse_expression();
    std::unique_ptr<Expr> parse_or();
    std::unique_ptr<Expr> parse_and();
    std::unique_ptr<Expr> parse_not();
    std::unique_ptr<Expr> parse_comparison();
    std::unique_ptr<Expr> parse_joined();
    std::unique_ptr<Expr> parse_additive();
    std::unique_ptr<Expr> parse_multiplicative();
    std::unique_ptr<Expr> parse_unary();
    std::unique_ptr<Expr> parse_primary();

    // Statement Grammar
    std::unique_ptr<Stmt> parse_statement();
    std::unique_ptr<Stmt> parse_remember();
    std::unique_ptr<Stmt> parse_set();
    std::unique_ptr<Stmt> parse_show();
    std::unique_ptr<Stmt> parse_if();
    std::unique_ptr<Stmt> parse_repeat();
    std::unique_ptr<Stmt> parse_while();
    std::unique_ptr<Stmt> parse_for();
    std::unique_ptr<Stmt> parse_function();
    std::unique_ptr<Stmt> parse_ask();
    std::unique_ptr<Stmt> parse_math_mutation();
};

} // namespace fridge
