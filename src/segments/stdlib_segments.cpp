#include "fridge/segments/registry.hpp"
#include "fridge/runtime/interpreter.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace fridge {

static std::string lower_word(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static std::string fmt_num(double v) {
    if (std::floor(v) == v) return std::to_string(static_cast<long long>(v));
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

void register_all_segments() {
    auto& reg = SegmentRegistry::instance();

    // 1. show <expression>
    reg.register_command("show", [](const std::vector<Token>& t, Interpreter& interp) {
        std::cout << interp.evaluate_tokens(t, 1) << "\n";
    });

    // 2. remember <name> as <value> (Supports variables and 'list containing ...')
    reg.register_command("remember", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "as") {
            if (t.size() >= 5 && lower_word(t[3].lexeme) == "list" && lower_word(t[4].lexeme) == "containing") {
                std::vector<std::string> items;
                for (size_t i = 5; i < t.size(); ++i) {
                    if (t[i].type == TokenType::Comma) continue;
                    items.push_back(interp.evaluate_tokens({t[i]}, 0));
                }
                interp.set_list(t[1].lexeme, items);
                return;
            }
            interp.set_var(t[1].lexeme, interp.evaluate_tokens(t, 3));
        }
    });

    // 3. set <name> to <value> OR set item <idx> of <list> to <val>
    reg.register_command("set", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 7 && lower_word(t[1].lexeme) == "item" && lower_word(t[3].lexeme) == "of" && lower_word(t[5].lexeme) == "to") {
            int idx = std::stoi(interp.evaluate_tokens({t[2]}, 0));
            auto& lst = interp.get_list(t[4].lexeme);
            if (idx >= 1 && idx <= static_cast<int>(lst.size())) {
                lst[idx - 1] = interp.evaluate_tokens(t, 6);
            }
            return;
        }
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "to") {
            interp.set_var(t[1].lexeme, interp.evaluate_tokens(t, 3));
        }
    });

    // 4. append <val> to <list>
    reg.register_command("append", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "to") {
            interp.get_list(t[3].lexeme).push_back(interp.evaluate_tokens({t[1]}, 0));
        }
    });

    // 5. Math mutations: add, subtract, multiply, divide
    reg.register_command("add", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 3 && lower_word(t[1].lexeme) == "obj") {
            std::cout << "[3D Engine] Added OBJ '" << t[2].lexeme << "' to InfiniteWorld scene graph.\n";
            return;
        }
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "to") {
            double cur = std::stod(interp.get_var(t[3].lexeme).empty() ? "0" : interp.get_var(t[3].lexeme));
            double val = std::stod(interp.evaluate_tokens({t[1]}, 0));
            interp.set_var(t[3].lexeme, fmt_num(cur + val));
        }
    });

    reg.register_command("subtract", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "from") {
            double cur = std::stod(interp.get_var(t[3].lexeme).empty() ? "0" : interp.get_var(t[3].lexeme));
            double val = std::stod(interp.evaluate_tokens({t[1]}, 0));
            interp.set_var(t[3].lexeme, fmt_num(cur - val));
        }
    });

    reg.register_command("multiply", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "by") {
            double cur = std::stod(interp.get_var(t[1].lexeme).empty() ? "0" : interp.get_var(t[1].lexeme));
            double val = std::stod(interp.evaluate_tokens({t[3]}, 0));
            interp.set_var(t[1].lexeme, fmt_num(cur * val));
        }
    });

    reg.register_command("divide", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 4 && lower_word(t[2].lexeme) == "by") {
            double cur = std::stod(interp.get_var(t[1].lexeme).empty() ? "0" : interp.get_var(t[1].lexeme));
            double val = std::stod(interp.evaluate_tokens({t[3]}, 0));
            if (val != 0) interp.set_var(t[1].lexeme, fmt_num(cur / val));
        }
    });

    // 6. ask "<prompt>" into <var>
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

    // 7. listen for key/button/log ...
    reg.register_command("listen", [](const std::vector<Token>& t, Interpreter&) {
        if (t.size() >= 4 && lower_word(t[1].lexeme) == "for") {
            std::cout << "[Event Listener] Listening for " << t[2].lexeme << ": " << t[3].lexeme << "\n";
        }
    });

    // 8. 3D Hierarchies & World Movement
    reg.register_command("child", [](const std::vector<Token>&, Interpreter&) {
        std::cout << "[3D Engine] Attaching child hierarchy block {...}\n";
    });

    reg.register_command("move", [](const std::vector<Token>& t, Interpreter&) {
        if (t.size() >= 4 && lower_word(t[1].lexeme) == "world" && lower_word(t[2].lexeme) == "environment") {
            std::cout << "[3D World] Moving object '" << t[3].lexeme << "' in environment.\n";
        }
    });

    reg.register_command("play", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 3 && lower_word(t[1].lexeme) == "sound") {
            std::string s = interp.has_var(t[2].lexeme) ? interp.get_var(t[2].lexeme) : t[2].lexeme;
            std::cout << "[Audio Engine] Playing sound: " << s << "\n";
        }
    });

    // 9. File I/O: save & read
    reg.register_command("save", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 5 && lower_word(t[2].lexeme) == "into" && lower_word(t[3].lexeme) == "file") {
            std::ofstream out(t[4].lexeme);
            out << interp.evaluate_tokens({t[1]}, 0);
        }
    });

    reg.register_command("read", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 5 && lower_word(t[1].lexeme) == "file" && lower_word(t[3].lexeme) == "into") {
            std::ifstream in(t[2].lexeme);
            std::stringstream buf;
            buf << in.rdbuf();
            interp.set_var(t[4].lexeme, buf.str());
        }
    });

    // 10. wait <n> seconds / milliseconds
    reg.register_command("wait", [](const std::vector<Token>& t, Interpreter& interp) {
        if (t.size() >= 3) {
            int amount = std::stoi(interp.evaluate_tokens({t[1]}, 0));
            std::string unit = lower_word(t[2].lexeme);
            if (unit == "seconds" || unit == "second") {
                std::this_thread::sleep_for(std::chrono::seconds(amount));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(amount));
            }
        }
    });
}

} // namespace fridge
