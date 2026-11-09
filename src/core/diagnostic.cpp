#pragma once
#include "fridge/core/token.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fridge {

enum class DiagnosticSeverity {
    Error,
    Warning,
    Note
};

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string code;
    std::string message;
    SourceLocation location;
    std::string suggestion;
};

class DiagnosticEngine {
public:
    static DiagnosticEngine& instance() {
        static DiagnosticEngine eng;
        return eng;
    }

    void report(Diagnostic diag) {
        diagnostics_.push_back(std::move(diag));
    }

    void error(std::string code, std::string msg, SourceLocation loc, std::string sug = "") {
        report({DiagnosticSeverity::Error, std::move(code), std::move(msg), loc, std::move(sug)});
    }

    bool has_errors() const {
        for (const auto& d : diagnostics_) {
            if (d.severity == DiagnosticSeverity::Error) return true;
        }
        return false;
    }

    const std::vector<Diagnostic>& list() const { return diagnostics_; }
    void clear() { diagnostics_.clear(); }

    void print_all(std::ostream& os) const {
        for (const auto& d : diagnostics_) {
            os << "[" << d.code << "] "
               << (d.severity == DiagnosticSeverity::Error ? "Error: " : "Warning: ")
               << d.message << "\n"
               << "  --> " << d.location.filename << ":" << d.location.line << ":" << d.location.column << "\n";
            if (!d.suggestion.empty()) {
                os << "  Suggestion: " << d.suggestion << "\n";
            }
            os << "\n";
        }
    }

    std::string to_json() const {
        std::ostringstream ss;
        ss << "[\n";
        for (size_t i = 0; i < diagnostics_.size(); ++i) {
            const auto& d = diagnostics_[i];
            ss << "  {\n"
               << "    \"code\": \"" << d.code << "\",\n"
               << "    \"severity\": \"" << (d.severity == DiagnosticSeverity::Error ? "error" : "warning") << "\",\n"
               << "    \"message\": \"" << d.message << "\",\n"
               << "    \"file\": \"" << d.location.filename << "\",\n"
               << "    \"line\": " << d.location.line << ",\n"
               << "    \"column\": " << d.location.column << ",\n"
               << "    \"suggestion\": \"" << d.suggestion << "\"\n"
               << "  }" << (i + 1 < diagnostics_.size() ? "," : "") << "\n";
        }
        ss << "]";
        return ss.str();
    }

private:
    DiagnosticEngine() = default;
    std::vector<Diagnostic> diagnostics_;
};

} // namespace fridge
