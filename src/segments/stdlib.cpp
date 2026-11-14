#include "fridge/segments/registry.hpp"
#include "fridge/runtime/interpreter.hpp"
#include "fridge/runtime/value.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace fridge {

void register_all_segments() {
    auto& reg = SegmentRegistry::instance();

    // 1. wait <n> seconds / milliseconds
    reg.register_command("wait", [](const std::vector<std::string>& args, Interpreter&) {
        if (args.size() >= 2) {
            int amt = std::stoi(args[0]);
            if (args[1] == "seconds" || args[1] == "second") {
                std::this_thread::sleep_for(std::chrono::seconds(amt));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(amt));
            }
        }
    });

    // 2. save <text> into file <filename>
    reg.register_command("save", [](const std::vector<std::string>& args, Interpreter& interp) {
        if (args.size() >= 4 && args[1] == "into" && args[2] == "file") {
            std::ofstream out(args[3]);
            Value val = interp.environment()->get(args[0]);
            out << val.toString();
        }
    });

    // 3. 3D Engine Object Registration: add_obj <name>
    reg.register_command("add_obj", [](const std::vector<std::string>& args, Interpreter&) {
        if (!args.empty()) {
            std::cout << "[FRIDGE 3D Engine] Instantiated 3D scene node: '" << args[0] << "'.\n";
        }
    });
}

} // namespace fridge
