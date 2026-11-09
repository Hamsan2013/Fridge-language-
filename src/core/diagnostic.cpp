#include "fridge/core/diagnostic.hpp"
#include <sstream>

namespace fridge {

void DiagnosticEngine::print_all(std::ostream& os) const {
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

std::string DiagnosticEngine::to_json() const {
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

} // namespace fridge
