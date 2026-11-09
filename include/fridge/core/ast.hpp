#pragma once
#include "fridge/core/token.hpp"
#include <memory>
#include <string>
#include <vector>

namespace fridge {

// Base Expression
struct Expr {
    SourceLocation location;
    virtual ~Expr() = default;
};

// Literals
struct LiteralExpr : public Expr {
    Token token;
    explicit LiteralExpr(Token tok) : token(std::move(tok)) { location = token.location; }
};

// Variable Access
struct VariableExpr : public Expr {
    std::string name;
    explicit VariableExpr(Token tok) : name(tok.lexeme) { location = tok.location; }
};

// Binary Expressions (A + B, A is greater than B, etc.)
struct BinaryExpr : public Expr {
    std::unique_ptr<Expr> left;
    TokenType op;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::unique_ptr<Expr> l, TokenType o, std::unique_ptr<Expr> r, SourceLocation loc)
        : left(std::move(l)), op(o), right(std::move(r)) { location = loc; }
};

// Unary Expressions (not A, minus A)
struct UnaryExpr : public Expr {
    TokenType op;
    std::unique_ptr<Expr> right;
    UnaryExpr(TokenType o, std::unique_ptr<Expr> r, SourceLocation loc)
        : op(o), right(std::move(r)) { location = loc; }
};

// Function Invocation: call <name> with arg1, arg2
struct CallExpr : public Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    CallExpr(std::string name, std::vector<std::unique_ptr<Expr>> args, SourceLocation loc)
        : callee(std::move(name)), arguments(std::move(args)) { location = loc; }
};

// Collection Query Expressions
struct ListAccessExpr : public Expr {
    enum class AccessType { ItemAt, FirstItem, LastItem, Count };
    AccessType access;
    std::string list_name;
    std::unique_ptr<Expr> index_expr; // null for First/Last/Count
    ListAccessExpr(AccessType acc, std::string name, std::unique_ptr<Expr> idx, SourceLocation loc)
        : access(acc), list_name(std::move(name)), index_expr(std::move(idx)) { location = loc; }
};

// Base Statement Node
struct Stmt {
    SourceLocation location;
    virtual ~Stmt() = default;
};

struct ShowStmt : public Stmt {
    std::unique_ptr<Expr> expression;
    explicit ShowStmt(std::unique_ptr<Expr> expr, SourceLocation loc)
        : expression(std::move(expr)) { location = loc; }
};

struct VarDeclStmt : public Stmt {
    std::string name;
    std::unique_ptr<Expr> initializer;
    VarDeclStmt(std::string n, std::unique_ptr<Expr> init, SourceLocation loc)
        : name(std::move(n)), initializer(std::move(init)) { location = loc; }
};

struct AssignmentStmt : public Stmt {
    std::string name;
    std::unique_ptr<Expr> value;
    AssignmentStmt(std::string n, std::unique_ptr<Expr> val, SourceLocation loc)
        : name(std::move(n)), value(std::move(val)) { location = loc; }
};

struct MathMutateStmt : public Stmt {
    enum class Op { Add, Subtract, Multiply, Divide };
    Op op;
    std::string target;
    std::unique_ptr<Expr> amount;
    MathMutateStmt(Op o, std::string tgt, std::unique_ptr<Expr> amt, SourceLocation loc)
        : op(o), target(std::move(tgt)), amount(std::move(amt)) { location = loc; }
};

struct IfStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> then_branch;
    std::vector<std::unique_ptr<Stmt>> else_branch;
    IfStmt(std::unique_ptr<Expr> cond, std::vector<std::unique_ptr<Stmt>> then_b,
           std::vector<std::unique_ptr<Stmt>> else_b, SourceLocation loc)
        : condition(std::move(cond)), then_branch(std::move(then_b)),
          else_branch(std::move(else_b)) { location = loc; }
};

struct RepeatStmt : public Stmt {
    std::unique_ptr<Expr> count_expr;
    std::vector<std::unique_ptr<Stmt>> body;
    RepeatStmt(std::unique_ptr<Expr> count, std::vector<std::unique_ptr<Stmt>> b, SourceLocation loc)
        : count_expr(std::move(count)), body(std::move(b)) { location = loc; }
};

struct WhileStmt : public Stmt {
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> body;
    WhileStmt(std::unique_ptr<Expr> cond, std::vector<std::unique_ptr<Stmt>> b, SourceLocation loc)
        : condition(std::move(cond)), body(std::move(b)) { location = loc; }
};

struct ForEachStmt : public Stmt {
    std::string iterator_name;
    std::string collection_name;
    std::vector<std::unique_ptr<Stmt>> body;
    ForEachStmt(std::string it, std::string coll, std::vector<std::unique_ptr<Stmt>> b, SourceLocation loc)
        : iterator_name(std::move(it)), collection_name(std::move(coll)), body(std::move(b)) { location = loc; }
};

struct FunctionDeclStmt : public Stmt {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::unique_ptr<Stmt>> body;
    FunctionDeclStmt(std::string n, std::vector<std::string> params,
                     std::vector<std::unique_ptr<Stmt>> b, SourceLocation loc)
        : name(std::move(n)), parameters(std::move(params)), body(std::move(b)) { location = loc; }
};

struct ReturnStmt : public Stmt {
    std::unique_ptr<Expr> value;
    ReturnStmt(std::unique_ptr<Expr> val, SourceLocation loc)
        : value(std::move(val)) { location = loc; }
};

struct AskStmt : public Stmt {
    std::unique_ptr<Expr> prompt;
    std::string target_variable;
    AskStmt(std::unique_ptr<Expr> p, std::string target, SourceLocation loc)
        : prompt(std::move(p)), target_variable(std::move(target)) { location = loc; }
};

struct ListDeclStmt : public Stmt {
    std::string name;
    std::vector<std::unique_ptr<Expr>> elements;
    ListDeclStmt(std::string n, std::vector<std::unique_ptr<Expr>> elems, SourceLocation loc)
        : name(std::move(n)), elements(std::move(elems)) { location = loc; }
};

struct MapDeclStmt : public Stmt {
    std::string name;
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> entries;
    MapDeclStmt(std::string n, std::vector<std::pair<std::string, std::unique_ptr<Expr>>> ent, SourceLocation loc)
        : name(std::move(n)), entries(std::move(ent)) { location = loc; }
};

struct GenericCommandStmt : public Stmt {
    std::string command;
    std::vector<std::string> arguments;
    GenericCommandStmt(std::string cmd, std::vector<std::string> args, SourceLocation loc)
        : command(std::move(cmd)), arguments(std::move(args)) { location = loc; }
};

} // namespace fridge
