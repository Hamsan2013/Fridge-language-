#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "updater.hpp"

namespace fs = std::filesystem;

#ifndef FRIDGE_VERSION
#define FRIDGE_VERSION "0.5.2"
#endif

namespace {
std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool starts_with_ci(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() &&
        lower(value.substr(0, prefix.size())) == lower(prefix);
}

bool parse_number(const std::string& value, double& number) {
    if (value.empty()) return false;
    char* end = nullptr;
    number = std::strtod(value.c_str(), &end);
    return end != value.c_str() && end != nullptr && *end == '\0' && std::isfinite(number);
}

std::string number_string(double value) {
    if (!std::isfinite(value)) throw std::runtime_error("A calculation produced a non-finite number.");
    if (std::fabs(value) < 1e-12) value = 0;
    std::ostringstream out;
    out.precision(15);
    out << value;
    return out.str();
}

struct SourceLine {
    std::string text;
    size_t number = 0;
};

struct FunctionDef {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<SourceLine> body;
};

std::vector<std::string> split_arguments(const std::string& text) {
    std::vector<std::string> result;
    std::string part;
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (char c : text) {
        if (in_string) {
            part += c;
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') {
            in_string = true;
            part += c;
        } else if (c == '(') {
            ++depth;
            part += c;
        } else if (c == ')') {
            --depth;
            if (depth < 0) throw std::runtime_error("Unexpected ')' in argument list.");
            part += c;
        } else if (c == ',' && depth == 0) {
            result.push_back(trim(part));
            part.clear();
        } else {
            part += c;
        }
    }
    if (in_string || depth != 0) throw std::runtime_error("Unclosed string or parenthesis in argument list.");
    if (!trim(part).empty() || !text.empty()) result.push_back(trim(part));
    return result;
}

class ExpressionParser {
public:
    using Variables = std::unordered_map<std::string, std::string>;

    ExpressionParser(const std::string& input, const Variables& variables)
        : variables_(variables) {
        tokenize(input);
    }

    std::string parse() {
        if (tokens_.empty()) return {};
        std::string result = parse_or();
        if (position_ != tokens_.size()) {
            throw std::runtime_error("Unexpected token in expression: " + tokens_[position_].value);
        }
        return result;
    }

private:
    struct Token {
        enum class Kind { Word, Number, String, Symbol } kind;
        std::string value;
    };

    const Variables& variables_;
    std::vector<Token> tokens_;
    size_t position_ = 0;

    void tokenize(const std::string& input) {
        size_t i = 0;
        while (i < input.size()) {
            const unsigned char c = static_cast<unsigned char>(input[i]);
            if (std::isspace(c)) { ++i; continue; }
            if (input[i] == '"') {
                ++i;
                std::string value;
                bool closed = false;
                while (i < input.size()) {
                    char ch = input[i++];
                    if (ch == '"') { closed = true; break; }
                    if (ch == '\\') {
                        if (i >= input.size()) throw std::runtime_error("String ends with an escape character.");
                        const char escaped = input[i++];
                        switch (escaped) {
                            case 'n': value += '\n'; break;
                            case 'r': value += '\r'; break;
                            case 't': value += '\t'; break;
                            case '\\': value += '\\'; break;
                            case '"': value += '"'; break;
                            default: throw std::runtime_error(std::string("Unsupported string escape: \\") + escaped);
                        }
                    } else value += ch;
                }
                if (!closed) throw std::runtime_error("Unclosed string literal.");
                tokens_.push_back({Token::Kind::String, value});
            } else if (std::isdigit(c) || (input[i] == '.' && i + 1 < input.size() &&
                                                  std::isdigit(static_cast<unsigned char>(input[i + 1])))) {
                const size_t start = i++;
                while (i < input.size() && (std::isdigit(static_cast<unsigned char>(input[i])) || input[i] == '.')) ++i;
                const std::string value = input.substr(start, i - start);
                double parsed = 0;
                if (!parse_number(value, parsed)) throw std::runtime_error("Invalid numeric literal: " + value);
                tokens_.push_back({Token::Kind::Number, value});
            } else if (std::isalpha(c) || input[i] == '_') {
                const size_t start = i++;
                while (i < input.size() && (std::isalnum(static_cast<unsigned char>(input[i])) || input[i] == '_')) ++i;
                tokens_.push_back({Token::Kind::Word, input.substr(start, i - start)});
             } else if (std::string("+-*/%()><=!&|").find(input[i]) != std::string::npos) {
                std::string op(1, input[i++]);
                if ((op == ">" || op == "<" || op == "=" || op == "!" || op == "&" || op == "|") &&
                    i < input.size() && input[i] == (op == "&" ? '&' : op == "|" ? '|' : '=')) {
                    op += input[i++];
                } else if (op == "=" || op == "!" || op == "&" || op == "|") {
                    throw std::runtime_error("Expected a second character after operator '" + op + "'.");
                }
                tokens_.push_back({Token::Kind::Symbol, op});
            } else {
                throw std::runtime_error(std::string("Invalid character in expression: ") + input[i]);
            }
        }
    }

    bool has(size_t offset = 0) const { return position_ + offset < tokens_.size(); }
    const Token& current() const {
        static const Token empty{Token::Kind::Symbol, ""};
        return has() ? tokens_[position_] : empty;
    }
    bool symbol(const std::string& value) const {
        return has() && current().kind == Token::Kind::Symbol && current().value == value;
    }
    bool word(const std::string& value, size_t offset = 0) const {
        return has(offset) && tokens_[position_ + offset].kind == Token::Kind::Word &&
            lower(tokens_[position_ + offset].value) == value;
    }
    bool phrase(const std::string& first, const std::string& second) const {
        return word(first) && word(second, 1);
    }
    void take_phrase() { position_ += 2; }

    static bool numeric_binary(const std::string& a, const std::string& b, char op, std::string& output) {
        double left = 0, right = 0;
        if (!parse_number(a, left) || !parse_number(b, right)) return false;
        switch (op) {
            case '+': output = number_string(left + right); break;
            case '-': output = number_string(left - right); break;
            case '*': output = number_string(left * right); break;
            case '/':
                if (right == 0) throw std::runtime_error("Division by zero.");
                output = number_string(left / right); break;
            case '%':
                if (right == 0) throw std::runtime_error("Modulo by zero.");
                output = number_string(std::fmod(left, right)); break;
            default: return false;
        }
        return true;
    }

    std::string parse_or() {
        std::string left = parse_and();
        while (word("or") || symbol("||")) {
            ++position_;
            const std::string right = parse_and();
            const bool a = truthy(left), b = truthy(right);
            left = (a || b) ? "true" : "false";
        }
        return left;
    }

    std::string parse_and() {
        std::string left = parse_comparison();
        while (word("and") || symbol("&&")) {
            ++position_;
            const std::string right = parse_comparison();
            const bool a = truthy(left), b = truthy(right);
            left = (a && b) ? "true" : "false";
        }
        return left;
    }

    std::string parse_comparison() {
        std::string left = parse_join();
        std::string op;
        if (symbol("==") || symbol("!=") || symbol(">") || symbol("<") || symbol(">=") || symbol("<=")) {
            op = current().value;
            ++position_;
        } else if (word("is")) {
            ++position_;
            if (word("not")) { ++position_; op = "!="; }
            else if (word("greater") && word("than", 1)) { take_phrase(); op = ">"; }
            else if (word("less") && word("than", 1)) { take_phrase(); op = "<"; }
            else if (word("at") && (word("least", 1) || word("most", 1))) {
                const bool least = word("least", 1);
                take_phrase();
                op = least ? ">=" : "<=";
            } else op = "==";
        } else if (phrase("greater", "than")) { take_phrase(); op = ">"; }
        else if (phrase("less", "than")) { take_phrase(); op = "<"; }
        else if (phrase("at", "least")) { take_phrase(); op = ">="; }
        else if (phrase("at", "most")) { take_phrase(); op = "<="; }
        if (op.empty()) return left;

        const std::string right = parse_join();
        double a = 0, b = 0;
        const bool numeric = parse_number(left, a) && parse_number(right, b);
        bool result = false;
        if (op == "==") result = lower(left) == lower(right);
        else if (op == "!=") result = lower(left) != lower(right);
        else if (numeric && op == ">") result = a > b;
        else if (numeric && op == "<") result = a < b;
        else if (numeric && op == ">=") result = a >= b;
        else if (numeric && op == "<=") result = a <= b;
        else if (!numeric && op != "==" && op != "!=")
            throw std::runtime_error("Numeric comparison requires two numeric values.");
        return result ? "true" : "false";
    }

    std::string parse_join() {
        std::string value = parse_add();
        while (phrase("joined", "with")) {
            take_phrase();
            value += parse_add();
        }
        return value;
    }

    std::string parse_add() {
        std::string left = parse_multiply();
        while (symbol("+") || symbol("-") || word("plus") || word("minus")) {
            char op = (symbol("-") || word("minus")) ? '-' : '+';
            ++position_;
            const std::string right = parse_multiply();
            std::string result;
            if (!numeric_binary(left, right, op, result)) {
                if (op == '+') left += right;
                else throw std::runtime_error("Subtraction requires numeric values.");
            } else left = result;
        }
        return left;
    }

    std::string parse_multiply() {
        std::string left = parse_unary();
        while (symbol("*") || symbol("/") || symbol("%") ||
               phrase("multiplied", "by") || phrase("divided", "by") || word("modulo")) {
            char op = '*';
            if (symbol("/")) op = '/';
            else if (symbol("%") || word("modulo")) op = '%';
            else if (phrase("divided", "by")) op = '/';
            if (phrase("multiplied", "by") || phrase("divided", "by")) take_phrase();
            else ++position_;
            const std::string right = parse_unary();
            std::string result;
            if (!numeric_binary(left, right, op, result))
                throw std::runtime_error("Multiplication, division, and modulo require numeric values.");
            left = result;
        }
        return left;
    }

    std::string parse_unary() {
        if (symbol("-")) { ++position_; double v = 0; const auto s = parse_unary(); if (!parse_number(s, v)) throw std::runtime_error("Unary minus requires a number."); return number_string(-v); }
        if (symbol("+")) { ++position_; double v = 0; const auto s = parse_unary(); if (!parse_number(s, v)) throw std::runtime_error("Unary plus requires a number."); return number_string(v); }
        if (word("not") || symbol("!")) { ++position_; return truthy(parse_unary()) ? "false" : "true"; }
        return parse_primary();
    }

    std::string parse_primary() {
        if (!has()) throw std::runtime_error("Expression is missing a value.");
        if (symbol("(")) {
            ++position_;
            std::string value = parse_or();
            if (!symbol(")")) throw std::runtime_error("Missing ')' in expression.");
            ++position_;
            return value;
        }
        const Token token = current();
        ++position_;
        if (token.kind == Token::Kind::String || token.kind == Token::Kind::Number) return token.value;
        if (token.kind == Token::Kind::Word) {
            const std::string key = lower(token.value);
            if (key == "true" || key == "false") return key;
            const auto found = variables_.find(key);
            return found == variables_.end() ? token.value : found->second;
        }
        throw std::runtime_error("Unexpected symbol in expression: " + token.value);
    }

    static bool truthy(const std::string& value) {
        const std::string normalized = lower(trim(value));
        if (normalized.empty() || normalized == "false" || normalized == "0" || normalized == "nothing") return false;
        return true;
    }
};

bool is_truthy(const std::string& value) {
    const std::string normalized = lower(trim(value));
    return !(normalized.empty() || normalized == "false" || normalized == "0" || normalized == "nothing");
}

bool scan_braces(const std::string& line, int& depth, bool& in_string, bool& escaped) {
    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (in_string) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') in_string = true;
        else if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') break;
        else if (c == '{') ++depth;
        else if (c == '}') {
            --depth;
            if (depth < 0) return false;
        }
    }
    return true;
}

class Interpreter {
public:
    using Variables = ExpressionParser::Variables;

    void execute(const std::vector<SourceLine>& lines) {
        execute_block(lines, 0);
    }

private:
    Variables variables_;
    std::unordered_map<std::string, FunctionDef> functions_;
    size_t instructions_ = 0;
    bool returned_ = false;
    std::string return_value_;
    size_t recursion_depth_ = 0;

    std::string eval(const std::string& expression) const {
        return ExpressionParser(expression, variables_).parse();
    }

    fs::path safe_path(const std::string& raw, bool create_parent) const {
        if (raw.empty() || raw.size() > 4096) throw std::runtime_error("File path is empty or exceeds the 4096-byte safety limit.");
        const fs::path requested = fs::u8path(raw);
        if (requested.empty() || requested.is_absolute() || requested.has_root_name())
            throw std::runtime_error("File paths must be relative to the current project directory.");

        for (const auto& part : requested) {
            if (part == ".." || part == ".") {
                if (part == "..") throw std::runtime_error("File paths cannot escape the current project directory.");
            }
        }

        std::error_code ec;
        const fs::path root = fs::weakly_canonical(fs::current_path(), ec);
        if (ec) throw std::runtime_error("Cannot resolve project directory: " + ec.message());
        fs::path candidate = (root / requested).lexically_normal();

        auto within_root = [&root](const fs::path& path) {
            auto r = root.begin();
            auto p = path.begin();
            for (; r != root.end(); ++r, ++p) {
                if (p == path.end() || *r != *p) return false;
            }
            return true;
        };

        fs::path walk = root;
        for (const auto& part : requested) {
            if (part == "." || part.empty()) continue;
            walk /= part;
            const auto status = fs::symlink_status(walk, ec);
            if (!ec && fs::is_symlink(status))
                throw std::runtime_error("File operations through symbolic links are blocked for safety.");
            ec.clear();
        }

        if (create_parent && candidate.has_parent_path()) {
            fs::create_directories(candidate.parent_path(), ec);
            if (ec) throw std::runtime_error("Cannot create parent directory: " + ec.message());
            walk = fs::weakly_canonical(candidate.parent_path(), ec);
            if (ec || !within_root(walk))
                throw std::runtime_error("File path resolves outside the current project directory.");
        }

        if (fs::exists(candidate, ec)) {
            const fs::path resolved = fs::weakly_canonical(candidate, ec);
            if (ec || !within_root(resolved))
                throw std::runtime_error("File path resolves outside the current project directory.");
            candidate = resolved;
        }
        if (!within_root(candidate))
            throw std::runtime_error("File path resolves outside the current project directory.");
        return candidate;
    }

    std::vector<SourceLine> collect_block(const std::vector<SourceLine>& lines, size_t& index) const {
        const std::string header = lines[index].text;
        bool in_string = false, escaped = false;
        size_t opening = std::string::npos;
        for (size_t i = 0; i < header.size(); ++i) {
            const char c = header[i];
            if (in_string) {
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') in_string = false;
            } else if (c == '"') in_string = true;
            else if (c == '/' && i + 1 < header.size() && header[i + 1] == '/') break;
            else if (c == '{') { opening = i; break; }
        }
        if (opening == std::string::npos)
            throw std::runtime_error("Expected '{' to open block at line " + std::to_string(lines[index].number));

        int depth = 1;
        std::vector<SourceLine> body;
        auto scan_from = [&](const std::string& text, size_t start) -> size_t {
            bool string = false, escape = false;
            for (size_t i = start; i < text.size(); ++i) {
                const char c = text[i];
                if (string) {
                    if (escape) escape = false;
                    else if (c == '\\') escape = true;
                    else if (c == '"') string = false;
                } else if (c == '"') string = true;
                else if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') break;
                else if (c == '{') ++depth;
                else if (c == '}') {
                    --depth;
                    if (depth == 0) return i;
                    if (depth < 0) throw std::runtime_error("Unexpected '}' at line " + std::to_string(lines[index].number));
                }
            }
            return std::string::npos;
        };

        std::string tail = header.substr(opening + 1);
        size_t close = scan_from(tail, 0);
        if (close != std::string::npos) {
            if (!trim(tail.substr(close + 1)).empty())
                throw std::runtime_error("Put code after a closing block brace on a new line (line " +
                                         std::to_string(lines[index].number) + ").");
            return body;
        }

        for (size_t line = index + 1; line < lines.size(); ++line) {
            close = scan_from(lines[line].text, 0);
            if (close != std::string::npos) {
                if (!trim(lines[line].text.substr(0, close)).empty())
                    body.push_back({trim(lines[line].text.substr(0, close)), lines[line].number});
                if (!trim(lines[line].text.substr(close + 1)).empty())
                    throw std::runtime_error("Put code after a closing block brace on a new line (line " +
                                             std::to_string(lines[line].number) + ").");
                index = line;
                return body;
            }
            body.push_back(lines[line]);
        }
        throw std::runtime_error("Unclosed block beginning at line " + std::to_string(lines[index].number));
    }

    void execute_block(const std::vector<SourceLine>& lines, size_t depth) {
        if (depth > 64) throw std::runtime_error("Maximum function/block nesting depth exceeded.");
        for (size_t i = 0; i < lines.size(); ++i) {
            if (returned_) return;
            if (++instructions_ > 1000000)
                throw std::runtime_error("Execution stopped: the 1,000,000-instruction safety limit was reached.");

            const std::string line = trim(lines[i].text);
            if (line.empty() || line == "}" || starts_with_ci(line, "//") || starts_with_ci(line, "note ")) continue;
            std::smatch match;
            const auto& source = lines[i];

            try {
                if (std::regex_match(line, match, std::regex(R"((?:rem|remember)\s+([A-Za-z_][A-Za-z0-9_]*)\s+as\s+(.+))", std::regex::icase))) {
                    variables_[lower(match[1].str())] = eval(match[2].str()); continue;
                }
                if (std::regex_match(line, match, std::regex(R"(set\s+([A-Za-z_][A-Za-z0-9_]*)\s+to\s+(.+))", std::regex::icase))) {
                    variables_[lower(match[1].str())] = eval(match[2].str()); continue;
                }
                if (std::regex_match(line, match, std::regex(R"(show\s+(.+))", std::regex::icase))) {
                    std::cout << eval(match[1].str()) << '\n'; continue;
                }
                if (std::regex_match(line, match, std::regex(R"(ask\s+(.+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase))) {
                    std::cout << eval(match[1].str()) << ' ' << std::flush;
                    std::string value;
                    if (!std::getline(std::cin, value)) throw std::runtime_error("Input stream closed while waiting for ask.");
                    variables_[lower(match[2].str())] = value; continue;
                }
                if (std::regex_match(line, match, std::regex(R"(stop\s+with\s+error\s+(.+))", std::regex::icase)))
                    throw std::runtime_error(eval(match[1].str()));

                if (std::regex_match(line, match, std::regex(R"(create\s+file\s+(.+?)\s+with\s+(.+))", std::regex::icase))) {
                    const std::string path = eval(match[1].str());
                    const std::string content = eval(match[2].str());
                    const fs::path target = safe_path(path, true);
                    std::ofstream out(target, std::ios::binary | std::ios::trunc);
                    if (!out) throw std::runtime_error("Cannot create file: " + path);
                    out << content;
                    out.flush();
                    if (!out) throw std::runtime_error("Writing file failed: " + path);
                    out.close();
                    if (out.fail()) throw std::runtime_error("Closing file failed: " + path);
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(add\s+(.+?)\s+to\s+file\s+(.+))", std::regex::icase))) {
                    const std::string content = eval(match[1].str());
                    const std::string path = eval(match[2].str());
                    const fs::path target = safe_path(path, true);
                    std::ofstream out(target, std::ios::binary | std::ios::app);
                    if (!out) throw std::runtime_error("Cannot append to file: " + path);
                    out << content;
                    out.flush();
                    if (!out) throw std::runtime_error("Appending to file failed: " + path);
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(read\s+file\s+(.+?)\s+into\s+([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase))) {
                    const std::string path = eval(match[1].str());
                    const fs::path target = safe_path(path, false);
                    std::error_code size_error;
                    const auto input_size = fs::file_size(target, size_error);
                    if (size_error || input_size > 64ULL * 1024ULL * 1024ULL)
                        throw std::runtime_error("Cannot read file or file exceeds the 64 MiB safety limit: " + path);
                    std::ifstream in(target, std::ios::binary);
                    if (!in) throw std::runtime_error("Cannot read file: " + path);
                    std::ostringstream data;
                    data << in.rdbuf();
                    if (!in.good() && !in.eof()) throw std::runtime_error("Reading file failed: " + path);
                    variables_[lower(match[2].str())] = data.str();
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"((?:del\s*\(\s*(.+?)\s*\)|delete\s+file\s+(.+)))", std::regex::icase))) {
                    const std::string raw = match[1].matched ? match[1].str() : match[2].str();
                    const std::string path = eval(raw);
                    const fs::path target = safe_path(path, false);
                    std::error_code ec;
                    if (!fs::is_regular_file(target, ec) || ec)
                        throw std::runtime_error("Cannot delete missing or non-regular file: " + path);
                    if (!fs::remove(target, ec) || ec)
                        throw std::runtime_error("Cannot delete file: " + path + (ec ? " (" + ec.message() + ")" : ""));
                    continue;
                }

                if (std::regex_match(line, match, std::regex(R"(add\s+(.+?)\s+to\s+([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase))) {
                    const std::string variable = lower(match[2].str());
                    variables_[variable] = eval(variable + " + (" + match[1].str() + ")");
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(subtract\s+(.+?)\s+from\s+([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase))) {
                    const std::string variable = lower(match[2].str());
                    variables_[variable] = eval(variable + " - (" + match[1].str() + ")");
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(multiply\s+([A-Za-z_][A-Za-z0-9_]*)\s+by\s+(.+))", std::regex::icase))) {
                    const std::string variable = lower(match[1].str());
                    variables_[variable] = eval(variable + " * (" + match[2].str() + ")");
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(divide\s+([A-Za-z_][A-Za-z0-9_]*)\s+by\s+(.+))", std::regex::icase))) {
                    const std::string variable = lower(match[1].str());
                    variables_[variable] = eval(variable + " / (" + match[2].str() + ")");
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(while\s+(.+?)\s*\{)", std::regex::icase))) {
                    const std::string condition = match[1].str();
                    const auto body = collect_block(lines, i);
                    size_t iterations = 0;
                    while (is_truthy(eval(condition))) {
                        if (++iterations > 100000)
                            throw std::runtime_error("while loop exceeded the 100000-iteration safety limit.");
                        execute_block(body, depth + 1);
                        if (returned_) break;
                    }
                    continue;
                }

                if (std::regex_match(line, match, std::regex(R"(fnc\s+([A-Za-z_][A-Za-z0-9_]*)\s*\((.*?)\)\s*\{)", std::regex::icase))) {
                    FunctionDef fn;
                    fn.name = lower(match[1].str());
                    fn.parameters = split_arguments(match[2].str());
                    for (const auto& param : fn.parameters) {
                        if (!std::regex_match(param, std::regex(R"([A-Za-z_][A-Za-z0-9_]*)")))
                            throw std::runtime_error("Invalid function parameter name: " + param);
                    }
                    fn.body = collect_block(lines, i);
                    if (functions_.count(fn.name)) throw std::runtime_error("Function already defined: " + fn.name);
                    functions_[fn.name] = std::move(fn);
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(repeat\s+(.+?)\s+times\s*\{)", std::regex::icase))) {
                    const std::string count_text = eval(match[1].str());
                    double raw_count = 0;
                    if (!parse_number(count_text, raw_count) || raw_count < 0 || raw_count > 100000 ||
                        std::floor(raw_count) != raw_count)
                        throw std::runtime_error("repeat count must be a whole number between 0 and 100000.");
                    const auto body = collect_block(lines, i);
                    for (size_t count = 0; count < static_cast<size_t>(raw_count); ++count) {
                        execute_block(body, depth + 1);
                        if (returned_) break;
                    }
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(if\s+(.+?)\s*\{)", std::regex::icase))) {
                    const bool condition = is_truthy(eval(match[1].str()));
                    const auto if_body = collect_block(lines, i);
                    std::vector<SourceLine> else_body;
                    size_t next = i + 1;
                    while (next < lines.size() && trim(lines[next].text).empty()) ++next;
                    if (next < lines.size() && std::regex_match(trim(lines[next].text), std::regex(R"(otherwise\s*\{)", std::regex::icase))) {
                        i = next;
                        else_body = collect_block(lines, i);
                    }
                    execute_block(condition ? if_body : else_body, depth + 1);
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(attempt\s*\{)", std::regex::icase))) {
                    const auto try_body = collect_block(lines, i);
                    std::vector<SourceLine> catch_body;
                    size_t next = i + 1;
                    while (next < lines.size() && trim(lines[next].text).empty()) ++next;
                    if (next < lines.size() && std::regex_match(trim(lines[next].text), std::regex(R"(error\s*\(\s*\)\s*\{)", std::regex::icase))) {
                        i = next;
                        catch_body = collect_block(lines, i);
                    }
                    try {
                        execute_block(try_body, depth + 1);
                    } catch (const std::exception& error) {
                        variables_["last_error"] = error.what();
                        execute_block(catch_body, depth + 1);
                    }
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(return(?:\s+(.+))?)", std::regex::icase))) {
                    if (recursion_depth_ == 0) throw std::runtime_error("return can only be used inside a function.");
                    return_value_ = match[1].matched ? eval(match[1].str()) : "";
                    returned_ = true;
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"(([A-Za-z_][A-Za-z0-9_]*)\s*\((.*)\)\s*;?)"))) {
                    const std::string name = lower(match[1].str());
                    const auto found = functions_.find(name);
                    if (found == functions_.end()) throw std::runtime_error("Unknown function: " + name);
                    if (++recursion_depth_ > 64) {
                        --recursion_depth_;
                        throw std::runtime_error("Maximum function recursion depth exceeded.");
                    }
                    const auto raw_args = split_arguments(match[2].str());
                    std::vector<std::string> args;
                    args.reserve(raw_args.size());
                    for (const auto& raw_arg : raw_args) args.push_back(eval(raw_arg));
                    const FunctionDef fn = found->second;
                    if (args.size() != fn.parameters.size()) {
                        --recursion_depth_;
                        throw std::runtime_error("Function '" + name + "' expects " +
                            std::to_string(fn.parameters.size()) + " argument(s), received " +
                            std::to_string(args.size()) + ".");
                    }
                    struct SavedVariable { std::string key; bool existed; std::string value; };
                    std::vector<SavedVariable> old_values;
                    for (size_t a = 0; a < args.size(); ++a) {
                        const std::string parameter = lower(fn.parameters[a]);
                        const auto old = variables_.find(parameter);
                        old_values.push_back({parameter, old != variables_.end(), old != variables_.end() ? old->second : std::string()});
                        variables_[parameter] = args[a];
                    }
                    const bool old_returned = returned_;
                    const std::string old_return_value = return_value_;
                    returned_ = false;
                    return_value_.clear();
                    try {
                        execute_block(fn.body, depth + 1);
                    } catch (...) {
                        for (const auto& entry : old_values) {
                            if (entry.existed) variables_[entry.key] = entry.value;
                            else variables_.erase(entry.key);
                        }
                        --recursion_depth_;
                        returned_ = old_returned;
                        return_value_ = old_return_value;
                        throw;
                    }
                    const bool function_returned = returned_;
                    const std::string function_result = return_value_;
                    for (const auto& entry : old_values) {
                        if (entry.existed) variables_[entry.key] = entry.value;
                        else variables_.erase(entry.key);
                    }
                    returned_ = old_returned;
                    return_value_ = old_return_value;
                    if (function_returned) variables_[name + "_result"] = function_result;
                    --recursion_depth_;
                    continue;
                }

                if (std::regex_match(line, match, std::regex(R"((draw\s+rectangle|draw\s+circle|add\s+SPRITE|create\s+2d\s+canvas)\b.*)", std::regex::icase))) {
                    std::cout << "[FRIDGE 2D command accepted; renderer not bundled in this CLI] " << line << '\n';
                    continue;
                }
                if (std::regex_match(line, match, std::regex(R"((load\s+3d\s+model|add\s+OBJ)\b.*)", std::regex::icase))) {
                    std::cout << "[FRIDGE 3D command accepted; renderer not bundled in this CLI] " << line << '\n';
                    continue;
                }

                throw std::runtime_error("Unknown or unsupported statement: " + line);
            } catch (const std::exception& error) {
                throw std::runtime_error("Line " + std::to_string(source.number) + ": " + error.what());
            }
        }
    }
};

std::vector<SourceLine> read_source(const fs::path& path) {
    std::error_code size_error;
    const auto source_size = fs::file_size(path, size_error);
    if (size_error) throw std::runtime_error("Cannot inspect source file: " + path.string());
    if (source_size > 10ULL * 1024ULL * 1024ULL)
        throw std::runtime_error("Source file exceeds the 10 MiB safety limit.");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open file: " + path.string());
    std::vector<SourceLine> lines;
    std::string line;
    size_t number = 0;
    while (std::getline(input, line)) {
        ++number;
        if (line.size() > 1024 * 1024)
            throw std::runtime_error("Line " + std::to_string(number) + " is larger than the 1 MiB safety limit.");
        if (line.find('\0') != std::string::npos)
            throw std::runtime_error("Source file contains a NUL byte at line " + std::to_string(number) + ".");
        lines.push_back({line, number});
    }
    if (!input.eof() && input.fail()) throw std::runtime_error("Error while reading source file: " + path.string());
    if (lines.size() > 100000) throw std::runtime_error("Source file exceeds the 100,000-line safety limit.");

    // Normalize the documented legacy "end if" / "end repeat" block closers
    // and block headers into the v0.5 curly-brace representation.
    for (auto& source : lines) {
        const std::string current = trim(source.text);
        static const std::regex legacy_end(R"(^end\s+(if|repeat|function|attempt|while|for|otherwise|error)$)", std::regex::icase);
        std::smatch match;
        if (std::regex_match(current, match, legacy_end)) {
            source.text = "}";
            continue;
        }
        if (current.find('{') != std::string::npos || current == "}") continue;
        if (std::regex_match(current, std::regex(R"(^if\s+.+$)", std::regex::icase)) ||
            std::regex_match(current, std::regex(R"(^while\s+.+$)", std::regex::icase)) ||
            std::regex_match(current, std::regex(R"(^repeat\s+.+\s+times$)", std::regex::icase)) ||
            std::regex_match(current, std::regex(R"(^fnc\s+[A-Za-z_][A-Za-z0-9_]*\s*\(.*\)$)", std::regex::icase)) ||
            lower(current) == "attempt" || lower(current) == "otherwise" ||
            std::regex_match(current, std::regex(R"(^error\s*\(\s*\)$)", std::regex::icase))) {
            source.text += " {";
        }
    }
    return lines;
}

int check_source(const fs::path& path) {
    const auto lines = read_source(path);
    int depth = 0;
    bool in_string = false, escaped = false;
    for (const auto& line : lines) {
        if (!scan_braces(line.text, depth, in_string, escaped)) {
            std::cerr << "FRG-CHECK-001: Unexpected '}' at line " << line.number << ".\n";
            return 1;
        }
    }
    if (in_string) {
        std::cerr << "FRG-CHECK-002: Unclosed string literal at end of file.\n";
        return 1;
    }
    if (depth != 0) {
        std::cerr << "FRG-CHECK-003: " << depth << " unclosed block brace(s).\n";
        return 1;
    }

    auto validate_expression = [](const std::string& expression, const SourceLine& source) {
        try {
            (void)ExpressionParser(expression, Interpreter::Variables{}).parse();
            return true;
        } catch (const std::exception& error) {
            std::cerr << "FRG-CHECK-008: Invalid expression at line " << source.number
                      << ": " << error.what() << "\n";
            return false;
        }
    };

    const std::regex fnc_header(R"(^fnc\s+([A-Za-z_][A-Za-z0-9_]*)\s*\((.*?)\)\s*\{$)", std::regex::icase);
    const std::regex attempt_header(R"(^attempt\s*\{$)", std::regex::icase);
    const std::regex catch_header(R"(^error\s*\(\s*\)\s*\{$)", std::regex::icase);
    const std::regex declaration(R"((?:rem|remember)\s+([A-Za-z_][A-Za-z0-9_]*)\s+as\s+(.+))", std::regex::icase);
    const std::regex assignment(R"(set\s+([A-Za-z_][A-Za-z0-9_]*)\s+to\s+(.+))", std::regex::icase);
    const std::regex output(R"(show\s+(.+))", std::regex::icase);
    const std::regex input(R"(ask\s+(.+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase);
    const std::regex stop_error(R"(stop\s+with\s+error\s+(.+))", std::regex::icase);
    const std::regex create_file(R"(create\s+file\s+(.+?)\s+with\s+(.+))", std::regex::icase);
    const std::regex append_file(R"(add\s+(.+?)\s+to\s+file\s+(.+))", std::regex::icase);
    const std::regex read_file(R"(read\s+file\s+(.+?)\s+into\s+([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase);
    const std::regex delete_file(R"(delete\s+file\s+(.+))", std::regex::icase);
    const std::regex delete_call(R"(del\s*\(\s*(.+?)\s*\))", std::regex::icase);
    const std::regex if_header(R"(if\s+(.+?)\s*\{)", std::regex::icase);
    const std::regex while_header(R"(while\s+(.+?)\s*\{)", std::regex::icase);
    const std::regex repeat_header(R"(repeat\s+(.+?)\s+times\s*\{)", std::regex::icase);
    const std::regex subtract(R"(subtract\s+(.+?)\s+from\s+([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase);
    const std::regex multiply(R"(multiply\s+([A-Za-z_][A-Za-z0-9_]*)\s+by\s+(.+))", std::regex::icase);
    const std::regex divide(R"(divide\s+([A-Za-z_][A-Za-z0-9_]*)\s+by\s+(.+))", std::regex::icase);
    const std::regex add_variable(R"(add\s+(.+?)\s+to\s+([A-Za-z_][A-Za-z0-9_]*))", std::regex::icase);
    const std::regex function_call(R"(([A-Za-z_][A-Za-z0-9_]*)\s*\((.*)\)\s*;?)");

    for (const auto& source : lines) {
        const std::string line = trim(source.text);
        if (line.empty() || line == "}" || starts_with_ci(line, "//") || starts_with_ci(line, "note ")) continue;

        std::smatch match;
        if (starts_with_ci(line, "fnc ") && !std::regex_match(line, match, fnc_header)) {
            std::cerr << "FRG-CHECK-004: Function definition must use fnc name(parameters) { at line " << source.number << ".\n";
            return 1;
        }
        if (starts_with_ci(line, "attempt") && !std::regex_match(line, attempt_header)) {
            std::cerr << "FRG-CHECK-005: attempt must be followed by '{' at line " << source.number << ".\n";
            return 1;
        }
        if (starts_with_ci(line, "error") && !std::regex_match(line, catch_header)) {
            std::cerr << "FRG-CHECK-006: Catch block must be written as error() { at line " << source.number << ".\n";
            return 1;
        }

        if (std::regex_match(line, match, declaration) || std::regex_match(line, match, assignment) ||
            std::regex_match(line, match, output) || std::regex_match(line, match, stop_error)) {
            if (!validate_expression(match[match.size() - 1].str(), source)) return 1;
            continue;
        }
        if (std::regex_match(line, match, input)) {
            if (!validate_expression(match[1].str(), source)) return 1;
            continue;
        }
        if (std::regex_match(line, match, create_file) || std::regex_match(line, match, append_file)) {
            if (!validate_expression(match[1].str(), source) ||
                !validate_expression(match[2].str(), source)) return 1;
            continue;
        }
        if (std::regex_match(line, match, read_file) || std::regex_match(line, match, delete_file) ||
            std::regex_match(line, match, delete_call)) {
            const std::string target = match[1].str();
            if (!validate_expression(target, source)) return 1;
            continue;
        }
        if (std::regex_match(line, match, if_header) || std::regex_match(line, match, while_header) ||
            std::regex_match(line, match, repeat_header)) {
            if (!validate_expression(match[1].str(), source)) return 1;
            continue;
        }
        if (std::regex_match(line, match, add_variable)) {
            if (!validate_expression(match[1].str(), source)) return 1;
            continue;
        }
        if (std::regex_match(line, match, subtract)) {
            if (!validate_expression(match[1].str(), source)) return 1;
            continue;
        }
        if (std::regex_match(line, match, multiply) || std::regex_match(line, match, divide)) {
            if (!validate_expression(match[2].str(), source)) return 1;
            continue;
        }
        if (std::regex_match(line, match, fnc_header)) {
            try {
                const auto params = split_arguments(match[2].str());
                for (const auto& param : params) {
                    if (!std::regex_match(param, std::regex(R"([A-Za-z_][A-Za-z0-9_]*)"))) {
                        std::cerr << "FRG-CHECK-009: Invalid function parameter at line " << source.number << ": " << param << "\n";
                        return 1;
                    }
                }
            } catch (const std::exception& error) {
                std::cerr << "FRG-CHECK-009: Invalid function parameters at line " << source.number << ": " << error.what() << "\n";
                return 1;
            }
            continue;
        }
        if (std::regex_match(line, match, attempt_header) || std::regex_match(line, match, catch_header) ||
            starts_with_ci(line, "otherwise") || starts_with_ci(line, "draw ") ||
            starts_with_ci(line, "load 3d ") || starts_with_ci(line, "add obj ")) continue;
        if (starts_with_ci(line, "return")) {
            const std::string expression = trim(line.substr(6));
            if (!expression.empty() && !validate_expression(expression, source)) return 1;
            continue;
        }
        if (std::regex_match(line, match, function_call)) {
            try {
                for (const auto& argument : split_arguments(match[2].str())) {
                    if (!validate_expression(argument, source)) return 1;
                }
            } catch (const std::exception& error) {
                std::cerr << "FRG-CHECK-010: Invalid function call at line " << source.number << ": " << error.what() << "\n";
                return 1;
            }
            continue;
        }

        std::cerr << "FRG-CHECK-007: Unknown statement at line " << source.number << ": " << line << "\n";
        return 1;
    }

    std::cout << "FRIDGE check passed: " << lines.size()
              << " line(s), block structure, supported statements and expression syntax are valid.\n";
    return 0;
}

void print_help() {
    std::cout << "FRIDGE v" << FRIDGE_VERSION << "\n"
              << "Usage:\n"
              << "  fridge run <file.frg>     Execute a FRIDGE program\n"
              << "  fridge check <file.frg>   Validate source without executing it\n"
              << "  fridge update             Check and install the latest official release\n"
              << "  fridge version            Display the installed version\n"
              << "  fridge help               Display this help\n"
              << "\nPrograms use the current directory as their project root for file I/O.\n";
}

int run_program(const fs::path& path) {
#ifndef FRIDGE_TESTING
    fridge_update::ReleaseInfo release;
    std::string error;
    if (!fridge_update::check_latest(release, error)) {
        std::cerr << "[FRG-UPD-REQUIRED] Cannot verify the official FRIDGE release: " << error << "\n"
                  << "Execution is blocked. Connect to the internet and run: fridge update\n";
        return 3;
    }
    if (fridge_update::update_required(release)) {
        std::cerr << "[FRG-UPD-REQUIRED] This FRIDGE build (v" << FRIDGE_VERSION
                  << ") is out of date. The latest release is " << release.latest_version << ".\n"
                  << "Execution is blocked until you install the update. Run: fridge update\n";
        return 3;
    }
#endif
    try {
        Interpreter interpreter;
        interpreter.execute(read_source(path));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FRG-RUN-001] " << error.what() << '\n';
        return 1;
    }
}

void print_version() {
    std::cout << "FRIDGE " << FRIDGE_VERSION << "\n"
#ifdef _WIN32
              << "Platform: Windows\n"
#elif defined(__APPLE__)
              << "Platform: macOS\n"
#elif defined(__ANDROID__)
              << "Platform: Android (automatic package updates unsupported)\n"
#elif defined(__linux__)
              << "Platform: Linux\n"
#else
              << "Platform: Unknown\n"
#endif
              << "Language runtime: C++17\n";
}
} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) { print_help(); return 0; }
        const std::string command = argv[1];
        if (command == "help" || command == "--help" || command == "-h") {
            print_help(); return 0;
        }
        if (command == "version" || command == "--version" || command == "-v") {
            print_version(); return 0;
        }
        if (command == "update") return fridge_update::run_update_command();

        if (command == "run" || command == "check") {
            if (argc != 3) {
                std::cerr << "Usage: fridge " << command << " <file.frg>\n";
                return 2;
            }
            const fs::path path = fs::u8path(argv[2]);
            if (command == "check") {
                try { return check_source(path); }
                catch (const std::exception& error) {
                    std::cerr << "[FRG-CHECK-000] " << error.what() << '\n';
                    return 1;
                }
            }
            return run_program(path);
        }

        // Preserve the v0.5.0 direct-file invocation while fixing editor commands.
        if (argc == 2 && fs::path(command).extension() == ".frg")
            return run_program(fs::u8path(command));

        std::cerr << "Unknown command: " << command << "\nRun 'fridge help' for usage.\n";
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "[FRG-CLI-001] " << error.what() << '\n';
        return 1;
    }
}
