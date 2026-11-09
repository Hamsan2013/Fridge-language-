#pragma once
#include "fridge/core/token.hpp"
#include <iostream>
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

    void print_all(std::ostream& os) const;
    std::string to_json() const;

private:
    DiagnosticEngine() = default;
    std::vector<Diagnostic> diagnostics_;
};

} // namespace fridge
