#include "core/UIElement.h"
#include "core/ProfileManager.h"
#include "core/XMLParser.h"
#include "core/WidgetFactory.h"
#include "core/ScreenManager.h"
#include "layout/LayoutEngine.h"
#include "scripting/LuaBridge.h"
#include "animation/AnimationManager.h"
#include "renderer/RaylibRenderer.h"
#include "widgets/Button.h"
#include "raylib.h"

#include <cstdio>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

using namespace xscreen;

int main(int argc, char* argv[]) {
    std::string basePath = "ui/";
    if (argc > 1) basePath = argv[1];

    RaylibRenderer renderer;
    if (!renderer.init(1280, 720, "XScreen - South American Farming UI")) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }

    ProfileManager profiles;
    WidgetFactory factory;
    factory.registerDefaults();

    LuaBridge lua;
    if (!lua.init()) {
        std::fprintf(stderr, "Failed to initialize Lua\n");
        return 1;
    }

    LayoutEngine layout;
    ScreenManager screenManager(lua, layout, profiles, factory);

    lua_State* L = lua.getState();

    lua_getglobal(L, "_xscreen_callbacks");
    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getglobal(L, "_screenManager");
        auto* sm = static_cast<ScreenManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (sm) {
            const char* name = luaL_checkstring(L, 1);
            sm->show(name);
        }
        return 0;
    });
    lua_setfield(L, -2, "show");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getglobal(L, "_screenManager");
        auto* sm = static_cast<ScreenManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (sm) {
            const char* name = luaL_checkstring(L, 1);
            sm->close(name);
        }
        return 0;
    });
    lua_setfield(L, -2, "close");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getglobal(L, "_screenManager");
        auto* sm = static_cast<ScreenManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (sm) {
            const char* name = luaL_checkstring(L, 1);
            sm->push(name);
        }
        return 0;
    });
    lua_setfield(L, -2, "push");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getglobal(L, "_screenManager");
        auto* sm = static_cast<ScreenManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (sm) {
            sm->pop();
        }
        return 0;
    });
    lua_setfield(L, -2, "pop");
    lua_pop(L, 1);

    lua_pushlightuserdata(L, &screenManager);
    lua_setglobal(L, "_screenManager");

    AnimationManager animations;

    std::string mainMenuXml = basePath + "screens/MainMenu.xml";
    std::string settingsXml = basePath + "screens/SettingsScreen.xml";
    std::string testXml = basePath + "screens/TestScreen.xml";

    screenManager.loadScreen(mainMenuXml);
    screenManager.loadScreen(settingsXml);
    screenManager.loadScreen(testXml);

    screenManager.show("MainMenu");

    screenManager.registerCallbackHandler(
        [&](const std::string& screen, const std::string& callback) {
            std::printf("[App] Callback: %s::%s\n", screen.c_str(), callback.c_str());
        });

    while (!renderer.shouldClose()) {
        float dt = renderer.getDeltaTime();

        Vector2 mousePos = GetMousePosition();
        InputEvent moveEvent;
        moveEvent.action = InputAction::Move;
        moveEvent.x = mousePos.x;
        moveEvent.y = mousePos.y;
        screenManager.handleInput(moveEvent);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            InputEvent pressEvent;
            pressEvent.action = InputAction::Press;
            pressEvent.x = mousePos.x;
            pressEvent.y = mousePos.y;
            pressEvent.button = 0;
            screenManager.handleInput(pressEvent);

            auto* screen = screenManager.getActiveScreen();
            if (screen && screen->root) {
                std::function<void(const std::shared_ptr<UIElement>&)> checkCallbacks;
                checkCallbacks = [&](const std::shared_ptr<UIElement>& el) {
                    auto* btn = dynamic_cast<Button*>(el.get());
                    if (btn && btn->isPressed()) {
                        std::string cb = btn->getCallback("onClick");
                        if (!cb.empty()) {
                            screenManager.invokeCallback(cb);
                        }
                    }
                    for (auto& child : el->getChildren()) {
                        checkCallbacks(child);
                    }
                };
                checkCallbacks(screen->root);
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            InputEvent releaseEvent;
            releaseEvent.action = InputAction::Release;
            releaseEvent.x = mousePos.x;
            releaseEvent.y = mousePos.y;
            releaseEvent.button = 0;
            screenManager.handleInput(releaseEvent);
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            InputEvent scrollEvent;
            scrollEvent.action = InputAction::Scroll;
            scrollEvent.x = mousePos.x;
            scrollEvent.y = mousePos.y;
            scrollEvent.scrollDelta = wheel;
            screenManager.handleInput(scrollEvent);
        }

        int key = GetCharPressed();
        while (key > 0) {
            InputEvent keyEvent;
            keyEvent.action = InputAction::Press;
            keyEvent.key = key;
            screenManager.handleInput(keyEvent);
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            InputEvent keyEvent;
            keyEvent.action = InputAction::Press;
            keyEvent.key = 259;
            screenManager.handleInput(keyEvent);
        }

        screenManager.update(dt);
        animations.update(dt);

        renderer.beginFrame();
        screenManager.render(renderer,
                             static_cast<float>(renderer.getScreenWidth()),
                             static_cast<float>(renderer.getScreenHeight()));
        renderer.endFrame();
    }

    renderer.shutdown();
    lua.shutdown();

    return 0;
}
