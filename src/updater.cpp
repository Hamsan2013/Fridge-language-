#include "updater.hpp"

#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

#ifndef FRIDGE_VERSION
#define FRIDGE_VERSION "0.5.2"
#endif

namespace fridge_update {
namespace {
constexpr const char* kReleaseApi =
    "https://api.github.com/repos/Hamsan2013/Fridge-language-/releases/latest";
constexpr const char* kDownloadPrefix =
    "https://github.com/Hamsan2013/Fridge-language-/releases/download/";

std::string trim_v(std::string value) {
    if (!value.empty() && (value.front() == 'v' || value.front() == 'V')) value.erase(value.begin());
    return value;
}

std::vector<unsigned long long> version_parts(const std::string& value) {
    std::vector<unsigned long long> parts;
    std::string v = trim_v(value);
    std::regex pattern(R"(^([0-9]+)\.([0-9]+)\.([0-9]+)(?:[-+].*)?$)");
    std::smatch m;
    if (!std::regex_match(v, m, pattern)) return {};
    try {
        for (int i = 1; i <= 3; ++i) parts.push_back(std::stoull(m[i].str()));
    } catch (...) {
        return {};
    }
    return parts;
}

int compare_versions(const std::string& left, const std::string& right) {
    const auto a = version_parts(left);
    const auto b = version_parts(right);
    if (a.empty() || b.empty()) return 0;
    for (size_t i = 0; i < 3; ++i) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    return 0;
}

std::string json_string_field(const std::string& object, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    size_t p = object.find(needle);
    if (p == std::string::npos) return {};
    p = object.find(':', p + needle.size());
    if (p == std::string::npos) return {};
    ++p;
    while (p < object.size() && (object[p] == ' ' || object[p] == '\t' || object[p] == '\r' || object[p] == '\n')) ++p;
    if (p >= object.size() || object[p] != '"') return {};
    ++p;
    std::string out;
    while (p < object.size()) {
        char c = object[p++];
        if (c == '"') return out;
        if (c == '\\') {
            if (p >= object.size()) return {};
            char escaped = object[p++];
            switch (escaped) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                default: return {};
            }
        } else {
            out += c;
        }
    }
    return {};
}

size_t matching_json_brace(const std::string& source, size_t start) {
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (size_t i = start; i < source.size(); ++i) {
        const char c = source[i];
        if (in_string) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') in_string = true;
        else if (c == '{') ++depth;
        else if (c == '}' && --depth == 0) return i;
    }
    return std::string::npos;
}

std::string shell_quote(const std::string& input) {
#ifdef _WIN32
    std::string out = "\"";
    for (char c : input) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    out += "\"";
    return out;
#else
    std::string out = "'";
    for (char c : input) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
#endif
}

bool read_all(const fs::path& path, std::string& data) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream stream;
    stream << in.rdbuf();
    data = stream.str();
    return in.good() || in.eof();
}

std::string make_temp_name(const std::string& stem) {
    const auto ticks = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return stem + "_" + std::to_string(static_cast<unsigned long long>(ticks));
}

// Compact SHA-256 implementation used to verify the exact release asset digest
// returned by GitHub's Releases API before any installer is launched.
class Sha256 {
public:
    void update(const unsigned char* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            buffer_[buffer_len_++] = data[i];
            if (buffer_len_ == 64) {
                transform(buffer_.data());
                bit_len_ += 512;
                buffer_len_ = 0;
            }
        }
    }

    std::string finish() {
        size_t i = buffer_len_;
        if (buffer_len_ < 56) {
            buffer_[i++] = 0x80;
            while (i < 56) buffer_[i++] = 0x00;
        } else {
            buffer_[i++] = 0x80;
            while (i < 64) buffer_[i++] = 0x00;
            transform(buffer_.data());
            buffer_.fill(0);
        }
        bit_len_ += static_cast<uint64_t>(buffer_len_) * 8;
        for (int j = 0; j < 8; ++j) buffer_[63 - j] = static_cast<unsigned char>((bit_len_ >> (j * 8)) & 0xff);
        transform(buffer_.data());

        std::ostringstream out;
        for (uint32_t word : state_) out << std::hex << std::setw(8) << std::setfill('0') << word;
        return out.str();
    }

private:
    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static constexpr std::array<uint32_t, 64> k_ = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    void transform(const unsigned char* block) {
        uint32_t m[64]{};
        for (int i = 0; i < 16; ++i) {
            const int j = i * 4;
            m[i] = (static_cast<uint32_t>(block[j]) << 24) |
                   (static_cast<uint32_t>(block[j + 1]) << 16) |
                   (static_cast<uint32_t>(block[j + 2]) << 8) |
                    static_cast<uint32_t>(block[j + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr(m[i - 15], 7) ^ rotr(m[i - 15], 18) ^ (m[i - 15] >> 3);
            const uint32_t s1 = rotr(m[i - 2], 17) ^ rotr(m[i - 2], 19) ^ (m[i - 2] >> 10);
            m[i] = m[i - 16] + s0 + m[i - 7] + s1;
        }
        uint32_t a=state_[0], b=state_[1], c=state_[2], d=state_[3];
        uint32_t e=state_[4], f=state_[5], g=state_[6], h=state_[7];
        for (int i = 0; i < 64; ++i) {
            const uint32_t s1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
            const uint32_t ch = (e & f) ^ (~e & g);
            const uint32_t t1 = h + s1 + ch + k_[i] + m[i];
            const uint32_t s0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t t2 = s0 + maj;
            h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        state_[0]+=a; state_[1]+=b; state_[2]+=c; state_[3]+=d;
        state_[4]+=e; state_[5]+=f; state_[6]+=g; state_[7]+=h;
    }

    std::array<uint32_t, 8> state_ = {
        0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
    };
    std::array<unsigned char, 64> buffer_{};
    size_t buffer_len_ = 0;
    uint64_t bit_len_ = 0;
};

std::string sha256_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    Sha256 hash;
    std::array<unsigned char, 64 * 1024> buffer{};
    while (in) {
        in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto got = in.gcount();
        if (got > 0) hash.update(buffer.data(), static_cast<size_t>(got));
    }
    if (!in.eof()) return {};
    return hash.finish();
}

std::string platform_asset(const std::string& tag) {
#ifdef _WIN32
#  if defined(_M_X64) || defined(__x86_64__)
    return "FRIDGE-Setup-" + tag + ".exe";
#  else
    return {};
#  endif
#elif defined(__APPLE__)
#  if defined(__x86_64__) || defined(__aarch64__) || defined(__arm64__)
    return "FRIDGE-macOS-" + tag + ".pkg";
#  else
    return {};
#  endif
#elif defined(__ANDROID__)
    return {};
#elif defined(__linux__)
#  if defined(__x86_64__) || defined(__amd64__)
    return "FRIDGE-Linux-" + tag + ".deb";
#  else
    return {};
#  endif
#else
    return {};
#endif
}

bool run_curl(const std::string& url, const fs::path& output) {
#ifdef _WIN32
    const std::string curl = "curl.exe";
#else
    const std::string curl = "curl";
#endif
    const std::string command = curl + " --fail --location --silent --show-error --retry 2 "
        "--user-agent \"FRIDGE-Updater/" FRIDGE_VERSION "\" " +
        shell_quote(url) + " --output " + shell_quote(output.string());
    return std::system(command.c_str()) == 0 && fs::exists(output) && fs::file_size(output) > 0;
}

bool install_asset(const fs::path& asset) {
#ifdef _WIN32
    // The installer uses restartreplace for the running CLI executable.
    const std::string command = "start \"\" /B " + shell_quote(asset.string()) +
        " /SILENT /SUPPRESSMSGBOXES /NORESTART";
#elif __APPLE__
    const std::string command = "sudo installer -pkg " + shell_quote(asset.string()) + " -target /";
#elif __linux__
    const std::string command = "sudo dpkg -i " + shell_quote(asset.string());
#else
    (void)asset;
    return false;
#endif
    return std::system(command.c_str()) == 0;
}
} // namespace

bool check_latest(ReleaseInfo& info, std::string& error) {
    info.current_version = FRIDGE_VERSION;
    const fs::path temp = fs::temp_directory_path() / make_temp_name("fridge_release") .append(".json");
    if (!run_curl(kReleaseApi, temp)) {
        error = "Could not contact the official FRIDGE Releases API. Check your internet connection.";
        return false;
    }

    std::string json;
    const bool loaded = read_all(temp, json);
    std::error_code ec;
    fs::remove(temp, ec);
    if (!loaded || json.empty()) {
        error = "The latest release response could not be read.";
        return false;
    }

    info.latest_version = json_string_field(json, "tag_name");
    if (info.latest_version.empty() || version_parts(info.latest_version).empty()) {
        error = "The latest FRIDGE release has an invalid or missing version tag.";
        return false;
    }

    const std::string required_name = platform_asset(info.latest_version);
    if (required_name.empty()) {
        error = "Automatic updates are not supported on this operating system.";
        return false;
    }

    const size_t assets_key = json.find("\"assets\"");
    const size_t array_start = assets_key == std::string::npos ? std::string::npos : json.find('[', assets_key);
    if (array_start == std::string::npos) {
        error = "The latest FRIDGE release has no asset list.";
        return false;
    }
    size_t array_end = array_start;
    int depth = 0;
    bool in_string = false, escaped = false;
    for (; array_end < json.size(); ++array_end) {
        const char c = json[array_end];
        if (in_string) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') in_string = true;
        else if (c == '[') ++depth;
        else if (c == ']' && --depth == 0) break;
    }
    if (array_end >= json.size()) {
        error = "The latest release asset list is malformed.";
        return false;
    }

    bool found = false;
    size_t pos = array_start + 1;
    while (pos < array_end) {
        const size_t object_start = json.find('{', pos);
        if (object_start == std::string::npos || object_start >= array_end) break;
        const size_t object_end = matching_json_brace(json, object_start);
        if (object_end == std::string::npos || object_end > array_end) break;
        const std::string object = json.substr(object_start, object_end - object_start + 1);
        if (json_string_field(object, "name") == required_name) {
            info.asset_name = required_name;
            info.download_url = json_string_field(object, "browser_download_url");
            info.sha256 = json_string_field(object, "digest");
            found = true;
            break;
        }
        pos = object_end + 1;
    }

    if (!found || info.download_url.empty()) {
        error = "The latest release does not publish the exact installer for this operating system: " + required_name;
        return false;
    }
    if (info.download_url.rfind(kDownloadPrefix, 0) != 0 ||
        info.download_url.find('"') != std::string::npos ||
        info.download_url.find('\\') != std::string::npos ||
        info.download_url.find("..") != std::string::npos) {
        error = "The release contains an unsafe or unexpected download URL.";
        return false;
    }
    if (info.sha256.rfind("sha256:", 0) != 0 || info.sha256.size() != 71) {
        error = "GitHub did not provide a valid SHA-256 digest for the installer. Update blocked.";
        return false;
    }
    return true;
}

bool update_required(const ReleaseInfo& info) {
    // A developer build newer than the latest published tag can still run.
    return compare_versions(info.latest_version, info.current_version) > 0;
}

int run_update_command() {
    ReleaseInfo info;
    std::string error;
    if (!check_latest(info, error)) {
        std::cerr << "[FRG-UPD-001] " << error << "\n";
        return 1;
    }

    std::cout << "FRIDGE installed: v" << trim_v(info.current_version) << "\n";
    std::cout << "Latest release:   " << info.latest_version << "\n";
    if (!update_required(info)) {
        std::cout << "FRIDGE is up to date.\n";
        return 0;
    }

    std::cout << "\nUpdate available: " << info.asset_name << "\n";
    std::cout << "Downloading and installing the verified official release now...\n";
    const fs::path temp_dir = fs::temp_directory_path() / make_temp_name("fridge_update");
    std::error_code ec;
    fs::create_directories(temp_dir, ec);
    if (ec) {
        std::cerr << "[FRG-UPD-002] Cannot create update staging directory: " << ec.message() << "\n";
        return 1;
    }
    const fs::path asset = temp_dir / info.asset_name;

    std::cout << "Downloading official installer...\n";
    if (!run_curl(info.download_url, asset)) {
        std::cerr << "[FRG-UPD-003] Download failed. The existing installation was not changed.\n";
        fs::remove_all(temp_dir, ec);
        return 1;
    }

    std::string expected = info.sha256.substr(7);
    std::transform(expected.begin(), expected.end(), expected.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const std::string actual = sha256_file(asset);
    if (actual.empty() || actual != expected) {
        std::cerr << "[FRG-UPD-004] SHA-256 verification failed. Installer will not be executed.\n";
        fs::remove_all(temp_dir, ec);
        return 1;
    }

    std::cout << "SHA-256 verified. Launching the platform installer...\n";
    if (!install_asset(asset)) {
        std::cerr << "[FRG-UPD-005] Could not launch or install the update. Existing FRIDGE files were not intentionally modified.\n";
        return 1;
    }

#ifdef _WIN32
    std::cout << "The Windows installer has been launched. Complete its prompts, then reopen FRIDGE.\n";
#elif __APPLE__
    std::cout << "The macOS installer has completed. Reopen FRIDGE.\n";
#else
    std::cout << "The Linux package installer has completed. Reopen FRIDGE.\n";
#endif
    return 0;
}
} // namespace fridge_update
