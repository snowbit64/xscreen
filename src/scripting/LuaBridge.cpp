#include "LuaBridge.h"
#include <cstdio>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace xscreen {

static LuaBridge* s_instance = nullptr;

static int lua_ui_show(lua_State* L) {
    const char* screen = luaL_checkstring(L, 1);
    if (s_instance) {
        auto it = s_instance->getState();
        // Find the "show" binding
    }
    lua_getglobal(L, "_xscreen_callbacks");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "show");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, screen);
            lua_call(L, 1, 0);
        } else {
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return 0;
}

static int lua_ui_close(lua_State* L) {
    const char* screen = luaL_checkstring(L, 1);
    lua_getglobal(L, "_xscreen_callbacks");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "close");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, screen);
            lua_call(L, 1, 0);
        } else {
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return 0;
}

static int lua_ui_push(lua_State* L) {
    const char* screen = luaL_checkstring(L, 1);
    lua_getglobal(L, "_xscreen_callbacks");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "push");
        if (lua_isfunction(L, -1)) {
            lua_pushstring(L, screen);
            lua_call(L, 1, 0);
        } else {
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return 0;
}

static int lua_ui_pop(lua_State* L) {
    lua_getglobal(L, "_xscreen_callbacks");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "pop");
        if (lua_isfunction(L, -1)) {
            lua_call(L, 0, 0);
        } else {
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return 0;
}

static int lua_log_info(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    std::printf("[Lua] %s\n", msg);
    return 0;
}

LuaBridge::LuaBridge() { s_instance = this; }

LuaBridge::~LuaBridge() {
    shutdown();
    if (s_instance == this) s_instance = nullptr;
}

bool LuaBridge::init() {
    L_ = luaL_newstate();
    if (!L_) return false;

    luaL_openlibs(L_);

    lua_newtable(L_);

    lua_pushcfunction(L_, lua_ui_show);
    lua_setfield(L_, -2, "show");

    lua_pushcfunction(L_, lua_ui_close);
    lua_setfield(L_, -2, "close");

    lua_pushcfunction(L_, lua_ui_push);
    lua_setfield(L_, -2, "push");

    lua_pushcfunction(L_, lua_ui_pop);
    lua_setfield(L_, -2, "pop");

    lua_setglobal(L_, "UI");

    lua_pushcfunction(L_, lua_log_info);
    lua_setglobal(L_, "logInfo");

    lua_newtable(L_);
    lua_setglobal(L_, "_xscreen_callbacks");

    return true;
}

void LuaBridge::shutdown() {
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
}

bool LuaBridge::loadScript(const std::string& path) {
    if (!L_) return false;
    int result = luaL_dofile(L_, path.c_str());
    if (result != LUA_OK) {
        const char* err = lua_tostring(L_, -1);
        std::fprintf(stderr, "[LuaBridge] Error loading '%s': %s\n", path.c_str(), err ? err : "unknown");
        lua_pop(L_, 1);
        return false;
    }
    return true;
}

bool LuaBridge::loadString(const std::string& code) {
    if (!L_) return false;
    int result = luaL_dostring(L_, code.c_str());
    if (result != LUA_OK) {
        const char* err = lua_tostring(L_, -1);
        std::fprintf(stderr, "[LuaBridge] Error: %s\n", err ? err : "unknown");
        lua_pop(L_, 1);
        return false;
    }
    return true;
}

bool LuaBridge::callFunction(const std::string& tableName, const std::string& funcName) {
    if (!L_) return false;

    lua_getglobal(L_, tableName.c_str());
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }

    lua_getfield(L_, -1, funcName.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 2);
        return false;
    }

    lua_pushvalue(L_, -2);

    int result = lua_pcall(L_, 1, 0, 0);
    if (result != LUA_OK) {
        const char* err = lua_tostring(L_, -1);
        std::fprintf(stderr, "[LuaBridge] Error calling %s:%s: %s\n",
                     tableName.c_str(), funcName.c_str(), err ? err : "unknown");
        lua_pop(L_, 1);
    }

    lua_pop(L_, 1);
    return result == LUA_OK;
}

bool LuaBridge::callFunction(const std::string& tableName, const std::string& funcName,
                             const std::string& arg1) {
    if (!L_) return false;

    lua_getglobal(L_, tableName.c_str());
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }

    lua_getfield(L_, -1, funcName.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 2);
        return false;
    }

    lua_pushvalue(L_, -2);
    lua_pushstring(L_, arg1.c_str());

    int result = lua_pcall(L_, 2, 0, 0);
    if (result != LUA_OK) {
        const char* err = lua_tostring(L_, -1);
        std::fprintf(stderr, "[LuaBridge] Error calling %s:%s: %s\n",
                     tableName.c_str(), funcName.c_str(), err ? err : "unknown");
        lua_pop(L_, 1);
    }

    lua_pop(L_, 1);
    return result == LUA_OK;
}

bool LuaBridge::callGlobalFunction(const std::string& funcName) {
    if (!L_) return false;

    lua_getglobal(L_, funcName.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }

    int result = lua_pcall(L_, 0, 0, 0);
    if (result != LUA_OK) {
        const char* err = lua_tostring(L_, -1);
        std::fprintf(stderr, "[LuaBridge] Error calling %s: %s\n",
                     funcName.c_str(), err ? err : "unknown");
        lua_pop(L_, 1);
        return false;
    }
    return true;
}

void LuaBridge::registerUIBinding(const std::string& name,
                                  std::function<void(const std::string&)> func) {
    uiBindings_[name] = std::move(func);
}

void LuaBridge::setGlobalString(const std::string& name, const std::string& value) {
    if (!L_) return;
    lua_pushstring(L_, value.c_str());
    lua_setglobal(L_, name.c_str());
}

void LuaBridge::setGlobalNumber(const std::string& name, double value) {
    if (!L_) return;
    lua_pushnumber(L_, value);
    lua_setglobal(L_, name.c_str());
}

std::string LuaBridge::getGlobalString(const std::string& name) const {
    if (!L_) return "";
    lua_getglobal(L_, name.c_str());
    const char* str = lua_tostring(L_, -1);
    std::string result = str ? str : "";
    lua_pop(L_, 1);
    return result;
}

lua_State* LuaBridge::getState() const { return L_; }

} // namespace xscreen
