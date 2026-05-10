#pragma once

#include "UIElement.h"
#include "XMLParser.h"
#include "../scripting/LuaBridge.h"
#include "../layout/LayoutEngine.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace xscreen {

class Renderer;

struct Screen {
    std::string name;
    std::string scriptPath;
    std::string scriptTable;
    std::shared_ptr<UIElement> root;
    bool loaded = false;
};

class ScreenManager {
public:
    ScreenManager(LuaBridge& lua, LayoutEngine& layout, ProfileManager& profiles,
                  WidgetFactory& factory);

    bool loadScreen(const std::string& xmlPath);
    bool registerScreen(const std::string& name, ScreenDef def);

    void show(const std::string& name);
    void close(const std::string& name);
    void push(const std::string& name);
    void pop();

    void update(float dt);
    void render(Renderer& renderer, float screenW, float screenH);
    void handleInput(const InputEvent& event);

    Screen* getActiveScreen();
    Screen* getScreen(const std::string& name);
    const std::string& getActiveScreenName() const;

    void registerCallbackHandler(std::function<void(const std::string&, const std::string&)> handler);

    bool invokeCallback(const std::string& callbackName);

private:
    void activateScreen(const std::string& name);
    void deactivateScreen(const std::string& name);
    std::string inferTableName(const std::string& screenName) const;

    LuaBridge& lua_;
    LayoutEngine& layout_;
    ProfileManager& profiles_;
    WidgetFactory& factory_;

    std::unordered_map<std::string, Screen> screens_;
    std::vector<std::string> stack_;
    std::string activeScreen_;

    std::function<void(const std::string&, const std::string&)> callbackHandler_;
};

} // namespace xscreen
