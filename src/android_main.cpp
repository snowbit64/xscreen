#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <android/input.h>
#include <android/looper.h>
#include <android/native_window.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <pthread.h>
#include <unistd.h>

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
#include <jni.h>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#define TAG "xscreen"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

using namespace xscreen;

static void showSoftKeyboard(ANativeActivity* activity, bool show) {
    JavaVM* vm = activity->vm;
    JNIEnv* env = nullptr;
    vm->AttachCurrentThread(&env, nullptr);

    jobject ctx = activity->clazz;
    jclass ctxClass = env->GetObjectClass(ctx);
    jmethodID getSystemService = env->GetMethodID(ctxClass, "getSystemService",
        "(Ljava/lang/String;)Ljava/lang/Object;");

    jstring serviceName = env->NewStringUTF("input_method");
    jobject imm = env->CallObjectMethod(ctx, getSystemService, serviceName);
    env->DeleteLocalRef(serviceName);

    if (imm) {
        jclass immClass = env->GetObjectClass(imm);
        if (show) {
            jmethodID getWindow = env->GetMethodID(ctxClass, "getWindow",
                "()Landroid/view/Window;");
            jobject window = env->CallObjectMethod(ctx, getWindow);
            jclass windowClass = env->GetObjectClass(window);
            jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView",
                "()Landroid/view/View;");
            jobject decorView = env->CallObjectMethod(window, getDecorView);

            jmethodID showSoft = env->GetMethodID(immClass, "showSoftInput",
                "(Landroid/view/View;I)Z");
            env->CallBooleanMethod(imm, showSoft, decorView, 0);

            env->DeleteLocalRef(decorView);
            env->DeleteLocalRef(windowClass);
            env->DeleteLocalRef(window);
        } else {
            jmethodID getWindow = env->GetMethodID(ctxClass, "getWindow",
                "()Landroid/view/Window;");
            jobject window = env->CallObjectMethod(ctx, getWindow);
            jclass windowClass = env->GetObjectClass(window);
            jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView",
                "()Landroid/view/View;");
            jobject decorView = env->CallObjectMethod(window, getDecorView);
            jclass viewClass = env->GetObjectClass(decorView);
            jmethodID getWindowToken = env->GetMethodID(viewClass, "getWindowToken",
                "()Landroid/os/IBinder;");
            jobject token = env->CallObjectMethod(decorView, getWindowToken);

            jmethodID hideSoft = env->GetMethodID(immClass, "hideSoftInputFromWindow",
                "(Landroid/os/IBinder;I)Z");
            env->CallBooleanMethod(imm, hideSoft, token, 0);

            env->DeleteLocalRef(token);
            env->DeleteLocalRef(viewClass);
            env->DeleteLocalRef(decorView);
            env->DeleteLocalRef(windowClass);
            env->DeleteLocalRef(window);
        }
        env->DeleteLocalRef(immClass);
        env->DeleteLocalRef(imm);
    }
    env->DeleteLocalRef(ctxClass);
    vm->DetachCurrentThread();
}

static void installTextFieldFocusCallbacks(AppState* app, const std::shared_ptr<UIElement>& element) {
    auto* tf = dynamic_cast<TextField*>(element.get());
    if (tf) {
        tf->setFocusChangeCallback([app](bool focused) {
            LOGI("TextField focus changed: %s", focused ? "true" : "false");
            showSoftKeyboard(app->activity, focused);
        });
    }
    for (auto& child : element->getChildren()) {
        installTextFieldFocusCallbacks(app, child);
    }
}

static std::string readAssetString(AAssetManager* mgr, const char* path) {
    AAsset* asset = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open asset: %s", path);
        return "";
    }
    off_t len = AAsset_getLength(asset);
    std::string content(len, '\0');
    AAsset_read(asset, &content[0], len);
    AAsset_close(asset);
    return content;
}

static std::vector<unsigned char> readAssetBinary(AAssetManager* mgr, const char* path) {
    AAsset* asset = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open binary asset: %s", path);
        return {};
    }
    off_t len = AAsset_getLength(asset);
    std::vector<unsigned char> data(len);
    AAsset_read(asset, data.data(), len);
    AAsset_close(asset);
    return data;
}

struct AppState {
    ANativeActivity* activity = nullptr;
    ANativeWindow* window = nullptr;
    AInputQueue* inputQueue = nullptr;

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;

    bool running = false;
    bool windowReady = false;
    bool destroyed = false;
    bool hasFocus = false;

    int width = 0;
    int height = 0;

    std::unique_ptr<GLESRenderer> renderer;
    std::unique_ptr<ProfileManager> profiles;
    std::unique_ptr<WidgetFactory> factory;
    std::unique_ptr<LuaBridge> lua;
    std::unique_ptr<LayoutEngine> layout;
    std::unique_ptr<ScreenManager> screenManager;
    std::unique_ptr<AnimationManager> animations;

    std::vector<unsigned char> fontData;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    int msgPipe[2] = {-1, -1};
};

enum AppCmd {
    CMD_WINDOW_CREATED = 1,
    CMD_WINDOW_DESTROYED,
    CMD_INPUT_QUEUE_CREATED,
    CMD_INPUT_QUEUE_DESTROYED,
    CMD_RESUME,
    CMD_PAUSE,
    CMD_DESTROY,
};

static bool initEGL(AppState* app) {
    app->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(app->display, nullptr, nullptr);

    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(app->display, attribs, &config, 1, &numConfigs);
    if (numConfigs == 0) {
        LOGE("eglChooseConfig failed");
        return false;
    }

    EGLint format;
    eglGetConfigAttrib(app->display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);

    app->surface = eglCreateWindowSurface(app->display, config, app->window, nullptr);
    if (app->surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return false;
    }

    const EGLint ctxAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    app->context = eglCreateContext(app->display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (app->context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return false;
    }

    if (!eglMakeCurrent(app->display, app->surface, app->surface, app->context)) {
        LOGE("eglMakeCurrent failed");
        return false;
    }

    EGLint w, h;
    eglQuerySurface(app->display, app->surface, EGL_WIDTH, &w);
    eglQuerySurface(app->display, app->surface, EGL_HEIGHT, &h);
    app->width = w;
    app->height = h;

    LOGI("EGL initialized: %dx%d", w, h);
    return true;
}

static void termEGL(AppState* app) {
    if (app->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(app->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (app->context != EGL_NO_CONTEXT) eglDestroyContext(app->display, app->context);
        if (app->surface != EGL_NO_SURFACE) eglDestroySurface(app->display, app->surface);
        eglTerminate(app->display);
    }
    app->display = EGL_NO_DISPLAY;
    app->context = EGL_NO_CONTEXT;
    app->surface = EGL_NO_SURFACE;
}

static void setupLuaCallbacks(AppState* app) {
    lua_State* L = app->lua->getState();

    lua_getglobal(L, "_xscreen_callbacks");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getglobal(L, "_screenManager");
        auto* sm = static_cast<ScreenManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (sm) sm->show(luaL_checkstring(L, 1));
        return 0;
    });
    lua_setfield(L, -2, "show");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getglobal(L, "_screenManager");
        auto* sm = static_cast<ScreenManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (sm) sm->close(luaL_checkstring(L, 1));
        return 0;
    });
    lua_setfield(L, -2, "close");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getglobal(L, "_screenManager");
        auto* sm = static_cast<ScreenManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (sm) sm->push(luaL_checkstring(L, 1));
        return 0;
    });
    lua_setfield(L, -2, "push");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getglobal(L, "_screenManager");
        auto* sm = static_cast<ScreenManager*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (sm) sm->pop();
        return 0;
    });
    lua_setfield(L, -2, "pop");

    lua_pop(L, 1);

    lua_pushlightuserdata(L, app->screenManager.get());
    lua_setglobal(L, "_screenManager");
}

static bool loadScreenFromAsset(AppState* app, AAssetManager* mgr, const char* xmlAsset) {
    std::string xmlContent = readAssetString(mgr, xmlAsset);
    if (xmlContent.empty()) {
        LOGE("Failed to read XML asset: %s", xmlAsset);
        return false;
    }

    XMLParser parser(*app->profiles, *app->factory);
    ScreenDef def = parser.parseString(xmlContent);
    if (!def.root) {
        LOGE("Failed to parse XML: %s", xmlAsset);
        return false;
    }

    if (!def.script.empty()) {
        std::string scriptContent = readAssetString(mgr, def.script.c_str());
        if (!scriptContent.empty()) {
            if (!app->lua->loadString(scriptContent)) {
                LOGE("Failed to load Lua script for: %s", xmlAsset);
            } else {
                LOGI("Loaded Lua script for: %s", def.name.c_str());
            }
        }
    }

    def.script = "";
    std::string screenName = def.name;
    app->screenManager->registerScreen(def.name, std::move(def));
    LOGI("Loaded screen: %s", screenName.c_str());
    return true;
}

static bool initApp(AppState* app) {
    app->renderer = std::make_unique<GLESRenderer>();
    if (!app->renderer->init(app->width, app->height, "XScreen")) {
        LOGE("Failed to init renderer");
        return false;
    }

    app->profiles = std::make_unique<ProfileManager>();
    app->factory = std::make_unique<WidgetFactory>();
    app->factory->registerDefaults();

    app->lua = std::make_unique<LuaBridge>();
    if (!app->lua->init()) {
        LOGE("Failed to init Lua");
        return false;
    }

    app->layout = std::make_unique<LayoutEngine>();
    app->screenManager = std::make_unique<ScreenManager>(
        *app->lua, *app->layout, *app->profiles, *app->factory);
    app->animations = std::make_unique<AnimationManager>();

    setupLuaCallbacks(app);

    AAssetManager* mgr = app->activity->assetManager;

    app->fontData = readAssetBinary(mgr, "fonts/Roboto-Regular.ttf");
    if (!app->fontData.empty()) {
        FontId fid = app->renderer->loadFontFromMemory(app->fontData.data(),
                                                        static_cast<int>(app->fontData.size()), 48);
        if (fid != INVALID_FONT) {
            app->renderer->setDefaultFont(fid);
            LOGI("Default font loaded: Roboto-Regular.ttf");
        }
    } else {
        LOGE("Failed to load default font from assets");
    }

    loadScreenFromAsset(app, mgr, "screens/MainMenu.xml");
    loadScreenFromAsset(app, mgr, "screens/SettingsScreen.xml");
    loadScreenFromAsset(app, mgr, "screens/TestScreen.xml");
    loadScreenFromAsset(app, mgr, "screens/ScreenSwitchExample.xml");

    const char* screenNames[] = {"MainMenu", "SettingsScreen", "TestScreen", "ScreenSwitchExample"};
    for (const char* name : screenNames) {
        auto* s = app->screenManager->getScreen(name);
        if (s && s->root) installTextFieldFocusCallbacks(app, s->root);
    }

    app->screenManager->show("MainMenu");

    app->screenManager->registerCallbackHandler(
        [](const std::string& screen, const std::string& callback) {
            LOGI("Callback: %s::%s", screen.c_str(), callback.c_str());
        });

    LOGI("App initialized successfully");
    return true;
}

static void handleTouchInput(AppState* app, AInputEvent* event) {
    int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);

    switch (action) {
        case AMOTION_EVENT_ACTION_DOWN: {
            InputEvent moveEv;
            moveEv.action = InputAction::Move;
            moveEv.x = x;
            moveEv.y = y;
            app->screenManager->handleInput(moveEv);

            InputEvent pressEv;
            pressEv.action = InputAction::Press;
            pressEv.x = x;
            pressEv.y = y;
            pressEv.button = 0;
            app->screenManager->handleInput(pressEv);

            auto* screen = app->screenManager->getActiveScreen();
            if (screen && screen->root) {
                std::function<void(const std::shared_ptr<UIElement>&)> checkCallbacks;
                checkCallbacks = [&](const std::shared_ptr<UIElement>& el) {
                    auto* btn = dynamic_cast<Button*>(el.get());
                    if (btn && btn->isPressed()) {
                        std::string cb = btn->getCallback("onClick");
                        if (!cb.empty()) {
                            app->screenManager->invokeCallback(cb);
                        }
                    }
                    for (auto& child : el->getChildren()) {
                        checkCallbacks(child);
                    }
                };
                checkCallbacks(screen->root);
            }
            break;
        }
        case AMOTION_EVENT_ACTION_MOVE: {
            InputEvent moveEv;
            moveEv.action = InputAction::Move;
            moveEv.x = x;
            moveEv.y = y;
            app->screenManager->handleInput(moveEv);
            break;
        }
        case AMOTION_EVENT_ACTION_UP: {
            InputEvent releaseEv;
            releaseEv.action = InputAction::Release;
            releaseEv.x = x;
            releaseEv.y = y;
            releaseEv.button = 0;
            app->screenManager->handleInput(releaseEv);
            break;
        }
    }
}

static void sendCmd(AppState* app, int8_t cmd) {
    write(app->msgPipe[1], &cmd, sizeof(cmd));
}

static void processCmd(AppState* app) {
    int8_t cmd;
    if (read(app->msgPipe[0], &cmd, sizeof(cmd)) != sizeof(cmd)) return;

    switch (cmd) {
        case CMD_WINDOW_CREATED:
            if (app->window) {
                if (initEGL(app)) {
                    app->windowReady = true;
                    if (!app->renderer) {
                        initApp(app);
                    } else {
                        app->renderer->setScreenSize(app->width, app->height);
                        app->renderer->init(app->width, app->height, "XScreen");
                        if (!app->fontData.empty()) {
                            FontId fid = app->renderer->loadFontFromMemory(
                                app->fontData.data(),
                                static_cast<int>(app->fontData.size()), 48);
                            if (fid != INVALID_FONT) {
                                app->renderer->setDefaultFont(fid);
                                LOGI("Font reloaded after window recreate");
                            }
                        }
                    }
                }
            }
            break;

        case CMD_WINDOW_DESTROYED:
            app->windowReady = false;
            if (app->renderer) app->renderer->shutdown();
            termEGL(app);
            break;

        case CMD_INPUT_QUEUE_CREATED:
            break;

        case CMD_INPUT_QUEUE_DESTROYED:
            break;

        case CMD_RESUME:
            app->hasFocus = true;
            break;

        case CMD_PAUSE:
            app->hasFocus = false;
            break;

        case CMD_DESTROY:
            app->destroyed = true;
            break;
    }

    pthread_mutex_lock(&app->mutex);
    pthread_cond_signal(&app->cond);
    pthread_mutex_unlock(&app->mutex);
}

static void* appThread(void* arg) {
    AppState* app = static_cast<AppState*>(arg);

    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ALooper_addFd(looper, app->msgPipe[0], 1, ALOOPER_EVENT_INPUT, nullptr, nullptr);

    pthread_mutex_lock(&app->mutex);
    app->running = true;
    pthread_cond_signal(&app->cond);
    pthread_mutex_unlock(&app->mutex);

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!app->destroyed) {
        int events;
        int ident;

        while ((ident = ALooper_pollOnce(app->windowReady && app->hasFocus ? 0 : -1,
                                          nullptr, &events, nullptr)) >= 0) {
            if (ident == 1) {
                processCmd(app);
            }
        }

        if (app->destroyed) break;

        if (app->windowReady && app->hasFocus && app->renderer && app->screenManager) {
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;
            if (dt > 0.1f) dt = 0.016f;

            app->renderer->setDeltaTime(dt);
            app->screenManager->update(dt);
            if (app->animations) app->animations->update(dt);

            app->renderer->setScreenSize(app->width, app->height);
            app->renderer->beginFrame();
            app->screenManager->render(*app->renderer,
                                        static_cast<float>(app->width),
                                        static_cast<float>(app->height));
            app->renderer->endFrame();
            eglSwapBuffers(app->display, app->surface);
        }

        if (app->inputQueue) {
            AInputEvent* event = nullptr;
            while (AInputQueue_getEvent(app->inputQueue, &event) >= 0) {
                if (AInputQueue_preDispatchEvent(app->inputQueue, event)) continue;

                int handled = 0;
                if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
                    handleTouchInput(app, event);
                    handled = 1;
                }
                AInputQueue_finishEvent(app->inputQueue, event, handled);
            }
        }
    }

    if (app->screenManager) app->screenManager.reset();
    if (app->animations) app->animations.reset();
    if (app->lua) { app->lua->shutdown(); app->lua.reset(); }
    if (app->renderer) { app->renderer->shutdown(); app->renderer.reset(); }
    if (app->profiles) app->profiles.reset();
    if (app->factory) app->factory.reset();
    if (app->layout) app->layout.reset();
    termEGL(app);

    ALooper_removeFd(looper, app->msgPipe[0]);

    return nullptr;
}

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
    AppState* app = static_cast<AppState*>(activity->instance);
    pthread_mutex_lock(&app->mutex);
    app->window = window;
    sendCmd(app, CMD_WINDOW_CREATED);
    pthread_cond_wait(&app->cond, &app->mutex);
    pthread_mutex_unlock(&app->mutex);
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
    AppState* app = static_cast<AppState*>(activity->instance);
    pthread_mutex_lock(&app->mutex);
    sendCmd(app, CMD_WINDOW_DESTROYED);
    pthread_cond_wait(&app->cond, &app->mutex);
    app->window = nullptr;
    pthread_mutex_unlock(&app->mutex);
}

static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue) {
    AppState* app = static_cast<AppState*>(activity->instance);
    pthread_mutex_lock(&app->mutex);
    app->inputQueue = queue;
    sendCmd(app, CMD_INPUT_QUEUE_CREATED);
    pthread_cond_wait(&app->cond, &app->mutex);
    pthread_mutex_unlock(&app->mutex);
}

static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue) {
    AppState* app = static_cast<AppState*>(activity->instance);
    pthread_mutex_lock(&app->mutex);
    sendCmd(app, CMD_INPUT_QUEUE_DESTROYED);
    pthread_cond_wait(&app->cond, &app->mutex);
    app->inputQueue = nullptr;
    pthread_mutex_unlock(&app->mutex);
}

static void onResume(ANativeActivity* activity) {
    AppState* app = static_cast<AppState*>(activity->instance);
    sendCmd(app, CMD_RESUME);
}

static void onPause(ANativeActivity* activity) {
    AppState* app = static_cast<AppState*>(activity->instance);
    sendCmd(app, CMD_PAUSE);
}

static void onDestroy(ANativeActivity* activity) {
    AppState* app = static_cast<AppState*>(activity->instance);
    pthread_mutex_lock(&app->mutex);
    sendCmd(app, CMD_DESTROY);
    pthread_mutex_unlock(&app->mutex);

    pthread_t thread;
    memcpy(&thread, &app->mutex, 0);

    close(app->msgPipe[0]);
    close(app->msgPipe[1]);
    pthread_mutex_destroy(&app->mutex);
    pthread_cond_destroy(&app->cond);
    delete app;
}

extern "C" void ANativeActivity_onCreate(ANativeActivity* activity,
                                          void* savedState, size_t savedStateSize) {
    LOGI("ANativeActivity_onCreate");

    auto* app = new AppState();
    app->activity = activity;

    pthread_mutex_init(&app->mutex, nullptr);
    pthread_cond_init(&app->cond, nullptr);
    pipe(app->msgPipe);

    activity->instance = app;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    activity->callbacks->onResume = onResume;
    activity->callbacks->onPause = onPause;
    activity->callbacks->onDestroy = onDestroy;

    pthread_t thread;
    pthread_mutex_lock(&app->mutex);
    pthread_create(&thread, nullptr, appThread, app);
    while (!app->running) {
        pthread_cond_wait(&app->cond, &app->mutex);
    }
    pthread_mutex_unlock(&app->mutex);
    pthread_detach(thread);
}
