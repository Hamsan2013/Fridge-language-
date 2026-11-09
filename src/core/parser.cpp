#include "fridge/core/parser.hpp"
#include "fridge/core/diagnostic.hpp"
#include <algorithm>

namespace fridge {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

const Token& Parser::peek() const { return tokens_[current_]; }
const Token& Parser::previous() const { return tokens_[current_ - 1]; }
bool Parser::is_at_end() const { return peek().type == TokenType::EndOfFile; }

Token Parser::advance() {
    if (!is_at_end()) current_++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& err_code, const std::string& err_msg, const std::string& suggestion) {
    if (check(type)) return advance();
    DiagnosticEngine::instance().error(err_code, err_msg, peek().location, suggestion);
    return peek();
}

void Parser::skip_newlines() {
    while (match(TokenType::Newline));
}

std::vector<std::unique_ptr<Stmt>> Parser::parse_program() {
    std::vector<std::unique_ptr<Stmt>> stmts;
    skip_newlines();

    while (!is_at_end()) {
        auto stmt = parse_statement();
        if (stmt) {
            stmts.push_back(std::move(stmt));
        }
        skip_newlines();
    }
    return stmts;
}

std::unique_ptr<Stmt> Parser::parse_statement() {
    skip_newlines();
    if (is_at_end()) return nullptr;

    const auto& tok = peek();
    if (tok.type == TokenType::Show) return parse_show();
    if (tok.type == TokenType::Remember) return parse_remember();
    if (tok.type == TokenType::Set) return parse_set();
    if (tok.type == TokenType::If) return parse_if();
    if (tok.type == TokenType::Repeat) return parse_repeat();
    if (tok.type == TokenType::While) return parse_while();
    if (tok.type == TokenType::For) return parse_for();
    if (tok.type == TokenType::Create) return parse_function();
    if (tok.type == TokenType::Ask) return parse_ask();
    if (tok.type == TokenType::Add || tok.type == TokenType::Subtract ||
        tok.type == TokenType::Multiply || tok.type == TokenType::Divide) {
        return parse_math_mutation();
    }

    if (tok.type == TokenType::Return) {
        SourceLocation loc = advance().location;
        auto expr = parse_expression();
        return std::make_unique<ReturnStmt>(std::move(expr), loc);
    }

    // <var> becomes <expr>
    if (tok.type == TokenType::Identifier && current_ + 1 < tokens_.size()) {
        std::string name = tok.lexeme;
        SourceLocation loc = tok.location;
        if (tokens_[current_ + 1].type == TokenType::Becomes) {
            advance(); // consume identifier
            advance(); // consume becomes
            auto expr = parse_expression();
            return std::make_unique<AssignmentStmt>(name, std::move(expr), loc);
        }
    }

    // Generic fallback for extensible segments (child, wait, save, etc.)
    SourceLocation loc = advance().location;
    std::string cmd = previous().lexeme;
    std::vector<std::string> args;
    while (!check(TokenType::Newline) && !is_at_end()) {
        args.push_back(advance().lexeme);
    }
    return std::make_unique<GenericCommandStmt>(cmd, args, loc);
}

std::unique_ptr<Stmt> Parser::parse_show() {
    SourceLocation loc = advance().location; // consume 'show'
    auto expr = parse_expression();
    return std::make_unique<ShowStmt>(std::move(expr), loc);
}

std::unique_ptr<Stmt> Parser::parse_remember() {
    SourceLocation loc = advance().location; // consume 'remember'
    Token name = consume(TokenType::Identifier, "FRG-PARSE-002", "Expected variable name after 'remember'.", "Specify a valid identifier like 'score'.");
    consume(TokenType::As, "FRG-PARSE-003", "Expected 'as' after variable name.", "Follow 'remember <name>' with 'as'.");

    if (match(TokenType::List)) {
        consume(TokenType::Containing, "FRG-PARSE-004", "Expected 'containing' after 'list'.");
        std::vector<std::unique_ptr<Expr>> elements;
        if (!check(TokenType::Newline) && !is_at_end()) {
            do {
                elements.push_back(parse_expression());
            } while (match(TokenType::Comma));
        }
        return std::make_unique<ListDeclStmt>(name.lexeme, std::move(elements), loc);
    }

    auto init = parse_expression();
    return std::make_unique<VarDeclStmt>(name.lexeme, std::move(init), loc);
}

std::unique_ptr<Stmt> Parser::parse_set() {
    SourceLocation loc = advance().location; // consume 'set'
    Token name = consume(TokenType::Identifier, "FRG-PARSE-005", "Expected variable name after 'set'.");
    consume(TokenType::To, "FRG-PARSE-006", "Expected 'to' after variable name in assignment.");
    auto val = parse_expression();
    return std::make_unique<AssignmentStmt>(name.lexeme, std::move(val), loc);
}

std::unique_ptr<Stmt> Parser::parse_if() {
    SourceLocation loc = advance().location; // consume 'if'
    auto cond = parse_expression();
    skip_newlines();

    std::vector<std::unique_ptr<Stmt>> then_branch;
    std::vector<std::unique_ptr<Stmt>> else_branch;

    while (!check(TokenType::Otherwise) && !check(TokenType::End) && !is_at_end()) {
        then_branch.push_back(parse_statement());
        skip_newlines();
    }

    if (match(TokenType::Otherwise)) {
        skip_newlines();
        while (!check(TokenType::End) && !is_at_end()) {
            else_branch.push_back(parse_statement());
            skip_newlines();
        }
    }

    consume(TokenType::End, "FRG-PARSE-007", "Expected 'end if' to terminate if block.");
    if (check(TokenType::If)) advance();

    return std::make_unique<IfStmt>(std::move(cond), std::move(then_branch), std::move(else_branch), loc);
}

std::unique_ptr<Stmt> Parser::parse_repeat() {
    SourceLocation loc = advance().location; // consume 'repeat'
    auto count = parse_expression();
    consume(TokenType::Times, "FRG-PARSE-008", "Expected 'times' after repeat count.");
    skip_newlines();

    std::vector<std::unique_ptr<Stmt>> body;
    while (!check(TokenType::End) && !is_at_end()) {
        body.push_back(parse_statement());
        skip_newlines();
    }

    consume(TokenType::End, "FRG-PARSE-009", "Expected 'end repeat'.");
    if (check(TokenType::Repeat)) advance();

    return std::make_unique<RepeatStmt>(std::move(count), std::move(body), loc);
}

std::unique_ptr<Stmt> Parser::parse_while() {
    SourceLocation loc = advance().location; // consume 'while'
    auto cond = parse_expression();
    skip_newlines();

    std::vector<std::unique_ptr<Stmt>> body;
    while (!check(TokenType::End) && !is_at_end()) {
        body.push_back(parse_statement());
        skip_newlines();
    }

    consume(TokenType::End, "FRG-PARSE-010", "Expected 'end while'.");
    if (check(TokenType::While)) advance();

    return std::make_unique<WhileStmt>(std::move(cond), std::move(body), loc);
}

std::unique_ptr<Stmt> Parser::parse_for() {
    SourceLocation loc = advance().location; // consume 'for'
    consume(TokenType::Each, "FRG-PARSE-011", "Expected 'each' after 'for'.");
    Token it = consume(TokenType::Identifier, "FRG-PARSE-012", "Expected iterator name.");
    consume(TokenType::In, "FRG-PARSE-013", "Expected 'in' after iterator variable.");
    Token coll = consume(TokenType::Identifier, "FRG-PARSE-014", "Expected collection variable name.");
    skip_newlines();

    std::vector<std::unique_ptr<Stmt>> body;
    while (!check(TokenType::End) && !is_at_end()) {
        body.push_back(parse_statement());
        skip_newlines();
    }

    consume(TokenType::End, "FRG-PARSE-015", "Expected 'end for'.");
    if (check(TokenType::For)) advance();

    return std::make_unique<ForEachStmt>(it.lexeme, coll.lexeme, std::move(body), loc);
}

std::unique_ptr<Stmt> Parser::parse_function() {
    SourceLocation loc = advance().location; // consume 'create'
    if (check(TokenType::Function)) advance();
    if (check(TokenType::Called)) advance();

    Token name = consume(TokenType::Identifier, "FRG-PARSE-016", "Expected function name.");
    std::vector<std::string> params;

    if (match(TokenType::Taking)) {
        do {
            Token p = consume(TokenType::Identifier, "FRG-PARSE-017", "Expected parameter identifier.");
            params.push_back(p.lexeme);
        } while (match(TokenType::And) || match(TokenType::Comma));
    }

    if (match(TokenType::Returning)) {
        advance(); // consume return type (e.g. 'number', 'text')
    }

    skip_newlines();
    std::vector<std::unique_ptr<Stmt>> body;
    while (!check(TokenType::End) && !is_at_end()) {
        body.push_back(parse_statement());
        skip_newlines();
    }

    consume(TokenType::End, "FRG-PARSE-018", "Expected 'end function'.");
    if (check(TokenType::Function)) advance();

    return std::make_unique<FunctionDeclStmt>(name.lexeme, std::move(params), std::move(body), loc);
}

std::unique_ptr<Stmt> Parser::parse_ask() {
    SourceLocation loc = advance().location; // consume 'ask'
    std::unique_ptr<Expr> prompt = nullptr;
    if (check(TokenType::StringLiteral)) {
        prompt = std::make_unique<LiteralExpr>(advance());
    }
    consume(TokenType::Into, "FRG-PARSE-019", "Expected 'into' followed by target variable.");
    Token target = consume(TokenType::Identifier, "FRG-PARSE-020", "Expected variable name to receive input.");
    return std::make_unique<AskStmt>(std::move(prompt), target.lexeme, loc);
}

std::unique_ptr<Stmt> Parser::parse_math_mutation() {
    Token op_tok = advance();
    SourceLocation loc = op_tok.location;

    if (op_tok.type == TokenType::Add) {
        // Check if 3D 'add OBJ <name>'
        if (check(TokenType::Identifier) && peek().lexeme == "OBJ") {
            advance();
            std::string name = advance().lexeme;
            return std::make_unique<GenericCommandStmt>("add_obj", std::vector<std::string>{name}, loc);
        }
        auto amt = parse_expression();
        consume(TokenType::To, "FRG-PARSE-021", "Expected 'to' in add statement.");
        Token target = consume(TokenType::Identifier, "FRG-PARSE-022", "Expected target variable.");
        return std::make_unique<MathMutateStmt>(MathMutateStmt::Op::Add, target.lexeme, std::move(amt), loc);
    }
    if (op_tok.type == TokenType::Subtract) {
        auto amt = parse_expression();
        consume(TokenType::From, "FRG-PARSE-023", "Expected 'from' in subtract statement.");
        Token target = consume(TokenType::Identifier, "FRG-PARSE-024", "Expected target variable.");
        return std::make_unique<MathMutateStmt>(MathMutateStmt::Op::Subtract, target.lexeme, std::move(amt), loc);
    }
    if (op_tok.type == TokenType::Multiply) {
        Token target = consume(TokenType::Identifier, "FRG-PARSE-025", "Expected target variable.");
        consume(TokenType::By, "FRG-PARSE-026", "Expected 'by' in multiply statement.");
        auto amt = parse_expression();
        return std::make_unique<MathMutateStmt>(MathMutateStmt::Op::Multiply, target.lexeme, std::move(amt), loc);
    }
    if (op_tok.type == TokenType::Divide) {
        Token target = consume(TokenType::Identifier, "FRG-PARSE-027", "Expected target variable.");
        consume(TokenType::By, "FRG-PARSE-028", "Expected 'by' in divide statement.");
        auto amt = parse_expression();
        return std::make_unique<MathMutateStmt>(MathMutateStmt::Op::Divide, target.lexeme, std::move(amt), loc);
    }
    return nullptr;
}

// ----------------------------------------------------
// Pratt Precedence Expression Hierarchy
// ----------------------------------------------------

std::unique_ptr<Expr> Parser::parse_expression() {
    return parse_or();
}

std::unique_ptr<Expr> Parser::parse_or() {
    auto expr = parse_and();
    while (match(TokenType::Or)) {
        SourceLocation loc = previous().location;
        auto right = parse_and();
        expr = std::make_unique<BinaryExpr>(std::move(expr), TokenType::Or, std::move(right), loc);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_and() {
    auto expr = parse_not();
    while (match(TokenType::And)) {
        SourceLocation loc = previous().location;
        auto right = parse_not();
        expr = std::make_unique<BinaryExpr>(std::move(expr), TokenType::And, std::move(right), loc);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_not() {
    if (match(TokenType::Not)) {
        SourceLocation loc = previous().location;
        auto right = parse_comparison();
        return std::make_unique<UnaryExpr>(TokenType::Not, std::move(right), loc);
    }
    return parse_comparison();
}

std::unique_ptr<Expr> Parser::parse_comparison() {
    auto expr = parse_joined();

    // Multi-token English comparisons: "is greater than", "is less than", "is equal to", "is"
    if (match(TokenType::Is)) {
        SourceLocation loc = previous().location;
        TokenType op = TokenType::Is;
        if (match(TokenType::Not)) {
            op = TokenType::IsNot;
        } else if (match(TokenType::GreaterThan) || (check(TokenType::Identifier) && peek().lexeme == "greater")) {
            advance();
            if (check(TokenType::Identifier) && peek().lexeme == "than") advance();
            op = TokenType::GreaterThan;
        } else if (match(TokenType::LessThan) || (check(TokenType::Identifier) && peek().lexeme == "less")) {
            advance();
            if (check(TokenType::Identifier) && peek().lexeme == "than") advance();
            op = TokenType::LessThan;
        } else if (match(TokenType::EqualTo) || (check(TokenType::Identifier) && peek().lexeme == "equal")) {
            advance();
            if (check(TokenType::To)) advance();
            op = TokenType::EqualTo;
        }
        auto right = parse_joined();
        return std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right), loc);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_joined() {
    auto expr = parse_additive();
    while (check(TokenType::Identifier) && peek().lexeme == "joined") {
        SourceLocation loc = advance().location; // consume 'joined'
        if (check(TokenType::With)) advance(); // consume 'with'
        auto right = parse_additive();
        expr = std::make_unique<BinaryExpr>(std::move(expr), TokenType::JoinedWith, std::move(right), loc);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_additive() {
    auto expr = parse_multiplicative();
    while (match(TokenType::Plus) || match(TokenType::Minus) ||
           (check(TokenType::Identifier) && (peek().lexeme == "plus" || peek().lexeme == "minus"))) {
        SourceLocation loc = advance().location;
        TokenType op = (previous().lexeme == "plus") ? TokenType::Plus : TokenType::Minus;
        auto right = parse_multiplicative();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right), loc);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_multiplicative() {
    auto expr = parse_unary();
    while (check(TokenType::Identifier) && (peek().lexeme == "multiplied" || peek().lexeme == "divided" || peek().lexeme == "modulo")) {
        SourceLocation loc = advance().location;
        std::string op_name = previous().lexeme;
        TokenType op = TokenType::Modulo;
        if (op_name == "multiplied") {
            if (check(TokenType::By)) advance();
            op = TokenType::MultipliedBy;
        } else if (op_name == "divided") {
            if (check(TokenType::By)) advance();
            op = TokenType::DividedBy;
        }
        auto right = parse_unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right), loc);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_unary() {
    if (match(TokenType::Minus) || (check(TokenType::Identifier) && peek().lexeme == "minus")) {
        SourceLocation loc = advance().location;
        auto right = parse_primary();
        return std::make_unique<UnaryExpr>(TokenType::Minus, std::move(right), loc);
    }
    return parse_primary();
}

std::unique_ptr<Expr> Parser::parse_primary() {
    if (match(TokenType::NumberLiteral) || match(TokenType::StringLiteral) ||
        match(TokenType::BooleanLiteral) || match(TokenType::NothingLiteral)) {
        return std::make_unique<LiteralExpr>(previous());
    }

    if (match(TokenType::LeftParen)) {
        auto expr = parse_expression();
        consume(TokenType::RightParen, "FRG-PARSE-029", "Expected ')' to close expression.");
        return expr;
    }

    // Function calls: call <name> with arg1, arg2
    if (match(TokenType::Call)) {
        SourceLocation loc = previous().location;
        Token name = consume(TokenType::Identifier, "FRG-PARSE-030", "Expected function name after 'call'.");
        std::vector<std::unique_ptr<Expr>> args;
        if (match(TokenType::With)) {
            do {
                args.push_back(parse_expression());
            } while (match(TokenType::Comma));
        }
        return std::make_unique<CallExpr>(name.lexeme, std::move(args), loc);
    }

    // Collection Queries: first item in <list>, item X of <list>, number of items in <list>
    if (match(TokenType::First)) {
        SourceLocation loc = previous().location;
        if (match(TokenType::Item)) {
            if (match(TokenType::In)) {
                Token lst = consume(TokenType::Identifier, "FRG-PARSE-031", "Expected list identifier.");
                return std::make_unique<ListAccessExpr>(ListAccessExpr::AccessType::FirstItem, lst.lexeme, nullptr, loc);
            }
        }
    }
    if (match(TokenType::Last)) {
        SourceLocation loc = previous().location;
        if (match(TokenType::Item)) {
            if (match(TokenType::In)) {
                Token lst = consume(TokenType::Identifier, "FRG-PARSE-032", "Expected list identifier.");
                return std::make_unique<ListAccessExpr>(ListAccessExpr::AccessType::LastItem, lst.lexeme, nullptr, loc);
            }
        }
    }
    if (match(TokenType::Item)) {
        SourceLocation loc = previous().location;
        auto idx = parse_expression();
        consume(TokenType::Of, "FRG-PARSE-033", "Expected 'of' in list item access.");
        Token lst = consume(TokenType::Identifier, "FRG-PARSE-034", "Expected list identifier.");
        return std::make_unique<ListAccessExpr>(ListAccessExpr::AccessType::ItemAt, lst.lexeme, std::move(idx), loc);
    }

    if (match(TokenType::Identifier)) {
        return std::make_unique<VariableExpr>(previous());
    }

    DiagnosticEngine::instance().error("FRG-PARSE-035", "Expected expression but found '" + peek().lexeme + "'.", peek().location);
    advance();
    return nullptr;
}

} // namespace fridge
