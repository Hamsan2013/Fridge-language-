#pragma once
#include <string>
#include <vector>

namespace fridge {

enum class Severity {
    INFO,
    WARNING,
    ERROR
};

struct Diagnostic {
    Severity severity;
    std::string message;
    int line = 1;
    int column = 1;
};

class DiagnosticEngine {
public:
    void report(Severity sev, int line, int col, const std::string& msg);
    bool hasErrors() const;
    const std::vector<Diagnostic>& getDiagnostics() const;
    void printAll() const;
    void clear();

private:
    std::vector<Diagnostic> diagnostics;
};

} // namespace fridge
