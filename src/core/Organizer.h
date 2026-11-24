#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace organizer {

struct PlannedMove {
    std::filesystem::path source;
    std::filesystem::path destination;
    std::string category;
};

struct CompletedMove {
    std::filesystem::path source;
    std::filesystem::path destination;
};

struct FailedMove {
    std::filesystem::path source;
    std::string reason;
};

struct OrganizeResult {
    std::vector<CompletedMove> completed;
    std::vector<FailedMove> failed;
};

class Organizer final {
public:
    static std::vector<PlannedMove> buildPlan(const std::filesystem::path& directory);
    static OrganizeResult execute(const std::vector<PlannedMove>& plan);
};

} // namespace organizer
