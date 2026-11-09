#pragma once
#include "fridge/core/ast.hpp"
#include "fridge/runtime/environment.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace fridge {

struct FunctionObject {
    std::string name;
    std::vector<std::string> params;
    std::vector<const Stmt*> body;
};

class Interpreter {
public:
    explicit Interpreter(size_t max_instructions = 1000000);

    int execute(const std::vector<std::unique_ptr<Stmt>>& program);
    Value evaluate(const Expr* expr);
    void execute_statement(const Stmt* stmt);

    std::shared_ptr<Environment> environment() { return env_; }

private:
    std::shared_ptr<Environment> env_;
    std::shared_ptr<Environment> globals_;
    std::unordered_map<std::string, std::shared_ptr<FunctionObject>> functions_;

    bool returning_ = false;
    Value return_value_;

    size_t instruction_count_ = 0;
    size_t max_instructions_ = 1000000;
};

} // namespace fridge
