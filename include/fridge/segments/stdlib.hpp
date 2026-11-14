#pragma once
#include <string>

namespace fridge {

class StdLib {
public:
    static bool createFile(const std::string& path, const std::string& content, std::string& errOut);
    static bool appendFile(const std::string& path, const std::string& content, std::string& errOut);
    static bool readFile(const std::string& path, std::string& contentOut, std::string& errOut);
    static bool deleteFile(const std::string& path, std::string& errOut);
};

} // namespace fridge
