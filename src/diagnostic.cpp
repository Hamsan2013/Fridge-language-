#include "fridge/core/diagnostic.hpp"
#include <iostream>

namespace fridge {

void DiagnosticEngine::report(Severity sev, int line, int col, const std::string& msg) {
    diagnostics.push_back({ sev, msg, line, col });
}

bool DiagnosticEngine::hasErrors() const {
    for (const auto& d : diagnostics) {
        if (d.severity == Severity::ERROR) return true;
    }
    return false;
}

const std::vector<Diagnostic>& DiagnosticEngine::getDiagnostics() const {
    return diagnostics;
}

void DiagnosticEngine::printAll() const {
    for (const auto& d : diagnostics) {
        std::string prefix = (d.severity == Severity::ERROR) ? "[ERROR]" :
                             (d.severity == Severity::WARNING) ? "[WARN]" : "[INFO]";
        std::cerr << prefix << " Line " << d.line << ":" << d.column << " - " << d.message << std::endl;
    }
}

void DiagnosticEngine::clear() {
    diagnostics.clear();
}

} // namespace fridge
