#include "fridge/segments/backend.hpp"

#include <iostream>

namespace fridge {

void Backend::spawn3DNode(const std::string& name) {
    std::cout << "[FRIDGE 3D Engine] Instantiated 3D scene node: '" << name << "'." << std::endl;
}

void Backend::load3DModelAsset(const std::string& assetPath, const std::string& objectName) {
    std::cout << "[FRIDGE 3D Asset] Loaded model '" << assetPath << "' as '" << objectName << "'." << std::endl;
}

void Backend::draw2DRect(const std::string& x, const std::string& y, const std::string& w,
                         const std::string& h, const std::string& color) {
    std::cout << "[FRIDGE 2D Engine] Rectangle at " << x << "," << y << " size " << w << "x" << h
              << " color '" << color << "'." << std::endl;
}

void Backend::draw2DCircle(const std::string& x, const std::string& y, const std::string& r,
                           const std::string& color) {
    std::cout << "[FRIDGE 2D Engine] Circle at " << x << "," << y << " radius " << r
              << " color '" << color << "'." << std::endl;
}

void Backend::spawn2DSprite(const std::string& id, const std::string& x, const std::string& y) {
    std::cout << "[FRIDGE 2D Engine] Instantiated sprite '" << id << "' at " << x << "," << y << "."
              << std::endl;
}

} // namespace fridge
