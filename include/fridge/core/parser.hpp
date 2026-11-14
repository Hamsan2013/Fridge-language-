#pragma once
#include "fridge/core/token.hpp"
#include "fridge/core/diagnostic.hpp"
#include <memory>
#include <string>
#include <vector>

namespace fridge {

// --- AST Expressions ---
struct Expr {
    virtual ~Expr() = default;
};
using ExprPtr = std::shared_ptr<Expr>;

struct LiteralExpr : Expr {
    std::string value;
    explicit LiteralExpr(const std::string& v) : value(v) {}
};

struct VariableExpr : Expr {
    std::string name;
    explicit VariableExpr(const std::string& n) : name(n) {}
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr left;
    ExprPtr right;
    BinaryExpr(const std::string& o, ExprPtr l, ExprPtr r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
};

// --- AST Statements ---
struct Stmt {
    int line = 1;
    virtual ~Stmt() = default;
};
using StmtPtr = std::shared_ptr<Stmt>;

struct VarDeclStmt : Stmt {
    std::string name;
    ExprPtr initializer;
};

struct AssignStmt : Stmt {
    std::string name;
    ExprPtr value;
};

struct ShowStmt : Stmt {
    ExprPtr expr;
};

struct AskStmt : Stmt {
    ExprPtr prompt;
    std::string targetVar;
};

struct IfStmt : Stmt {
    ExprPtr condition;
    std::vector<StmtPtr> thenBranch;
    std::vector<StmtPtr> elseBranch;
};

struct RepeatStmt : Stmt {
    ExprPtr count;
    std::vector<StmtPtr> body;
};

struct FunctionStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
};

struct CallStmt : Stmt {
    std::string callee;
    std::vector<ExprPtr> arguments;
};

struct AttemptStmt : Stmt {
    std::vector<StmtPtr> tryBody;
    std::vector<StmtPtr> errorBody;
};

struct StopErrorStmt : Stmt {
    ExprPtr message;
};

enum class FileAction {
    CREATE_WRITE,
    APPEND,
    READ_INTO,
    DELETE_FILE
};

struct FileOpStmt : Stmt {
    FileAction action;
    ExprPtr pathExpr;
    ExprPtr contentExpr;
    std::string targetVar;
};

struct Engine3DStmt : Stmt {
    bool isModelAsset = false;
    ExprPtr assetPath;
    std::string objectName;
};

struct Engine2DStmt : Stmt {
    std::string kind;
    std::string id;
    ExprPtr x;
    ExprPtr y;
    ExprPtr w;
    ExprPtr h;
    ExprPtr r;
    std::string color;
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens, DiagnosticEngine& diag);
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens;
    DiagnosticEngine& diagnostics;
    size_t current = 0;

    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& consume(TokenType type, const std::string& errMsg);

    StmtPtr parseStatement();
    std::vector<StmtPtr> parseBlock();

    StmtPtr parseVarDecl();
    StmtPtr parseSet();
    StmtPtr parseShow();
    StmtPtr parseAsk();
    StmtPtr parseIf();
    StmtPtr parseRepeat();
    StmtPtr parseFunction();
    StmtPtr parseAttempt();
    StmtPtr parseStop();
    StmtPtr parseCreate();
    StmtPtr parseRead();
    StmtPtr parseAdd();
    StmtPtr parseDelete();
    StmtPtr parseLoad();
    StmtPtr parseDraw();
    StmtPtr parseIdentOrCall();

    ExprPtr parseExpression();
    ExprPtr parseComparison();
    ExprPtr parseJoin();
    ExprPtr parseTerm();
    ExprPtr parsePrimary();
};

} // namespace fridge
