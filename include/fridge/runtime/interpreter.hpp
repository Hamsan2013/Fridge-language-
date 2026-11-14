#pragma once
#include "fridge/core/parser.hpp"
#include "fridge/core/diagnostic.hpp"
#include "fridge/runtime/environment.hpp"
#include <unordered_map>
#include <vector>

namespace fridge {

class Interpreter {
public:
    explicit Interpreter(DiagnosticEngine& diag);
    void execute(const std::vector<StmtPtr>& statements);

private:
    DiagnosticEngine& diagnostics;
    Environment env;
    std::unordered_map<std::string, std::shared_ptr<FunctionStmt>> functions;

    void executeStmt(const StmtPtr& stmt);
    Value evaluate(const ExprPtr& expr);
};

} // namespace fridge
