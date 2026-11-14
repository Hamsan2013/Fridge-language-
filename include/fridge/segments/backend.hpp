#pragma once
#include <string>

namespace fridge {

class Backend {
public:
    static void spawn3DNode(const std::string& name);
    static void load3DModelAsset(const std::string& assetPath, const std::string& objectName);
    static void draw2DRect(const std::string& x, const std::string& y, const std::string& w, const std::string& h, const std::string& color);
    static void draw2DCircle(const std::string& x, const std::string& y, const std::string& r, const std::string& color);
    static void spawn2DSprite(const std::string& id, const std::string& x, const std::string& y);
};

} // namespace fridge
