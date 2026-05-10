#include "core/UIElement.h"
#include "core/ProfileManager.h"
#include "core/XMLParser.h"
#include "core/WidgetFactory.h"
#include "core/ScreenManager.h"
#include "layout/LayoutEngine.h"
#include "scripting/LuaBridge.h"
#include "animation/AnimationManager.h"
#include "renderer/GLESRenderer.h"
#include "widgets/Button.h"
#include "widgets/TextField.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <chrono>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

using namespace xscreen;

static ScreenManager* g_screenManager = nullptr;

static void glfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "[GLFW] Error %d: %s\n", error, description);
}

static void glfwCharCallback(GLFWwindow* /*window*/, unsigned int codepoint) {
    if (!g_screenManager || codepoint >= 128) return;
    InputEvent keyEvent;
    keyEvent.action = InputAction::Press;
    keyEvent.key = static_cast<int>(codepoint);
    g_screenManager->handleInput(keyEvent);
}

static void glfwKeyCallback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/) {
    if (!g_screenManager || action != GLFW_PRESS) return;
    int mappedKey = 0;
    if (key == GLFW_KEY_BACKSPACE) mappedKey = 259;
    else if (key == GLFW_KEY_DELETE) mappedKey = 261;
    else if (key == GLFW_KEY_LEFT) mappedKey = 263;
    else if (key == GLFW_KEY_RIGHT) mappedKey = 262;
    if (mappedKey) {
        InputEvent keyEvent;
        keyEvent.action = InputAction::Press;
        keyEvent.key = mappedKey;
        g_screenManager->handleInput(keyEvent);
    }
}

static void glfwScrollCallback(GLFWwindow* w, double /*xoffset*/, double yoffset) {
    if (!g_screenManager) return;
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    InputEvent scrollEvent;
    scrollEvent.action = InputAction::Scroll;
    scrollEvent.x = static_cast<float>(mx);
    scrollEvent.y = static_cast<float>(my);
    scrollEvent.scrollDelta = static_cast<float>(yoffset);
    g_screenManager->handleInput(scrollEvent);
}

int main(int argc, char* argv[]) {
    std::string basePath = "ui/";
    if (argc > 1) basePath = argv[1];

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(1280, 720,
        "XScreen - South American Farming UI", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetCharCallback(window, glfwCharCallback);
    glfwSetKeyCallback(window, glfwKeyCallback);
    glfwSetScrollCallback(window, glfwScrollCallback);

    int version = gladLoadGL(glfwGetProcAddress);
    if (!version) {
        std::fprintf(stderr, "Failed to initialize OpenGL loader (glad)\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    std::printf("OpenGL %d.%d loaded\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    GLESRenderer renderer;
    if (!renderer.init(fbW, fbH, "XScreen")) {
        std::fprintf(stderr, "Failed to initialize renderer\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::string fontPath = basePath + "fonts/Roboto-Regular.ttf";
    FontId defaultFont = renderer.loadFont(fontPath, 48);
    if (defaultFont != INVALID_FONT) {
        renderer.setDefaultFont(defaultFont);
        std::printf("Default font loaded: %s\n", fontPath.c_str());
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
    g_screenManager = &screenManager;

    lua_State* L = lua.getState();

    lua_getglobal(L, "_xscreen_callbacks");
    lua_pushcfunction(L, [](lua_State* L) -> int {
        if (g_screenManager) g_screenManager->show(luaL_checkstring(L, 1));
        return 0;
    });
    lua_setfield(L, -2, "show");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        if (g_screenManager) g_screenManager->close(luaL_checkstring(L, 1));
        return 0;
    });
    lua_setfield(L, -2, "close");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        if (g_screenManager) g_screenManager->push(luaL_checkstring(L, 1));
        return 0;
    });
    lua_setfield(L, -2, "push");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        if (g_screenManager) g_screenManager->pop();
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
    std::string switchExXml = basePath + "screens/ScreenSwitchExample.xml";

    screenManager.loadScreen(mainMenuXml);
    screenManager.loadScreen(settingsXml);
    screenManager.loadScreen(testXml);
    screenManager.loadScreen(switchExXml);

    screenManager.show("MainMenu");

    screenManager.registerCallbackHandler(
        [&](const std::string& screen, const std::string& callback) {
            std::printf("[App] Callback: %s::%s\n", screen.c_str(), callback.c_str());
        });

    auto lastTime = std::chrono::high_resolution_clock::now();
    double lastMouseX = 0, lastMouseY = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        if (dt > 0.1f) dt = 0.016f;
        renderer.setDeltaTime(dt);

        glfwGetFramebufferSize(window, &fbW, &fbH);
        renderer.setScreenSize(fbW, fbH);

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (mouseX != lastMouseX || mouseY != lastMouseY) {
            InputEvent moveEvent;
            moveEvent.action = InputAction::Move;
            moveEvent.x = static_cast<float>(mouseX);
            moveEvent.y = static_cast<float>(mouseY);
            screenManager.handleInput(moveEvent);
            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }

        static bool wasLeftPressed = false;
        bool leftPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        if (leftPressed && !wasLeftPressed) {
            InputEvent pressEvent;
            pressEvent.action = InputAction::Press;
            pressEvent.x = static_cast<float>(mouseX);
            pressEvent.y = static_cast<float>(mouseY);
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

        if (!leftPressed && wasLeftPressed) {
            InputEvent releaseEvent;
            releaseEvent.action = InputAction::Release;
            releaseEvent.x = static_cast<float>(mouseX);
            releaseEvent.y = static_cast<float>(mouseY);
            releaseEvent.button = 0;
            screenManager.handleInput(releaseEvent);
        }
        wasLeftPressed = leftPressed;

        screenManager.update(dt);
        animations.update(dt);

        renderer.beginFrame();
        screenManager.render(renderer,
                             static_cast<float>(fbW),
                             static_cast<float>(fbH));
        renderer.endFrame();
        glfwSwapBuffers(window);
    }

    renderer.shutdown();
    lua.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
