#pragma once

#include "UIElement.h"
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace xscreen {

class WidgetFactory {
public:
    using Creator = std::function<std::shared_ptr<UIElement>()>;

    void registerType(const std::string& typeName, Creator creator);

    std::shared_ptr<UIElement> create(const std::string& typeName) const;

    bool hasType(const std::string& typeName) const;

    void registerDefaults();

private:
    std::unordered_map<std::string, Creator> creators_;
};

} // namespace xscreen
