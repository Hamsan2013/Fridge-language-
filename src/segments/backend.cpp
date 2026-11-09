#include "fridge/segments/backend.hpp"
#include <memory>

namespace fridge {

static std::unique_ptr<BackendInterface> current_backend = nullptr;

void init_active_backend(BackendType type) {
    if (type == BackendType::Console) {
        current_backend = std::make_unique<ConsoleBackend>();
        current_backend->initialize();
    }
}

} // namespace fridge
