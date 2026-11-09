#pragma once
#include <filesystem>
#include <string>

namespace fridge {

class UZipPackage {
public:
    static bool pack(const std::filesystem::path& folder, const std::filesystem::path& output_uzip);
    static bool unpack(const std::filesystem::path& input_uzip, const std::filesystem::path& target_dir);
    static std::string mount_and_validate(const std::filesystem::path& uzip_path);

private:
    static bool is_safe_path(const std::filesystem::path& base, const std::filesystem::path& path);
};

} // namespace fridge
