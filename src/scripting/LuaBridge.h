#pragma once

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

struct lua_State;

namespace xscreen {

class UIElement;

class LuaBridge {
public:
    LuaBridge();
    ~LuaBridge();

    bool init();
    void shutdown();

    bool loadScript(const std::string& path);
    bool loadString(const std::string& code);

    bool callFunction(const std::string& tableName, const std::string& funcName);
    bool callFunction(const std::string& tableName, const std::string& funcName,
                      const std::string& arg1);
    bool callGlobalFunction(const std::string& funcName);

    void registerUIBinding(const std::string& name,
                           std::function<void(const std::string&)> func);

    void setGlobalString(const std::string& name, const std::string& value);
    void setGlobalNumber(const std::string& name, double value);
    std::string getGlobalString(const std::string& name) const;

    lua_State* getState() const;

private:
    lua_State* L_ = nullptr;
    std::unordered_map<std::string, std::function<void(const std::string&)>> uiBindings_;
};

} // namespace xscreen
