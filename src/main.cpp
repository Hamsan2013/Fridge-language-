#include "fridge/core/lexer.hpp"
#include "fridge/core/parser.hpp"
#include "fridge/runtime/interpreter.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void print_help() {
    std::cout << "FRIDGE Programming Language v0.3.0 (Created by Hamsan A)\n"
              << "Simple English. Real programs. 2D and 3D worlds.\n\n"
              << "Commands:\n"
              << "  fridge version            Show FRIDGE version\n"
              << "  fridge check <file.frg>   Check syntax without running\n"
              << "  fridge run <file.frg>     Run a FRIDGE program or .uzip package\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "version" || cmd == "--version" || cmd == "-v") {
        std::cout << "FRIDGE v0.3.0 (Native C++17 Toolchain - Created by Hamsan A)\n";
        return 0;
    }

    if (cmd == "check" || cmd == "run") {
        if (argc < 3) {
            std::cerr << "[FRG-CLI-001] Error: Please provide a .frg or .uzip file path.\n";
            return 1;
        }

        std::string path = argv[2];
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[FRG-ASSET-001] Error: Could not open file '" << path << "'.\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        fridge::Lexer lexer(buffer.str());
        auto tokens = lexer.tokenize();

        fridge::Parser parser(std::move(tokens));
        auto program = parser.parse();

        if (parser.has_errors()) {
            for (const auto& err : parser.errors()) {
                std::cerr << err << "\n";
            }
            return 1;
        }

        if (cmd == "check") {
            std::cout << "Check passed: '" << path << "' has valid FRIDGE syntax.\n";
            return 0;
        }

        fridge::Interpreter interpreter;
        return interpreter.execute(program);
    }

    print_help();
    return 1;
}
