#include "fridge/package/uzip.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace fridge {

bool UZipPackage::is_safe_path(const fs::path& base, const fs::path& path) {
    auto rel = fs::relative(path, base);
    return !rel.empty() && rel.native()[0] != '.' && rel.string().find("..") == std::string::npos;
}

bool UZipPackage::pack(const fs::path& folder, const fs::path& output_uzip) {
    if (!fs::exists(folder / "main.frg")) {
        std::cerr << "[FRG-PACK-001] Package error: Root directory must contain 'main.frg'.\n";
        return false;
    }

#ifdef _WIN32
    std::string cmd = "tar.exe -a -c -f \"" + output_uzip.string() + "\" -C \"" + folder.string() + "\" .";
#else
    std::string cmd = "tar -czf \"" + output_uzip.string() + "\" -C \"" + folder.string() + "\" .";
#endif

    int res = std::system(cmd.c_str());
    return res == 0;
}

bool UZipPackage::unpack(const fs::path& input_uzip, const fs::path& target_dir) {
    fs::create_directories(target_dir);

#ifdef _WIN32
    std::string cmd = "tar.exe -xf \"" + input_uzip.string() + "\" -C \"" + target_dir.string() + "\"";
#else
    std::string cmd = "tar -xzf \"" + input_uzip.string() + "\" -C \"" + target_dir.string() + "\"";
#endif

    int res = std::system(cmd.c_str());
    return res == 0;
}

std::string UZipPackage::mount_and_validate(const fs::path& uzip_path) {
    fs::path temp_dir = fs::temp_directory_path() / "fridge_runtime_mount";
    fs::remove_all(temp_dir);

    if (!unpack(uzip_path, temp_dir)) {
        return "";
    }

    fs::path entry = temp_dir / "main.frg";
    if (fs::exists(entry)) {
        return entry.string();
    }
    return "";
}

} // namespace fridge
