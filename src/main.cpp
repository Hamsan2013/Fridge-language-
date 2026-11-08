#include "fridge/core/lexer.hpp"
#include "fridge/core/parser.hpp"
#include "fridge/runtime/interpreter.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

void print_help() {
    std::cout << "FRIDGE Programming Language v0.3.0 (Created by Hamsan A)\n"
              << "Simple English. Real programs. 2D and 3D worlds.\n\n"
              << "Commands:\n"
              << "  fridge version                      Show FRIDGE version\n"
              << "  fridge check <file.frg>             Validate FRIDGE syntax\n"
              << "  fridge run <file.frg | app.uzip>    Execute a .frg script or .uzip package\n"
              << "  fridge pack <folder> <output.uzip>  Build a .uzip application package\n";
}

// Helper to extract .uzip archive into a temporary folder and return main.frg path
std::string mount_uzip_package(const std::string& uzip_path) {
    fs::path temp_dir = fs::temp_directory_path() / "fridge_uzip_runtime";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir);

#ifdef _WIN32
    // Windows 10/11 has built-in tar.exe that extracts zip/uzip archives natively
    std::string cmd = "tar -xf \"" + uzip_path + "\" -C \"" + temp_dir.string() + "\"";
#else
    std::string cmd = "unzip -q \"" + uzip_path + "\" -d \"" + temp_dir.string() + "\"";
#endif

    if (std::system(cmd.c_str()) != 0) {
        return "";
    }

    fs::path entry_file = temp_dir / "main.frg";
    if (fs::exists(entry_file)) {
        return entry_file.string();
    }
    return "";
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

    // Build a .uzip Universal Package from a project directory
    if (cmd == "pack") {
        if (argc < 4) {
            std::cerr << "[FRG-CLI-002] Usage: fridge pack <project_folder> <output.uzip>\n";
            return 1;
        }
        std::string folder = argv[2];
        std::string out_uzip = argv[3];

        if (!fs::exists(fs::path(folder) / "main.frg")) {
            std::cerr << "[FRG-PACK-001] Error: Project folder must contain 'main.frg'.\n";
            return 1;
        }

#ifdef _WIN32
        std::string zip_cmd = "tar -a -c -f \"" + out_uzip + "\" -C \"" + folder + "\" .";
#else
        std::string zip_cmd = "cd \"" + folder + "\" && zip -r \"" + fs::absolute(out_uzip).string() + "\" .";
#endif
        if (std::system(zip_cmd.c_str()) == 0) {
            std::cout << "Successfully packaged '" << folder << "' into '" << out_uzip << "'!\n";
            return 0;
        } else {
            std::cerr << "[FRG-PACK-002] Error: Failed to build .uzip archive.\n";
            return 1;
        }
    }

    if (cmd == "check" || cmd == "run") {
        if (argc < 3) {
            std::cerr << "[FRG-CLI-001] Error: Please provide a .frg or .uzip file path.\n";
            return 1;
        }

        std::string path = argv[2];

        // Automatically detect and unpack .uzip packages
        if (path.size() >= 5 && path.substr(path.size() - 5) == ".uzip") {
            std::string extracted_main = mount_uzip_package(path);
            if (extracted_main.empty()) {
                std::cerr << "[FRG-ASSET-002] Error: Invalid .uzip package or missing 'main.frg' inside '" << path << "'.\n";
                return 1;
            }
            path = extracted_main;
        }

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
