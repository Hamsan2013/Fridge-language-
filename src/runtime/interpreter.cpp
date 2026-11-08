#include "fridge/runtime/interpreter.hpp"
#include "fridge/segments/registry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>

namespace fridge {

static std::string normalize(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static std::string format_number(double val) {
    if (std::floor(val) == val) {
        return std::to_string(static_cast<long long>(val));
    }
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

Interpreter::Interpreter() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
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

void Interpreter::set_list(const std::string& name, const std::vector<std::string>& items) {
    lists_[normalize(name)] = items;
}

std::vector<std::string>& Interpreter::get_list(const std::string& name) {
    return lists_[normalize(name)];
}

bool Interpreter::has_list(const std::string& name) const {
    return lists_.find(normalize(name)) != lists_.end();
}

void Interpreter::set_map_entry(const std::string& map_name, const std::string& key, const std::string& val) {
    maps_[normalize(map_name)][normalize(key)] = val;
}

std::string Interpreter::get_map_entry(const std::string& map_name, const std::string& key) const {
    auto m = maps_.find(normalize(map_name));
    if (m != maps_.end()) {
        auto k = m->second.find(normalize(key));
        if (k != m->second.end()) return k->second;
    }
    return "";
}

std::string Interpreter::evaluate_tokens(const std::vector<Token>& tokens, size_t start_idx, size_t end_idx) {
    if (end_idx == 0 || end_idx > tokens.size()) end_idx = tokens.size();
    if (start_idx >= end_idx) return "";

    // Check for function call: call <func_name> with arg1, arg2
    if (normalize(tokens[start_idx].lexeme) == "call" && start_idx + 1 < end_idx) {
        std::string fn_name = normalize(tokens[start_idx + 1].lexeme);
        auto it = functions_.find(fn_name);
        if (it != functions_.end()) {
            std::vector<std::string> args;
            size_t arg_idx = start_idx + 2;
            if (arg_idx < end_idx && normalize(tokens[arg_idx].lexeme) == "with") arg_idx++;

            std::vector<Token> current_arg;
            for (size_t i = arg_idx; i < end_idx; ++i) {
                if (tokens[i].type == TokenType::Comma) {
                    args.push_back(evaluate_tokens(current_arg, 0));
                    current_arg.clear();
                } else {
                    current_arg.push_back(tokens[i]);
                }
            }
            if (!current_arg.empty()) args.push_back(evaluate_tokens(current_arg, 0));

            auto saved_vars = variables_;
            for (size_t p = 0; p < it->second.params.size() && p < args.size(); ++p) {
                set_var(it->second.params[p], args[p]);
            }
            returning_ = false;
            return_value_ = "";
            execute(it->second.body);
            std::string res = return_value_;
            returning_ = false;
            variables_ = saved_vars;
            return res;
        }
    }

    // Check for 1-based list queries: first item in <list>, last item in <list>, item X of <list>, number of items in <list>
    if (start_idx + 3 < end_idx && normalize(tokens[start_idx].lexeme) == "first" && normalize(tokens[start_idx + 1].lexeme) == "item" && normalize(tokens[start_idx + 2].lexeme) == "in") {
        auto& lst = get_list(tokens[start_idx + 3].lexeme);
        return lst.empty() ? "nothing" : lst.front();
    }
    if (start_idx + 3 < end_idx && normalize(tokens[start_idx].lexeme) == "last" && normalize(tokens[start_idx + 1].lexeme) == "item" && normalize(tokens[start_idx + 2].lexeme) == "in") {
        auto& lst = get_list(tokens[start_idx + 3].lexeme);
        return lst.empty() ? "nothing" : lst.back();
    }
    if (start_idx + 3 < end_idx && normalize(tokens[start_idx].lexeme) == "item" && normalize(tokens[start_idx + 2].lexeme) == "of") {
        int idx = std::stoi(evaluate_tokens({tokens[start_idx + 1]}, 0));
        auto& lst = get_list(tokens[start_idx + 3].lexeme);
        if (idx >= 1 && idx <= static_cast<int>(lst.size())) return lst[idx - 1];
        std::cerr << "[FRG-RUN-002] Runtime Error: List index " << idx << " is out of bounds (1-based).\n";
        return "nothing";
    }
    if (start_idx + 4 < end_idx && normalize(tokens[start_idx].lexeme) == "number" && normalize(tokens[start_idx + 1].lexeme) == "of" && normalize(tokens[start_idx + 2].lexeme) == "items" && normalize(tokens[start_idx + 3].lexeme) == "in") {
        auto& lst = get_list(tokens[start_idx + 4].lexeme);
        return std::to_string(lst.size());
    }

    // Random number between A and B
    if (start_idx + 5 < end_idx && normalize(tokens[start_idx].lexeme) == "random" && normalize(tokens[start_idx + 1].lexeme) == "number" && normalize(tokens[start_idx + 2].lexeme) == "between") {
        int min_v = std::stoi(evaluate_tokens({tokens[start_idx + 3]}, 0));
        int max_v = std::stoi(evaluate_tokens({tokens[start_idx + 5]}, 0));
        if (max_v >= min_v) return std::to_string(min_v + (std::rand() % (max_v - min_v + 1)));
    }

    // Standard expression evaluation
    std::string result;
    for (size_t i = start_idx; i < end_idx; ++i) {
        std::string low = normalize(tokens[i].lexeme);

        // Ignore optional noise words 'a', 'an', 'the'
        if (tokens[i].type == TokenType::Word && (low == "a" || low == "an" || low == "the")) {
            continue;
        }

        if (low == "joined" && i + 1 < end_idx && normalize(tokens[i + 1].lexeme) == "with") {
            i++;
            continue;
        }

        // Math operators: plus, minus, multiplied by, divided by, modulo
        if ((low == "plus" || low == "minus" || low == "modulo") && i + 1 < end_idx) {
            double a = std::stod(result.empty() ? "0" : result);
            std::string next_str = has_var(tokens[i + 1].lexeme) ? get_var(tokens[i + 1].lexeme) : tokens[i + 1].lexeme;
            double b = std::stod(next_str);
            if (low == "plus") result = format_number(a + b);
            else if (low == "minus") result = format_number(a - b);
            else if (low == "modulo") result = format_number(std::fmod(a, b));
            i++;
            continue;
        }

        if ((low == "multiplied" || low == "divided") && i + 2 < end_idx && normalize(tokens[i + 1].lexeme) == "by") {
            double a = std::stod(result.empty() ? "0" : result);
            std::string next_str = has_var(tokens[i + 2].lexeme) ? get_var(tokens[i + 2].lexeme) : tokens[i + 2].lexeme;
            double b = std::stod(next_str);
            if (low == "multiplied") {
                result = format_number(a * b);
            } else {
                if (b == 0) {
                    std::cerr << "[FRG-RUN-001] Runtime Error: Division by zero.\n";
                    return "0";
                }
                result = format_number(a / b);
            }
            i += 2;
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

bool Interpreter::evaluate_condition(const std::vector<Token>& tokens, size_t start_idx) {
    // Find comparison operator
    for (size_t i = start_idx; i < tokens.size(); ++i) {
        std::string w1 = normalize(tokens[i].lexeme);
        std::string w2 = (i + 1 < tokens.size()) ? normalize(tokens[i + 1].lexeme) : "";
        std::string w3 = (i + 2 < tokens.size()) ? normalize(tokens[i + 2].lexeme) : "";

        // "is greater than" / "greater than"
        if ((w1 == "is" && w2 == "greater" && w3 == "than") || (w1 == "greater" && w2 == "than")) {
            size_t op_len = (w1 == "is") ? 3 : 2;
            double left = std::stod(evaluate_tokens(tokens, start_idx, i));
            double right = std::stod(evaluate_tokens(tokens, i + op_len, tokens.size()));
            return left > right;
        }
        // "is less than" / "less than"
        if ((w1 == "is" && w2 == "less" && w3 == "than") || (w1 == "less" && w2 == "than")) {
            size_t op_len = (w1 == "is") ? 3 : 2;
            double left = std::stod(evaluate_tokens(tokens, start_idx, i));
            double right = std::stod(evaluate_tokens(tokens, i + op_len, tokens.size()));
            return left < right;
        }
        // "is at least" / "at least"
        if ((w1 == "is" && w2 == "at" && w3 == "least") || (w1 == "at" && w2 == "least")) {
            size_t op_len = (w1 == "is") ? 3 : 2;
            double left = std::stod(evaluate_tokens(tokens, start_idx, i));
            double right = std::stod(evaluate_tokens(tokens, i + op_len, tokens.size()));
            return left >= right;
        }
        // "is at most" / "at most"
        if ((w1 == "is" && w2 == "at" && w3 == "most") || (w1 == "at" && w2 == "most")) {
            size_t op_len = (w1 == "is") ? 3 : 2;
            double left = std::stod(evaluate_tokens(tokens, start_idx, i));
            double right = std::stod(evaluate_tokens(tokens, i + op_len, tokens.size()));
            return left <= right;
        }
        // "is not"
        if (w1 == "is" && w2 == "not") {
            std::string left = evaluate_tokens(tokens, start_idx, i);
            std::string right = evaluate_tokens(tokens, i + 2, tokens.size());
            return left != right;
        }
        // "is equal to" / "equal to" / "is"
        if ((w1 == "is" && w2 == "equal" && w3 == "to") || (w1 == "equal" && w2 == "to") || w1 == "is") {
            size_t op_len = (w1 == "is" && w2 == "equal") ? 3 : (w1 == "equal" ? 2 : 1);
            std::string left = evaluate_tokens(tokens, start_idx, i);
            std::string right = evaluate_tokens(tokens, i + op_len, tokens.size());
            return left == right;
        }
    }
    std::string val = evaluate_tokens(tokens, start_idx);
    return (normalize(val) == "true" || val == "1");
}

int Interpreter::execute(const std::vector<Statement>& program) {
    auto& reg = SegmentRegistry::instance();

    for (size_t i = 0; i < program.size(); ++i) {
        if (returning_) return 0;
        const auto& stmt = program[i];

        // 1. IF / OTHERWISE / END IF
        if (stmt.action == "if") {
            bool cond = evaluate_condition(stmt.tokens, 1);
            std::vector<Statement> if_body, else_body;
            bool in_else = false;
            int depth = 1;
            i++;
            while (i < program.size() && depth > 0) {
                if (program[i].action == "if") depth++;
                else if (program[i].action == "end" && program[i].tokens.size() > 1 && normalize(program[i].tokens[1].lexeme) == "if") {
                    depth--;
                    if (depth == 0) break;
                } else if (program[i].action == "otherwise" && depth == 1) {
                    in_else = true;
                    i++;
                    continue;
                }
                if (in_else) else_body.push_back(program[i]);
                else if_body.push_back(program[i]);
                i++;
            }
            if (cond) execute(if_body);
            else execute(else_body);
            continue;
        }

        // 2. REPEAT <N> TIMES ... END REPEAT
        if (stmt.action == "repeat" && stmt.tokens.size() >= 2) {
            int count = std::stoi(evaluate_tokens({stmt.tokens[1]}, 0));
            std::vector<Statement> body;
            int depth = 1;
            i++;
            while (i < program.size() && depth > 0) {
                if (program[i].action == "repeat") depth++;
                else if (program[i].action == "end" && program[i].tokens.size() > 1 && normalize(program[i].tokens[1].lexeme) == "repeat") {
                    depth--;
                    if (depth == 0) break;
                }
                body.push_back(program[i]);
                i++;
            }
            for (int r = 0; r < count; ++r) {
                execute(body);
                if (returning_) break;
            }
            continue;
        }

        // 3. WHILE <condition> ... END WHILE
        if (stmt.action == "while") {
            std::vector<Token> cond_tokens = stmt.tokens;
            std::vector<Statement> body;
            int depth = 1;
            i++;
            while (i < program.size() && depth > 0) {
                if (program[i].action == "while") depth++;
                else if (program[i].action == "end" && program[i].tokens.size() > 1 && normalize(program[i].tokens[1].lexeme) == "while") {
                    depth--;
                    if (depth == 0) break;
                }
                body.push_back(program[i]);
                i++;
            }
            while (evaluate_condition(cond_tokens, 1)) {
                execute(body);
                if (returning_) break;
            }
            continue;
        }

        // 4. FOR EACH <item> IN <list> ... END FOR
        if (stmt.action == "for" && stmt.tokens.size() >= 5 && normalize(stmt.tokens[1].lexeme) == "each") {
            std::string var_name = stmt.tokens[2].lexeme;
            std::string list_name = stmt.tokens[4].lexeme;
            std::vector<Statement> body;
            int depth = 1;
            i++;
            while (i < program.size() && depth > 0) {
                if (program[i].action == "for") depth++;
                else if (program[i].action == "end" && program[i].tokens.size() > 1 && normalize(program[i].tokens[1].lexeme) == "for") {
                    depth--;
                    if (depth == 0) break;
                }
                body.push_back(program[i]);
                i++;
            }
            if (has_list(list_name)) {
                auto items = get_list(list_name);
                for (const auto& item : items) {
                    set_var(var_name, item);
                    execute(body);
                    if (returning_) break;
                }
            }
            continue;
        }

        // 5. CREATE FUNCTION CALLED <name> TAKING <p1> AND <p2> ... END FUNCTION
        if (stmt.action == "create" && stmt.tokens.size() >= 4 && normalize(stmt.tokens[1].lexeme) == "function" && normalize(stmt.tokens[2].lexeme) == "called") {
            FunctionDef fn;
            fn.name = normalize(stmt.tokens[3].lexeme);
            for (size_t k = 4; k < stmt.tokens.size(); ++k) {
                std::string w = normalize(stmt.tokens[k].lexeme);
                if (w == "taking" || w == "and" || stmt.tokens[k].type == TokenType::Comma) continue;
                if (w == "returning") break;
                fn.params.push_back(stmt.tokens[k].lexeme);
            }
            i++;
            while (i < program.size() && !(program[i].action == "end" && program[i].tokens.size() > 1 && normalize(program[i].tokens[1].lexeme) == "function")) {
                fn.body.push_back(program[i]);
                i++;
            }
            functions_[fn.name] = std::move(fn);
            continue;
        }

        // 6. RETURN <expression>
        if (stmt.action == "return") {
            trigger_return(evaluate_tokens(stmt.tokens, 1));
            return 0;
        }

        // 7. <var> becomes <expression>
        if (stmt.tokens.size() >= 3 && normalize(stmt.tokens[1].lexeme) == "becomes") {
            set_var(stmt.tokens[0].lexeme, evaluate_tokens(stmt.tokens, 2));
            continue;
        }

        // 8. Standard Registered Commands
        if (reg.has_command(stmt.action)) {
            reg.execute(stmt.action, stmt.tokens, *this);
        }
    }
    return 0;
}

} // namespace fridge
