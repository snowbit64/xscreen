#pragma once

#include "Types.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace xscreen {

struct ProfileProperty {
    std::string name;
    std::string value;
};

struct Profile {
    std::string name;
    std::string extends;
    std::vector<ProfileProperty> properties;
};

class ProfileManager {
public:
    void registerProfile(const Profile& profile);
    void clear();

    bool hasProfile(const std::string& name) const;
    const Profile* getProfile(const std::string& name) const;

    std::string resolveProperty(const std::string& profileName,
                                const std::string& propName,
                                const std::string& fallback = "") const;

    std::unordered_map<std::string, std::string> resolveAllProperties(
        const std::string& profileName) const;

private:
    void collectProperties(const std::string& profileName,
                           std::unordered_map<std::string, std::string>& out,
                           int depth = 0) const;

    std::unordered_map<std::string, Profile> profiles_;
};

} // namespace xscreen
