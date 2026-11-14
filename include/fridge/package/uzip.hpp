#pragma once
#include <string>

namespace fridge {

class UzipPackage {
public:
    static bool pack(const std::string& inputFile, const std::string& outputUzip);
    static std::string unpackToSource(const std::string& uzipPath);
};

} // namespace fridge
