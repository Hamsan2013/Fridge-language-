#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <regex>

namespace fs = std::filesystem;

struct FunctionDef {
    std::string name;
    std::vector<std::string> params;
    std::vector<std::string> body;
};

class FridgeInterpreter {
private:
    std::unordered_map<std::string, std::string> variables;
    std::unordered_map<std::string, FunctionDef> functions;

    static std::string trim(const std::string& s) {
        size_t first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = s.find_last_not_of(" \t\r\n");
        return s.substr(first, (last - first + 1));
    }

    std::string evalExpr(const std::string& rawExpr) {
        std::string expr = trim(rawExpr);
        if (expr.empty()) return "";

        // Handle "joined with" string concatenation
        size_t joinPos = expr.find(" joined with ");
        if (joinPos != std::string::npos) {
            return evalExpr(expr.substr(0, joinPos)) + evalExpr(expr.substr(joinPos + 13));
        }

        // String literal
        if (expr.size() >= 2 && expr.front() == '"' && expr.back() == '"') {
            return expr.substr(1, expr.size() - 2);
        }

        // Simple addition / subtraction
        size_t plusPos = expr.find(" + ");
        if (plusPos != std::string::npos) {
            double a = std::stod(evalExpr(expr.substr(0, plusPos)));
            double b = std::stod(evalExpr(expr.substr(plusPos + 3)));
            double res = a + b;
            return (res == (long long)res) ? std::to_string((long long)res) : std::to_string(res);
        }
        size_t minusPos = expr.find(" - ");
        if (minusPos != std::string::npos) {
            double a = std::stod(evalExpr(expr.substr(0, minusPos)));
            double b = std::stod(evalExpr(expr.substr(minusPos + 3)));
            double res = a - b;
            return (res == (long long)res) ? std::to_string((long long)res) : std::to_string(res);
        }

        // Variable lookup
        if (variables.count(expr)) {
            return variables[expr];
        }

        return expr;
    }

    bool evalCondition(const std::string& rawCond) {
        std::string cond = trim(rawCond);
        size_t isPos = cond.find(" is ");
        if (isPos != std::string::npos) {
            std::string left = evalExpr(cond.substr(0, isPos));
            std::string right = evalExpr(cond.substr(isPos + 4));
            return left == right;
        }
        std::string val = evalExpr(cond);
        return (val == "true" || val == "1");
    }

    std::vector<std::string> collectBlock(const std::vector<std::string>& lines, size_t& idx) {
        std::vector<std::string> block;
        int depth = 1;
        idx++;
        while (idx < lines.size() && depth > 0) {
            std::string line = trim(lines[idx]);
            for (char c : line) {
                if (c == '{') depth++;
                else if (c == '}') depth--;
            }
            if (depth > 0) {
                block.push_back(lines[idx]);
            }
            idx++;
        }
        idx--; // Position on closing brace line
        return block;
    }

public:
    void executeBlock(const std::vector<std::string>& lines) {
        for (size_t i = 0; i < lines.size(); ++i) {
            std::string line = trim(lines[i]);
            if (line.empty() || line.rfind("//", 0) == 0 || line.rfind("note ", 0) == 0 || line == "}") {
                continue;
            }

            std::smatch match;

            // 1. Variable declaration: rem <var> as <expr> OR remember <var> as <expr>
            if (std::regex_match(line, match, std::regex(R"((?:rem|remember)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s+as\s+(.+))", std::regex::icase))) {
                variables[match[1].str()] = evalExpr(match[2].str());
                continue;
            }

            // 2. Variable mutation: set <var> to <expr>
            if (std::regex_match(line, match, std::regex(R"(set\s+([a-zA-Z_][a-zA-Z0-9_]*)\s+to\s+(.+))", std::regex::icase))) {
                variables[match[1].str()] = evalExpr(match[2].str());
                continue;
            }

            // 3. Console Output: show <expr>
            if (std::regex_match(line, match, std::regex(R"(show\s+(.+))", std::regex::icase))) {
                std::cout << evalExpr(match[1].str()) << std::endl;
                continue;
            }

            // 4. Interactive Input: ask <prompt> as <var>
            if (std::regex_match(line, match, std::regex(R"(ask\s+(.+)\s+as\s+([a-zA-Z_][a-zA-Z0-9_]*))", std::regex::icase))) {
                std::cout << evalExpr(match[1].str()) << " ";
                std::string input;
                std::getline(std::cin, input);
                variables[match[2].str()] = input;
                continue;
            }

            // 5. Stop with custom error: stop with error <expr>
            if (std::regex_match(line, match, std::regex(R"(stop\s+with\s+error\s+(.+))", std::regex::icase))) {
                throw std::runtime_error(evalExpr(match[1].str()));
            }

            // 6. File System: Create / Overwrite File
            if (std::regex_match(line, match, std::regex(R"(create\s+file\s+(\".*?\"|\S+)\s+with\s+(.+))", std::regex::icase))) {
                std::string pathStr = evalExpr(match[1].str());
                std::string content = evalExpr(match[2].str());
                fs::path p(pathStr);
                if (p.has_parent_path()) fs::create_directories(p.parent_path());
                std::ofstream out(p, std::ios::trunc);
                if (!out) throw std::runtime_error("Cannot create file: " + pathStr);
                out << content << "\n";
                continue;
            }

            // 7. File System: Append to File
            if (std::regex_match(line, match, std::regex(R"(add\s+(.+)\s+to\s+file\s+(\".*?\"|\S+))", std::regex::icase))) {
                std::string content = evalExpr(match[1].str());
                std::string pathStr = evalExpr(match[2].str());
                std::ofstream out(pathStr, std::ios::app);
                if (!out) throw std::runtime_error("Cannot append to file: " + pathStr);
                out << content << "\n";
                continue;
            }

            // 8. File System: Read File
            if (std::regex_match(line, match, std::regex(R"(read\s+file\s+(\".*?\"|\S+)\s+into\s+([a-zA-Z_][a-zA-Z0-9_]*))", std::regex::icase))) {
                std::string pathStr = evalExpr(match[1].str());
                std::ifstream in(pathStr);
                if (!in) throw std::runtime_error("File does not exist: " + pathStr);
                std::stringstream buffer;
                buffer << in.rdbuf();
                variables[match[2].str()] = trim(buffer.str());
                continue;
            }

            // 9. File System: Delete File via del("path") or delete file "path"
            if (std::regex_match(line, match, std::regex(R"((?:del\s*\(\s*(.+?)\s*\)|delete\s+file\s+(.+)))", std::regex::icase))) {
                std::string rawTarget = match[1].matched ? match[1].str() : match[2].str();
                std::string pathStr = evalExpr(rawTarget);
                if (!fs::exists(pathStr)) {
                    throw std::runtime_error("Cannot delete missing file: " + pathStr);
                }
                fs::remove(pathStr);
                continue;
            }

            // 10. 3D Asset Loading: load 3d model from "assets/..." as <id>
            if (std::regex_match(line, match, std::regex(R"(load\s+3d\s+model\s+from\s+(\".*?\"|\S+)\s+as\s+([a-zA-Z_][a-zA-Z0-9_]*))", std::regex::icase))) {
                std::string modelPath = evalExpr(match[1].str());
                std::string nodeName = match[2].str();
                std::cout << "[FRIDGE 3D Asset] Loaded model '" << modelPath << "' as '" << nodeName << "'." << std::endl;
                continue;
            }

            // 11. 3D Scene Node: add OBJ <name>
            if (std::regex_match(line, match, std::regex(R"(add\s+OBJ\s+([a-zA-Z0-9_]+))", std::regex::icase))) {
                std::cout << "[FRIDGE 3D Engine] Instantiated 3D scene node: '" << match[1].str() << "'." << std::endl;
                continue;
            }

            // 12. 2D Engine Commands
            if (std::regex_match(line, match, std::regex(R"((draw\s+rectangle|draw\s+circle|add\s+SPRITE|create\s+2d\s+canvas)\b.*)", std::regex::icase))) {
                std::cout << "[FRIDGE 2D Engine] " << line << std::endl;
                continue;
            }

            // 13. Function Definition: fnc name(params) { ... }
            if (std::regex_match(line, match, std::regex(R"(fnc\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\((.*?)\)\s*\{)", std::regex::icase))) {
                FunctionDef fn;
                fn.name = match[1].str();
                std::stringstream ss(match[2].str());
                std::string param;
                while (std::getline(ss, param, ',')) {
                    std::string p = trim(param);
                    if (!p.empty()) fn.params.push_back(p);
                }
                fn.body = collectBlock(lines, i);
                functions[fn.name] = fn;
                continue;
            }

            // 14. Safe Error Handling: attempt { ... } error() { ... }
            if (std::regex_match(line, match, std::regex(R"(attempt\s*\{)", std::regex::icase))) {
                std::vector<std::string> tryBlock = collectBlock(lines, i);
                std::vector<std::string> catchBlock;

                std::string closingLine = trim(lines[i]);
                if (std::regex_search(closingLine, std::regex(R"(\}\s*error\s*\(\s*\)\s*\{)", std::regex::icase))) {
                    catchBlock = collectBlock(lines, i);
                } else if (i + 1 < lines.size() && std::regex_match(trim(lines[i + 1]), std::regex(R"(error\s*\(\s*\)\s*\{)", std::regex::icase))) {
                    i++;
                    catchBlock = collectBlock(lines, i);
                }

                try {
                    executeBlock(tryBlock);
                } catch (const std::exception& e) {
                    variables["last_error"] = e.what();
                    executeBlock(catchBlock);
                }
                continue;
            }

            // 15. Repeat Loop: repeat N times { ... }
            if (std::regex_match(line, match, std::regex(R"(repeat\s+(.+)\s+times\s*\{)", std::regex::icase))) {
                int count = std::stoi(evalExpr(match[1].str()));
                std::vector<std::string> loopBody = collectBlock(lines, i);
                for (int c = 0; c < count; ++c) {
                    executeBlock(loopBody);
                }
                continue;
            }

            // 16. Conditionals: if <cond> { ... } otherwise { ... }
            if (std::regex_match(line, match, std::regex(R"(if\s+(.+?)\s*\{)", std::regex::icase))) {
                bool cond = evalCondition(match[1].str());
                std::vector<std::string> ifBody = collectBlock(lines, i);
                std::vector<std::string> elseBody;

                std::string closeLine = trim(lines[i]);
                if (std::regex_search(closeLine, std::regex(R"(\}\s*otherwise\s*\{)", std::regex::icase))) {
                    elseBody = collectBlock(lines, i);
                } else if (i + 1 < lines.size() && std::regex_match(trim(lines[i + 1]), std::regex(R"(otherwise\s*\{)", std::regex::icase))) {
                    i++;
                    elseBody = collectBlock(lines, i);
                }

                if (cond) executeBlock(ifBody);
                else if (!elseBody.empty()) executeBlock(elseBody);
                continue;
            }

            // 17. Function Call: name(arg1, arg2)
            if (std::regex_match(line, match, std::regex(R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*\((.*?)\))"))) {
                std::string fnName = match[1].str();
                if (functions.count(fnName)) {
                    const auto& fn = functions[fnName];
                    std::vector<std::string> args;
                    std::stringstream ss(match[2].str());
                    std::string arg;
                    while (std::getline(ss, arg, ',')) {
                        args.push_back(evalExpr(arg));
                    }
                    for (size_t p = 0; p < fn.params.size() && p < args.size(); ++p) {
                        variables[fn.params[p]] = args[p];
                    }
                    executeBlock(fn.body);
                    continue;
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "FRIDGE v0.5.0\nUsage: fridge <script.frg>\n";
        return 0;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();

    std::vector<std::string> lines;
    std::string line;
    std::stringstream source(buffer.str());
    while (std::getline(source, line)) {
        lines.push_back(line);
    }

    FridgeInterpreter interpreter;
    try {
        interpreter.executeBlock(lines);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}
