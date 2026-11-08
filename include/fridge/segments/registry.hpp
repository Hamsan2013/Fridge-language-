#pragma once
#include "fridge/core/token.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fridge {

class Interpreter;

using SegmentHandler = std::function<void(const std::vector<Token>&, Interpreter&)>;

class SegmentRegistry {
public:
    static SegmentRegistry& instance() {
        static SegmentRegistry reg;
        return reg;
    }

    void register_command(const std::string& keyword, SegmentHandler handler) {
        handlers_[keyword] = std::move(handler);
    }

    bool has_command(const std::string& keyword) const {
        return handlers_.find(keyword) != handlers_.end();
    }

    void execute(const std::string& keyword, const std::vector<Token>& tokens, Interpreter& interp) {
        auto it = handlers_.find(keyword);
        if (it != handlers_.end()) {
            it->second(tokens, interp);
        }
    }

private:
    SegmentRegistry() = default;
    std::unordered_map<std::string, SegmentHandler> handlers_;
};

void register_all_segments();

} // namespace fridge
