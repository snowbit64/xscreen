#include "AnimationManager.h"

namespace xscreen {

void AnimationManager::add(const std::string& name, std::shared_ptr<Animation> anim) {
    animations_[name] = std::move(anim);
}

void AnimationManager::remove(const std::string& name) {
    animations_.erase(name);
}

void AnimationManager::clear() {
    animations_.clear();
}

void AnimationManager::play(const std::string& name) {
    auto it = animations_.find(name);
    if (it != animations_.end()) {
        it->second->start();
    }
}

void AnimationManager::stop(const std::string& name) {
    auto it = animations_.find(name);
    if (it != animations_.end()) {
        it->second->stop();
    }
}

void AnimationManager::stopAll() {
    for (auto& [name, anim] : animations_) {
        anim->stop();
    }
}

void AnimationManager::update(float dt) {
    for (auto& [name, anim] : animations_) {
        anim->update(dt);
    }
}

std::shared_ptr<Animation> AnimationManager::get(const std::string& name) const {
    auto it = animations_.find(name);
    return (it != animations_.end()) ? it->second : nullptr;
}

} // namespace xscreen
