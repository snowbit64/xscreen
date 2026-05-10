#include "ProfileManager.h"

namespace xscreen {

void ProfileManager::registerProfile(const Profile& profile) {
    profiles_[profile.name] = profile;
}

void ProfileManager::clear() {
    profiles_.clear();
}

bool ProfileManager::hasProfile(const std::string& name) const {
    return profiles_.find(name) != profiles_.end();
}

const Profile* ProfileManager::getProfile(const std::string& name) const {
    auto it = profiles_.find(name);
    return (it != profiles_.end()) ? &it->second : nullptr;
}

std::string ProfileManager::resolveProperty(const std::string& profileName,
                                            const std::string& propName,
                                            const std::string& fallback) const {
    auto all = resolveAllProperties(profileName);
    auto it = all.find(propName);
    return (it != all.end()) ? it->second : fallback;
}

std::unordered_map<std::string, std::string> ProfileManager::resolveAllProperties(
    const std::string& profileName) const {
    std::unordered_map<std::string, std::string> result;
    collectProperties(profileName, result);
    return result;
}

void ProfileManager::collectProperties(const std::string& profileName,
                                       std::unordered_map<std::string, std::string>& out,
                                       int depth) const {
    if (depth > 16) return;

    auto it = profiles_.find(profileName);
    if (it == profiles_.end()) return;

    const auto& profile = it->second;

    if (!profile.extends.empty()) {
        collectProperties(profile.extends, out, depth + 1);
    }

    for (const auto& prop : profile.properties) {
        out[prop.name] = prop.value;
    }
}

} // namespace xscreen
