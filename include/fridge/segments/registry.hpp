#pragma once
#include <string>
#include <vector>

namespace fridge {

struct SegmentInfo {
    std::string name;
    std::string version;
    std::string description;
};

class SegmentRegistry {
public:
    static std::vector<SegmentInfo> listSegments();
};

} // namespace fridge
