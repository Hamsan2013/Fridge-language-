#include "fridge/runtime/interpreter.hpp"
#include "fridge/segments/registry.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

namespace fridge {

static std::string normalize(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

Interpreter::Interpreter() {
    register_all_segments();
}

void Interpreter::set_var(const std::string& name, const std::string& val) {
    variables_[normalize(name)] = val;
}

std::string Interpreter::get_var(const std::string& name) const {
    auto it = variables_.find(normalize(name));
    return it != variables_.end() ? it->second : "";
}

bool Interpreter::has_var(const std::string& name) const {
    return variables_.find(normalize(name)) != variables_.end();
}

std::string Interpreter::evaluate_tokens(const std::vector<Token>& tokens, size_t start_idx) const {
    std::string result;
    for (size_t i = start_idx; i < tokens.size(); ++i) {
        std::string low = normalize(tokens[i].lexeme);
        if (low == "joined" && i + 1 < tokens.size() && normalize(tokens[i + 1].lexeme) == "with") {
            i++;
            continue;
        }
        if (low == "plus" && i + 1 < tokens.size()) {
            double a = std::stod(result.empty() ? "0" : result);
            std::string next_str = has_var(tokens[i + 1].lexeme) ? get_var(tokens[i + 1].lexeme) : tokens[i + 1].lexeme;
            double b = std::stod(next_str);
            std::ostringstream oss;
            oss << (a + b);
            result = oss.str();
            i++;
            continue;
        }
        if (low == "minus" && i + 1 < tokens.size()) {
            double a = std::stod(result.empty() ? "0" : result);
            std::string next_str = has_var(tokens[i + 1].lexeme) ? get_var(tokens[i + 1].lexeme) : tokens[i + 1].lexeme;
            double b = std::stod(next_str);
            std::ostringstream oss;
            oss << (a - b);
            result = oss.str();
            i++;
            continue;
        }

        if (tokens[i].type == TokenType::Text || tokens[i].type == TokenType::Number) {
            result += tokens[i].lexeme;
        } else if (has_var(tokens[i].lexeme)) {
            result += get_var(tokens[i].lexeme);
        } else {
            result += tokens[i].lexeme;
        }
    }
    return result;
}

int Interpreter::execute(const std::vector<Statement>& program) {
    auto& reg = SegmentRegistry::instance();

    for (size_t i = 0; i < program.size(); ++i) {
        const auto& stmt = program[i];

        if (stmt.action == "repeat" && stmt.tokens.size() >= 3) {
            int count = std::stoi(stmt.tokens[1].lexeme);
            std::vector<Statement> body;
            i++;
            while (i < program.size() && !(program[i].action == "end" && program[i].tokens.size() > 1 && normalize(program[i].tokens[1].lexeme) == "repeat")) {
                body.push_back(program[i]);
                i++;
            }
            for (int r = 0; r < count; ++r) {
                execute(body);
            }
            continue;
        }

        if (reg.has_command(stmt.action)) {
            reg.execute(stmt.action, stmt.tokens, *this);
        }
    }
    return 0;
}

} // namespace fridge
