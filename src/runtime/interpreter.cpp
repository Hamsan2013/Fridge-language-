#include "fridge/runtime/interpreter.hpp"
#include "fridge/core/diagnostic.hpp"
#include "fridge/segments/registry.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace fridge {

Interpreter::Interpreter(size_t max_instructions)
    : max_instructions_(max_instructions) {
    globals_ = std::make_shared<Environment>();
    env_ = globals_;
}

Value Interpreter::evaluate(const Expr* expr) {
    if (!expr) return Value();

    if (auto lit = dynamic_cast<const LiteralExpr*>(expr)) {
        if (lit->token.type == TokenType::NumberLiteral) return Value(std::stod(lit->token.lexeme));
        if (lit->token.type == TokenType::StringLiteral) return Value(lit->token.lexeme);
        if (lit->token.type == TokenType::BooleanLiteral) return Value(lit->token.lexeme == "true");
        return Value();
    }

    if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        return env_->get(var->name);
    }

    if (auto un = dynamic_cast<const UnaryExpr*>(expr)) {
        Value r = evaluate(un->right.get());
        if (un->op == TokenType::Not) return Value(!r.as_boolean());
        if (un->op == TokenType::Minus) return Value(-r.as_number());
    }

    if (auto bin = dynamic_cast<const BinaryExpr*>(expr)) {
        Value l = evaluate(bin->left.get());
        Value r = evaluate(bin->right.get());

        switch (bin->op) {
            case TokenType::Plus: return Value(l.as_number() + r.as_number());
            case TokenType::Minus: return Value(l.as_number() - r.as_number());
            case TokenType::MultipliedBy: return Value(l.as_number() * r.as_number());
            case TokenType::DividedBy: {
                double denom = r.as_number();
                if (std::fabs(denom) < 1e-12) {
                    throw std::runtime_error("FRG-RUN-001: Division by zero.");
                }
                return Value(l.as_number() / denom);
            }
            case TokenType::Modulo: return Value(std::fmod(l.as_number(), r.as_number()));
            case TokenType::JoinedWith: return Value(l.to_string() + r.to_string());
            case TokenType::GreaterThan: return Value(l.as_number() > r.as_number());
            case TokenType::LessThan: return Value(l.as_number() < r.as_number());
            case TokenType::EqualTo:
            case TokenType::Is: return Value(l.equals(r));
            case TokenType::IsNot: return Value(!l.equals(r));
            case TokenType::And: return Value(l.as_boolean() && r.as_boolean());
            case TokenType::Or: return Value(l.as_boolean() || r.as_boolean());
            default: break;
        }
    }

    if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        auto it = functions_.find(call->callee);
        if (it == functions_.end()) {
            throw std::runtime_error("FRG-NAME-003: Function '" + call->callee + "' not found.");
        }
        auto fn = it->second;
        if (call->arguments.size() != fn->params.size()) {
            throw std::runtime_error("FRG-TYPE-006: Function '" + call->callee + "' expects " +
                                     std::to_string(fn->params.size()) + " arguments.");
        }

        auto fn_env = std::make_shared<Environment>(globals_);
        for (size_t i = 0; i < fn->params.size(); ++i) {
            fn_env->define(fn->params[i], evaluate(call->arguments[i].get()));
        }

        auto prev_env = env_;
        env_ = fn_env;
        returning_ = false;

        for (const auto* s : fn->body) {
            execute_statement(s);
            if (returning_) break;
        }

        Value res = return_value_;
        returning_ = false;
        env_ = prev_env;
        return res;
    }

    if (auto la = dynamic_cast<const ListAccessExpr*>(expr)) {
        Value val = env_->get(la->list_name);
        auto list = val.as_list();

        if (la->access == ListAccessExpr::AccessType::FirstItem) {
            if (list->empty()) return Value();
            return (*list).front();
        }
        if (la->access == ListAccessExpr::AccessType::LastItem) {
            if (list->empty()) return Value();
            return (*list).back();
        }
        if (la->access == ListAccessExpr::AccessType::ItemAt) {
            int idx = static_cast<int>(evaluate(la->index_expr.get()).as_number());
            if (idx < 1 || idx > static_cast<int>(list->size())) {
                throw std::runtime_error("FRG-RUN-002: List index " + std::to_string(idx) + " out of bounds (1-based).");
            }
            return (*list)[idx - 1];
        }
    }

    return Value();
}

void Interpreter::execute_statement(const Stmt* stmt) {
    if (!stmt || returning_) return;

    if (++instruction_count_ > max_instructions_) {
        throw std::runtime_error("FRG-RUN-003: Infinite loop detected! Exceeded execution limit.");
    }

    if (auto show = dynamic_cast<const ShowStmt*>(stmt)) {
        std::cout << evaluate(show->expression.get()).to_string() << "\n";
        return;
    }

    if (auto decl = dynamic_cast<const VarDeclStmt*>(stmt)) {
        env_->define(decl->name, evaluate(decl->initializer.get()));
        return;
    }

    if (auto ldecl = dynamic_cast<const ListDeclStmt*>(stmt)) {
        std::vector<Value> elems;
        for (const auto& e : ldecl->elements) {
            elems.push_back(evaluate(e.get()));
        }
        env_->define(ldecl->name, make_list(std::move(elems)));
        return;
    }

    if (auto assign = dynamic_cast<const AssignmentStmt*>(stmt)) {
        env_->assign(assign->name, evaluate(assign->value.get()));
        return;
    }

    if (auto mm = dynamic_cast<const MathMutateStmt*>(stmt)) {
        double cur = env_->get(mm->target).as_number();
        double amt = evaluate(mm->amount.get()).as_number();
        switch (mm->op) {
            case MathMutateStmt::Op::Add: env_->assign(mm->target, cur + amt); break;
            case MathMutateStmt::Op::Subtract: env_->assign(mm->target, cur - amt); break;
            case MathMutateStmt::Op::Multiply: env_->assign(mm->target, cur * amt); break;
            case MathMutateStmt::Op::Divide:
                if (std::fabs(amt) < 1e-12) throw std::runtime_error("FRG-RUN-001: Division by zero.");
                env_->assign(mm->target, cur / amt);
                break;
        }
        return;
    }

    if (auto ifs = dynamic_cast<const IfStmt*>(stmt)) {
        if (evaluate(ifs->condition.get()).as_boolean()) {
            for (const auto& s : ifs->then_branch) {
                execute_statement(s.get());
                if (returning_) break;
            }
        } else {
            for (const auto& s : ifs->else_branch) {
                execute_statement(s.get());
                if (returning_) break;
            }
        }
        return;
    }

    if (auto rep = dynamic_cast<const RepeatStmt*>(stmt)) {
        int count = static_cast<int>(evaluate(rep->count_expr.get()).as_number());
        for (int i = 0; i < count; ++i) {
            for (const auto& s : rep->body) {
                execute_statement(s.get());
                if (returning_) break;
            }
            if (returning_) break;
        }
        return;
    }

    if (auto wh = dynamic_cast<const WhileStmt*>(stmt)) {
        while (evaluate(wh->condition.get()).as_boolean()) {
            for (const auto& s : wh->body) {
                execute_statement(s.get());
                if (returning_) break;
            }
            if (returning_) break;
        }
        return;
    }

    if (auto fe = dynamic_cast<const ForEachStmt*>(stmt)) {
        Value val = env_->get(fe->collection_name);
        auto list = val.as_list();
        for (const auto& item : *list) {
            env_->define(fe->iterator_name, item);
            for (const auto& s : fe->body) {
                execute_statement(s.get());
                if (returning_) break;
            }
            if (returning_) break;
        }
        return;
    }

    if (auto fn = dynamic_cast<const FunctionDeclStmt*>(stmt)) {
        auto obj = std::make_shared<FunctionObject>();
        obj->name = fn->name;
        obj->params = fn->parameters;
        for (const auto& s : fn->body) {
            obj->body.push_back(s.get());
        }
        functions_[fn->name] = obj;
        return;
    }

    if (auto ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        return_value_ = evaluate(ret->value.get());
        returning_ = true;
        return;
    }

    if (auto ask = dynamic_cast<const AskStmt*>(stmt)) {
        if (ask->prompt) {
            std::cout << evaluate(ask->prompt.get()).to_string();
        }
        std::string input;
        std::getline(std::cin, input);
        env_->define(ask->target_variable, Value(input));
        return;
    }

    if (auto gen = dynamic_cast<const GenericCommandStmt*>(stmt)) {
        if (SegmentRegistry::instance().has_command(gen->command)) {
            SegmentRegistry::instance().execute(gen->command, gen->arguments, *this);
        }
        return;
    }
}

int Interpreter::execute(const std::vector<std::unique_ptr<Stmt>>& program) {
    try {
        for (const auto& stmt : program) {
            execute_statement(stmt.get());
        }
        return 0;
    } catch (const std::exception& ex) {
        DiagnosticEngine::instance().error("FRG-RUN-ERR", ex.what(), {"runtime", 0, 0});
        DiagnosticEngine::instance().print_all(std::cerr);
        return 1;
    }
}

} // namespace fridge
