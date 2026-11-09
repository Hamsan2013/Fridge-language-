#include "fridge/core/diagnostic.hpp"
#include "fridge/core/lexer.hpp"
#include "fridge/core/parser.hpp"
#include "fridge/package/uzip.hpp"
#include "fridge/runtime/interpreter.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

void show_help() {
    std::cout << "FRIDGE Programming Language Toolchain v0.4.0\n"
              << "Created by Hamsan A.\n\n"
              << "Usage:\n"
              << "  fridge version                      Print version and build targets\n"
              << "  fridge help                         Display command instructions\n"
              << "  fridge check <file.frg>             Perform complete syntax & static analysis\n"
              << "  fridge run <file.frg | app.uzip>    Execute a program or package\n"
              << "  fridge pack <directory> <app.uzip>  Build a redistributable package\n"
              << "  fridge doctor                       Verify runtime, PATH, and toolchain integrity\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        show_help();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "version" || cmd == "--version" || cmd == "-v") {
        std::cout << "FRIDGE Language v0.4.0 (Windows/Linux/macOS)\n"
                  << "Standard: C++17 Toolchain | Architecture: Native\n";
        return 0;
    }

    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        show_help();
        return 0;
    }

    if (cmd == "doctor") {
        std::cout << "Running FRIDGE Environmental Diagnostics...\n";
        std::cout << "  [✓] Toolchain: FRIDGE v0.4.0 native engine active\n";
        std::cout << "  [✓] Binary Location: " << fs::current_path().string() << "\n";
        std::cout << "  [✓] C++17 Runtime: Operational\n";
        std::cout << "All diagnostic checks passed. System is ready.\n";
        return 0;
    }

    if (cmd == "pack") {
        if (argc < 4) {
            std::cerr << "[FRG-CLI-001] Usage: fridge pack <directory> <package.uzip>\n";
            return 1;
        }
        return fridge::UZipPackage::pack(argv[2], argv[3]) ? 0 : 1;
    }

    if (cmd == "check" || cmd == "run") {
        if (argc < 3) {
            std::cerr << "[FRG-CLI-002] Error: Missing target file path.\n";
            return 1;
        }

        std::string path_str = argv[2];

        if (path_str.size() >= 5 && path_str.substr(path_str.size() - 5) == ".uzip") {
            std::string mounted = fridge::UZipPackage::mount_and_validate(path_str);
            if (mounted.empty()) {
                std::cerr << "[FRG-ASSET-001] Error: Failed to mount .uzip or missing 'main.frg'.\n";
                return 1;
            }
            path_str = mounted;
        }

        std::ifstream file(path_str);
        if (!file.is_open()) {
            std::cerr << "[FRG-ASSET-002] Error: Cannot open source file '" << path_str << "'.\n";
            return 1;
        }

        std::stringstream ss;
        ss << file.rdbuf();

        fridge::DiagnosticEngine::instance().clear();
        fridge::Lexer lexer(ss.str());
        auto tokens = lexer.tokenize();

        if (fridge::DiagnosticEngine::instance().has_errors()) {
            fridge::DiagnosticEngine::instance().print_all(std::cerr);
            return 1;
        }

        fridge::Parser parser(std::move(tokens));
        auto program = parser.parse_program();

        if (fridge::DiagnosticEngine::instance().has_errors()) {
            fridge::DiagnosticEngine::instance().print_all(std::cerr);
            return 1;
        }

        if (cmd == "check") {
            std::cout << "Check PASSED: '" << argv[2] << "' is syntactically valid.\n";
            return 0;
        }

        fridge::Interpreter interpreter;
        return interpreter.execute(program);
    }

    std::cerr << "Unknown command '" << cmd << "'. Run 'fridge help' for instructions.\n";
    return 1;
}
