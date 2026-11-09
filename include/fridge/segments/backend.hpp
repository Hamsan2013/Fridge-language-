#pragma once
#include <string>

namespace fridge {

enum class BackendType {
    Console,
    DesktopUI,
    Game2D,
    Game3D
};

class BackendInterface {
public:
    virtual ~BackendInterface() = default;
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
    virtual BackendType type() const = 0;
};

class ConsoleBackend : public BackendInterface {
public:
    void initialize() override {}
    void shutdown() override {}
    BackendType type() const override { return BackendType::Console; }
};

void init_active_backend(BackendType type);

} // namespace fridge
