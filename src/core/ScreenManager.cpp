#include "ScreenManager.h"
#include "../renderer/Renderer.h"
#include <cstdio>
#include <algorithm>

namespace xscreen {

ScreenManager::ScreenManager(LuaBridge& lua, LayoutEngine& layout,
                             ProfileManager& profiles, WidgetFactory& factory)
    : lua_(lua), layout_(layout), profiles_(profiles), factory_(factory) {}

bool ScreenManager::loadScreen(const std::string& xmlPath) {
    XMLParser parser(profiles_, factory_);
    ScreenDef def = parser.parseFile(xmlPath);
    if (!def.root) {
        std::fprintf(stderr, "[ScreenManager] Failed to load: %s\n", xmlPath.c_str());
        return false;
    }
    return registerScreen(def.name, std::move(def));
}

bool ScreenManager::registerScreen(const std::string& name, ScreenDef def) {
    Screen screen;
    screen.name = name;
    screen.scriptPath = def.script;
    screen.root = std::move(def.root);
    screen.scriptTable = inferTableName(name);
    screen.loaded = true;

    if (!screen.scriptPath.empty()) {
        if (!lua_.loadScript(screen.scriptPath)) {
            std::fprintf(stderr, "[ScreenManager] Warning: Failed to load script: %s\n",
                         screen.scriptPath.c_str());
        }
    }

    screens_[name] = std::move(screen);
    std::printf("[ScreenManager] Registered screen: %s\n", name.c_str());
    return true;
}

void ScreenManager::show(const std::string& name) {
    if (screens_.find(name) == screens_.end()) {
        std::fprintf(stderr, "[ScreenManager] Screen not found: %s\n", name.c_str());
        return;
    }

    if (!activeScreen_.empty()) {
        deactivateScreen(activeScreen_);
    }

    stack_.clear();
    activeScreen_ = name;
    activateScreen(name);
}

void ScreenManager::close(const std::string& name) {
    if (activeScreen_ == name) {
        deactivateScreen(name);
        activeScreen_.clear();

        if (!stack_.empty()) {
            activeScreen_ = stack_.back();
            stack_.pop_back();
            activateScreen(activeScreen_);
        }
    }
}

void ScreenManager::push(const std::string& name) {
    if (screens_.find(name) == screens_.end()) {
        std::fprintf(stderr, "[ScreenManager] Screen not found: %s\n", name.c_str());
        return;
    }

    if (!activeScreen_.empty()) {
        stack_.push_back(activeScreen_);
    }

    activeScreen_ = name;
    activateScreen(name);
}

void ScreenManager::pop() {
    if (!activeScreen_.empty()) {
        deactivateScreen(activeScreen_);
    }

    if (!stack_.empty()) {
        activeScreen_ = stack_.back();
        stack_.pop_back();
        activateScreen(activeScreen_);
    } else {
        activeScreen_.clear();
    }
}

void ScreenManager::update(float dt) {
    auto it = screens_.find(activeScreen_);
    if (it != screens_.end() && it->second.root) {
        it->second.root->update(dt);
    }
}

void ScreenManager::render(Renderer& renderer, float screenW, float screenH) {
    auto it = screens_.find(activeScreen_);
    if (it != screens_.end() && it->second.root) {
        layout_.compute(*it->second.root, screenW, screenH);
        it->second.root->render(renderer);
    }
}

void ScreenManager::handleInput(const InputEvent& event) {
    auto it = screens_.find(activeScreen_);
    if (it != screens_.end() && it->second.root) {
        it->second.root->handleInput(event);
    }
}

Screen* ScreenManager::getActiveScreen() {
    auto it = screens_.find(activeScreen_);
    return (it != screens_.end()) ? &it->second : nullptr;
}

Screen* ScreenManager::getScreen(const std::string& name) {
    auto it = screens_.find(name);
    return (it != screens_.end()) ? &it->second : nullptr;
}

const std::string& ScreenManager::getActiveScreenName() const {
    return activeScreen_;
}

void ScreenManager::registerCallbackHandler(
    std::function<void(const std::string&, const std::string&)> handler) {
    callbackHandler_ = std::move(handler);
}

bool ScreenManager::invokeCallback(const std::string& callbackName) {
    auto* screen = getActiveScreen();
    if (!screen) return false;

    bool result = lua_.callFunction(screen->scriptTable, callbackName);

    if (callbackHandler_) {
        callbackHandler_(activeScreen_, callbackName);
    }

    return result;
}

void ScreenManager::activateScreen(const std::string& name) {
    auto it = screens_.find(name);
    if (it == screens_.end()) return;

    auto& screen = it->second;
    lua_.callFunction(screen.scriptTable, "onOpen");
    std::printf("[ScreenManager] Activated: %s\n", name.c_str());
}

void ScreenManager::deactivateScreen(const std::string& name) {
    auto it = screens_.find(name);
    if (it == screens_.end()) return;

    auto& screen = it->second;
    lua_.callFunction(screen.scriptTable, "onClose");
    std::printf("[ScreenManager] Deactivated: %s\n", name.c_str());
}

std::string ScreenManager::inferTableName(const std::string& screenName) const {
    return screenName;
}

} // namespace xscreen
