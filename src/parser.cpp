#include "fridge/core/parser.hpp"

namespace fridge {

Parser::Parser(const std::vector<Token>& t, DiagnosticEngine& diag)
    : tokens(t), diagnostics(diag) {}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::consume(TokenType type, const std::string& errMsg) {
    if (check(type)) return advance();
    diagnostics.report(Severity::ERROR, peek().line, peek().column, errMsg);
    return peek();
}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> stmts;
    while (!isAtEnd()) {
        if (match(TokenType::RBRACE)) continue;
        StmtPtr s = parseStatement();
        if (s) stmts.push_back(s);
    }
    return stmts;
}

std::vector<StmtPtr> Parser::parseBlock() {
    std::vector<StmtPtr> stmts;
    if (match(TokenType::LBRACE)) {
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            StmtPtr s = parseStatement();
            if (s) stmts.push_back(s);
        }
        consume(TokenType::RBRACE, "Expected '}' to close block.");
    } else {
        while (!check(TokenType::END) && !check(TokenType::OTHERWISE) && !isAtEnd()) {
            StmtPtr s = parseStatement();
            if (s) stmts.push_back(s);
        }
        if (match(TokenType::END)) {
            if (!isAtEnd()) advance();
        }
    }
    return stmts;
}

StmtPtr Parser::parseStatement() {
    int ln = peek().line;
    StmtPtr stmt = nullptr;

    if (match(TokenType::REMEMBER)) stmt = parseVarDecl();
    else if (match(TokenType::SET)) stmt = parseSet();
    else if (match(TokenType::SHOW)) stmt = parseShow();
    else if (match(TokenType::ASK)) stmt = parseAsk();
    else if (match(TokenType::IF)) stmt = parseIf();
    else if (match(TokenType::REPEAT)) stmt = parseRepeat();
    else if (match(TokenType::FUNCTION)) stmt = parseFunction();
    else if (match(TokenType::ATTEMPT)) stmt = parseAttempt();
    else if (match(TokenType::STOP)) stmt = parseStop();
    else if (match(TokenType::CREATE)) stmt = parseCreate();
    else if (match(TokenType::READ)) stmt = parseRead();
    else if (match(TokenType::ADD)) stmt = parseAdd();
    else if (match(TokenType::DELETE_KW)) stmt = parseDelete();
    else if (match(TokenType::LOAD)) stmt = parseLoad();
    else if (match(TokenType::DRAW)) stmt = parseDraw();
    else if (match(TokenType::IDENTIFIER)) stmt = parseIdentOrCall();
    else advance();

    if (stmt) stmt->line = ln;
    return stmt;
}

StmtPtr Parser::parseVarDecl() {
    const Token& nameTok = consume(TokenType::IDENTIFIER, "Expected variable name after 'rem'/'remember'.");
    consume(TokenType::AS, "Expected 'as' in variable declaration.");
    auto node = std::make_shared<VarDeclStmt>();
    node->name = nameTok.lexeme;
    node->initializer = parseExpression();
    return node;
}

StmtPtr Parser::parseSet() {
    const Token& nameTok = consume(TokenType::IDENTIFIER, "Expected variable name after 'set'.");
    consume(TokenType::TO, "Expected 'to' after variable name.");
    auto node = std::make_shared<AssignStmt>();
    node->name = nameTok.lexeme;
    node->value = parseExpression();
    return node;
}

StmtPtr Parser::parseShow() {
    auto node = std::make_shared<ShowStmt>();
    node->expr = parseExpression();
    return node;
}

StmtPtr Parser::parseAsk() {
    auto node = std::make_shared<AskStmt>();
    node->prompt = parseExpression();
    consume(TokenType::AS, "Expected 'as' after ask prompt.");
    node->targetVar = consume(TokenType::IDENTIFIER, "Expected variable name after 'as'.").lexeme;
    return node;
}

StmtPtr Parser::parseIf() {
    auto node = std::make_shared<IfStmt>();
    node->condition = parseExpression();
    node->thenBranch = parseBlock();
    if (match(TokenType::OTHERWISE)) {
        node->elseBranch = parseBlock();
    }
    return node;
}

StmtPtr Parser::parseRepeat() {
    auto node = std::make_shared<RepeatStmt>();
    node->count = parseExpression();
    match(TokenType::TIMES);
    node->body = parseBlock();
    return node;
}

StmtPtr Parser::parseFunction() {
    match(TokenType::CALLED);
    auto node = std::make_shared<FunctionStmt>();
    node->name = consume(TokenType::IDENTIFIER, "Expected function name.").lexeme;

    if (match(TokenType::LPAREN)) {
        if (!check(TokenType::RPAREN)) {
            do {
                node->params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name inside '()'.").lexeme);
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RPAREN, "Expected ')' after function parameters.");
    }
    node->body = parseBlock();
    return node;
}

StmtPtr Parser::parseAttempt() {
    auto node = std::make_shared<AttemptStmt>();
    node->tryBody = parseBlock();

    if (match(TokenType::ERROR)) {
        consume(TokenType::LPAREN, "FRIDGE v0.5.0 Rule: 'error' catch block must use empty small brackets 'error()'.");
        consume(TokenType::RPAREN, "FRIDGE v0.5.0 Rule: 'error()' brackets must be empty.");
        node->errorBody = parseBlock();
    } else {
        diagnostics.report(Severity::ERROR, peek().line, peek().column, "Expected 'error() { ... }' after 'attempt' block.");
    }
    return node;
}

StmtPtr Parser::parseStop() {
    match(TokenType::WITH);
    match(TokenType::ERROR);
    auto node = std::make_shared<StopErrorStmt>();
    node->message = parseExpression();
    return node;
}

StmtPtr Parser::parseCreate() {
    if (match(TokenType::FUNCTION)) return parseFunction();
    if (match(TokenType::FILE_KW)) {
        auto node = std::make_shared<FileOpStmt>();
        node->action = FileAction::CREATE_WRITE;
        node->pathExpr = parseExpression();
        consume(TokenType::WITH, "Expected 'with' in 'create file <path> with <content>'.");
        node->contentExpr = parseExpression();
        return node;
    }
    while (!isAtEnd() && peek().line == previous().line) advance();
    return nullptr;
}

StmtPtr Parser::parseRead() {
    consume(TokenType::FILE_KW, "Expected 'file' after 'read'.");
    auto node = std::make_shared<FileOpStmt>();
    node->action = FileAction::READ_INTO;
    node->pathExpr = parseExpression();
    consume(TokenType::INTO, "Expected 'into' after file path.");
    node->targetVar = consume(TokenType::IDENTIFIER, "Expected variable name after 'into'.").lexeme;
    return node;
}

StmtPtr Parser::parseAdd() {
    if (match(TokenType::OBJ)) {
        auto node = std::make_shared<Engine3DStmt>();
        node->isModelAsset = false;
        node->objectName = consume(TokenType::IDENTIFIER, "Expected object name after 'add OBJ'.").lexeme;
        return node;
    }
    if (match(TokenType::SPRITE)) {
        auto node = std::make_shared<Engine2DStmt>();
        node->kind = "sprite";
        node->id = consume(TokenType::IDENTIFIER, "Expected sprite name after 'add SPRITE'.").lexeme;
        if (match(TokenType::AT)) {
            node->x = parseExpression();
            match(TokenType::COMMA);
            node->y = parseExpression();
        } else {
            node->x = std::make_shared<LiteralExpr>("150");
            node->y = std::make_shared<LiteralExpr>("150");
        }
        return node;
    }
    auto node = std::make_shared<FileOpStmt>();
    node->action = FileAction::APPEND;
    node->contentExpr = parseExpression();
    consume(TokenType::TO, "Expected 'to' in 'add <content> to file <path>'.");
    consume(TokenType::FILE_KW, "Expected 'file' after 'to'.");
    node->pathExpr = parseExpression();
    return node;
}

StmtPtr Parser::parseDelete() {
    auto node = std::make_shared<FileOpStmt>();
    node->action = FileAction::DELETE_FILE;
    if (match(TokenType::LPAREN)) {
        node->pathExpr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after path in del(...).");
    } else {
        match(TokenType::FILE_KW);
        node->pathExpr = parseExpression();
    }
    return node;
}

StmtPtr Parser::parseLoad() {
    if (check(TokenType::NUMBER) || check(TokenType::IDENTIFIER)) advance(); // "3d"
    match(TokenType::MODEL);
    consume(TokenType::FROM, "Expected 'from' in 'load 3d model from <path> as <name>'.");
    auto node = std::make_shared<Engine3DStmt>();
    node->isModelAsset = true;
    node->assetPath = parseExpression();
    consume(TokenType::AS, "Expected 'as' after 3D model path.");
    node->objectName = consume(TokenType::IDENTIFIER, "Expected identifier for 3D model.").lexeme;
    return node;
}

StmtPtr Parser::parseDraw() {
    auto node = std::make_shared<Engine2DStmt>();
    if (match(TokenType::RECTANGLE)) {
        node->kind = "rect";
        consume(TokenType::AT, "Expected 'at' after 'draw rectangle'.");
        node->x = parseExpression();
        match(TokenType::COMMA);
        node->y = parseExpression();
        match(TokenType::WITH);
        match(TokenType::WIDTH);
        node->w = parseExpression();
        match(TokenType::HEIGHT);
        node->h = parseExpression();
        if (match(TokenType::COLOR) && check(TokenType::STRING)) {
            node->color = advance().lexeme;
        } else {
            node->color = "#58a6ff";
        }
    } else if (match(TokenType::CIRCLE)) {
        node->kind = "circle";
        consume(TokenType::AT, "Expected 'at' after 'draw circle'.");
        node->x = parseExpression();
        match(TokenType::COMMA);
        node->y = parseExpression();
        match(TokenType::WITH);
        match(TokenType::RADIUS);
        node->r = parseExpression();
        if (match(TokenType::COLOR) && check(TokenType::STRING)) {
            node->color = advance().lexeme;
        } else {
            node->color = "#ff7b72";
        }
    }
    return node;
}

StmtPtr Parser::parseIdentOrCall() {
    std::string ident = previous().lexeme;
    if (match(TokenType::LPAREN)) {
        auto call = std::make_shared<CallStmt>();
        call->callee = ident;
        if (!check(TokenType::RPAREN)) {
            do {
                call->arguments.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RPAREN, "Expected ')' after arguments
