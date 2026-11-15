#pragma once
#include <string>

namespace fridge_update {
struct ReleaseInfo {
    std::string current_version;
    std::string latest_version;
    std::string asset_name;
    std::string download_url;
    std::string sha256;
};

bool check_latest(ReleaseInfo& info, std::string& error);
bool update_required(const ReleaseInfo& info);
int run_update_command();
}
