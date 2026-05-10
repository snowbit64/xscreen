#pragma once

#include "UIElement.h"
#include "ProfileManager.h"
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace tinyxml2 { class XMLElement; }

namespace xscreen {

class WidgetFactory;

struct ScreenDef {
    std::string name;
    std::string script;
    std::shared_ptr<UIElement> root;
};

class XMLParser {
public:
    XMLParser(ProfileManager& profiles, WidgetFactory& factory);

    ScreenDef parseFile(const std::string& path);
    ScreenDef parseString(const std::string& xml);

private:
    void parseProfiles(tinyxml2::XMLElement* profilesEl);
    std::shared_ptr<UIElement> parseElement(tinyxml2::XMLElement* el);
    void applyAttributes(tinyxml2::XMLElement* el, UIElement& element);
    void applyProfileToElement(const std::string& profileName, UIElement& element);

    SizeValue parseSizeValue(const std::string& str) const;
    Alignment parseAlignment(const std::string& str) const;
    Padding parsePadding(const std::string& str) const;
    Color4 parseColor(const std::string& str) const;

    ProfileManager& profiles_;
    WidgetFactory& factory_;
};

} // namespace xscreen
