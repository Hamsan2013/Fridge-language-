#include "fridge/segments/registry.hpp"
#include "fridge/runtime/interpreter.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

namespace fridge {

static std::string lower_word(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

void register_all_segments() {
    auto& reg = SegmentRegistry::instance();

    // 1. show <expression>
    reg.register_command("show", [](const std::vector<Token>& t, Interpreter& interp) {
        std::cout << interp.evaluate_tokens(t, 1) << "\n";
    });

    // 2. remember <name> as <value>
    reg.register_command("remember", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "as") {
            interp.set_var(t[1].lexeme, interp.evaluate_tokens(t, 3));
        }
    });

    // 3. set <name> to <value>
    reg.register_command("set", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "to") {
            interp.set_var(t[1].lexeme, interp.evaluate_tokens(t, 3));
        }
    });

    // 4. ask "<prompt>" into <var>
    reg.register_command("ask", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "into") {
            std::cout << t[1].lexeme;
            std::string input;
            std::getline(std::cin, input);
            interp.set_var(t[3].lexeme, input);
        } else if (t.size() >= 3 && lower_word(t[1].lexeme) == "into") {
            std::string input;
            std::getline(std::cin, input);
            interp.set_var(t[2].lexeme, input);
        }
    });

    // 5. listen for key/button/log ...
    reg.register_command("listen", [](const std::vector<Token>& t, Interpreter&) {
        if (t.size() >= 4 && lower_word(t[1].lexeme) == "for") {
            std::cout << "[Event Listener Registered] Listening for " << t[2].lexeme << ": " << t[3].lexeme << "\n";
        }
    });

    // 6. add <number> to <var> OR add OBJ <name>
    reg.register_command("add", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 3 && lower_word(t[1].lexeme) == "obj") {
            std::cout << "[3D Engine] Added OBJ '" << t[2].lexeme << "' to InfiniteWorld scene graph.\n";
            return;
        }
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "to") {
            double current = std::stod(interp.get_var(t[3].lexeme).empty() ? "0" : interp.get_var(t[3].lexeme));
            double delta = std::stod(interp.evaluate_tokens({t[1]}, 0));
            interp.set_var(t[3].lexeme, std::to_string(current + delta));
        }
    });

    // 7. child { ... }
    reg.register_command("child", [](const std::vector<Token>&, Interpreter&) {
        std::cout << "[3D Engine] Attaching child hierarchy block {...}\n";
    });

    // 8. move world environment <obj> by vector3(...)
    reg.register_command("move", [](const std::vector<Token>& t, Interpreter&) {
        if (t.size() >= 4 && lower_word(t[1].lexeme) == "world" && lower_word(t[2].lexeme) == "environment") {
            std::cout << "[3D World] Moving object '" << t[3].lexeme << "' in environment.\n";
        }
    });

    // 9. play sound <asset>
    reg.register_command("play", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 3 && lower_word(t[1].lexeme) == "sound") {
            std::string asset_name = interp.has_var(t[2].lexeme) ? interp.get_var(t[2].lexeme) : t[2].lexeme;
            std::cout << "[Audio Engine] Playing sound: " << asset_name << "\n";
        }
    });

    // 10. wait <n> seconds / milliseconds
    reg.register_command("wait", [](const std::vector<Token>& t, Interpreter&) {
        if (t.size() >= 3) {
            int amount = std::stoi(t[1].lexeme);
            std::string unit = lower_word(t[2].lexeme);
            if (unit == "seconds" || unit == "second") {
                std::this_thread::sleep_for(std::chrono::seconds(amount));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(amount));
            }
        }
    });

    // 11. save <text> into file <filename>
    reg.register_command("save", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 5 && lower_word(t[2].lexeme) == "into" && lower_word(t[3].lexeme) == "file") {
            std::ofstream out(t[4].lexeme);
            out << interp.evaluate_tokens({t[1]}, 0);
        }
    });
}

} // namespace fridge
