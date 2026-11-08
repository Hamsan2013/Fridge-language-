#pragma once
#include "fridge/core/parser.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace fridge {

struct FunctionDef {
    std::string name;
    std::vector<std::string> params;
    std::vector<Statement> body;
};

class Interpreter {
public:
    Interpreter();
    int execute(const std::vector<Statement>& program);

    // Variable Storage
    void set_var(const std::string& name, const std::string& val);
    std::string get_var(const std::string& name) const;
    bool has_var(const std::string& name) const;

    // 1-Based List Storage
    void set_list(const std::string& name, const std::vector<std::string>& items);
    std::vector<std::string>& get_list(const std::string& name);
    bool has_list(const std::string& name) const;

    // Map Storage
    void set_map_entry(const std::string& map_name, const std::string& key, const std::string& val);
    std::string get_map_entry(const std::string& map_name, const std::string& key) const;

    // Evaluation
    std::string evaluate_tokens(const std::vector<Token>& tokens, size_t start_idx, size_t end_idx = 0);
    bool evaluate_condition(const std::vector<Token>& tokens, size_t start_idx);

    // Return state for functions
    void trigger_return(const std::string& val) { return_value_ = val; returning_ = true; }
    bool is_returning() const { return returning_; }

private:
    std::unordered_map<std::string, std::string> variables_;
    std::unordered_map<std::string, std::vector<std::string>> lists_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> maps_;
    std::unordered_map<std::string, FunctionDef> functions_;

    bool returning_ = false;
    std::string return_value_;
};

} // namespace fridge
