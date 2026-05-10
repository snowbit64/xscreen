#pragma once

#include "Animation.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace xscreen {

class AnimationManager {
public:
    void add(const std::string& name, std::shared_ptr<Animation> anim);
    void remove(const std::string& name);
    void clear();

    void play(const std::string& name);
    void stop(const std::string& name);
    void stopAll();

    void update(float dt);

    std::shared_ptr<Animation> get(const std::string& name) const;

private:
    std::unordered_map<std::string, std::shared_ptr<Animation>> animations_;
};

} // namespace xscreen
