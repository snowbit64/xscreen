#include "XMLParser.h"
#include "WidgetFactory.h"
#include "../widgets/Button.h"
#include "../widgets/Label.h"
#include "../widgets/ImageWidget.h"
#include "../widgets/Container.h"
#include "../widgets/ScrollView.h"
#include "../widgets/ProgressBar.h"
#include "../widgets/TextField.h"
#include "../layout/VBoxLayout.h"
#include "../layout/HBoxLayout.h"

#include "tinyxml2.h"
#include <cstdio>
#include <sstream>

namespace xscreen {

XMLParser::XMLParser(ProfileManager& profiles, WidgetFactory& factory)
    : profiles_(profiles), factory_(factory) {}

ScreenDef XMLParser::parseFile(const std::string& path) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError err = doc.LoadFile(path.c_str());
    if (err != tinyxml2::XML_SUCCESS) {
        std::fprintf(stderr, "[XMLParser] Failed to load: %s (error %d)\n", path.c_str(), err);
        return {};
    }

    auto* root = doc.RootElement();
    if (!root) return {};

    ScreenDef def;

    auto* profilesEl = root->FirstChildElement("Profiles");
    if (profilesEl) {
        parseProfiles(profilesEl);
    }

    auto* screenEl = root->FirstChildElement("Screen");
    if (screenEl) {
        def.name = screenEl->Attribute("name") ? screenEl->Attribute("name") : "unnamed";
        def.script = screenEl->Attribute("script") ? screenEl->Attribute("script") : "";

        auto screenRoot = std::make_shared<UIElement>();
        screenRoot->setId(def.name);

        auto* child = screenEl->FirstChildElement();
        while (child) {
            auto element = parseElement(child);
            if (element) {
                screenRoot->addChild(element);
            }
            child = child->NextSiblingElement();
        }

        def.root = screenRoot;
    }

    return def;
}

ScreenDef XMLParser::parseString(const std::string& xml) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError err = doc.Parse(xml.c_str());
    if (err != tinyxml2::XML_SUCCESS) {
        std::fprintf(stderr, "[XMLParser] Failed to parse XML string\n");
        return {};
    }

    auto* root = doc.RootElement();
    if (!root) return {};

    ScreenDef def;

    auto* profilesEl = root->FirstChildElement("Profiles");
    if (profilesEl) {
        parseProfiles(profilesEl);
    }

    auto* screenEl = root->FirstChildElement("Screen");
    if (screenEl) {
        def.name = screenEl->Attribute("name") ? screenEl->Attribute("name") : "unnamed";
        def.script = screenEl->Attribute("script") ? screenEl->Attribute("script") : "";

        auto screenRoot = std::make_shared<UIElement>();
        screenRoot->setId(def.name);

        auto* child = screenEl->FirstChildElement();
        while (child) {
            auto element = parseElement(child);
            if (element) {
                screenRoot->addChild(element);
            }
            child = child->NextSiblingElement();
        }

        def.root = screenRoot;
    }

    return def;
}

void XMLParser::parseProfiles(tinyxml2::XMLElement* profilesEl) {
    auto* profileEl = profilesEl->FirstChildElement("Profile");
    while (profileEl) {
        Profile profile;
        profile.name = profileEl->Attribute("name") ? profileEl->Attribute("name") : "";
        profile.extends = profileEl->Attribute("extends") ? profileEl->Attribute("extends") : "";

        auto* propEl = profileEl->FirstChildElement("Property");
        while (propEl) {
            ProfileProperty prop;
            prop.name = propEl->Attribute("name") ? propEl->Attribute("name") : "";
            prop.value = propEl->Attribute("value") ? propEl->Attribute("value") : "";
            if (!prop.name.empty()) {
                profile.properties.push_back(prop);
            }
            propEl = propEl->NextSiblingElement("Property");
        }

        if (!profile.name.empty()) {
            profiles_.registerProfile(profile);
        }

        profileEl = profileEl->NextSiblingElement("Profile");
    }
}

std::shared_ptr<UIElement> XMLParser::parseElement(tinyxml2::XMLElement* el) {
    std::string tagName = el->Name();
    auto element = factory_.create(tagName);
    if (!element) return nullptr;

    applyAttributes(el, *element);

    if (!element->getProfile().empty()) {
        applyProfileToElement(element->getProfile(), *element);
    }

    auto* child = el->FirstChildElement();
    while (child) {
        std::string childTag = child->Name();
        if (childTag != "Profiles" && childTag != "Property") {
            auto childElement = parseElement(child);
            if (childElement) {
                element->addChild(childElement);
            }
        }
        child = child->NextSiblingElement();
    }

    return element;
}

void XMLParser::applyAttributes(tinyxml2::XMLElement* el, UIElement& element) {
    if (auto* val = el->Attribute("id")) element.setId(val);
    if (auto* val = el->Attribute("profile")) element.setProfile(val);

    if (auto* val = el->Attribute("width")) {
        element.setSizeValue(parseSizeValue(val), element.getHeightValue());
    }
    if (auto* val = el->Attribute("height")) {
        element.setSizeValue(element.getWidthValue(), parseSizeValue(val));
    }

    if (auto* val = el->Attribute("align")) {
        element.setAlignment(parseAlignment(val), element.getAlignmentY());
    }
    if (auto* val = el->Attribute("alignX")) {
        element.setAlignment(parseAlignment(val), element.getAlignmentY());
    }
    if (auto* val = el->Attribute("alignY")) {
        element.setAlignment(element.getAlignmentX(), parseAlignment(val));
    }

    if (auto* val = el->Attribute("spacing")) {
        element.setSpacing(parseSizeValue(val).value);
    }

    if (auto* val = el->Attribute("padding")) {
        element.setPadding(parsePadding(val));
    }

    if (auto* val = el->Attribute("visible")) {
        std::string v = val;
        element.setVisible(v != "false" && v != "0");
    }

    if (auto* val = el->Attribute("text")) {
        if (auto* btn = dynamic_cast<Button*>(&element)) btn->setText(val);
        else if (auto* lbl = dynamic_cast<Label*>(&element)) lbl->setText(val);
    }

    if (auto* val = el->Attribute("fontSize")) {
        float fs = std::stof(val);
        if (auto* btn = dynamic_cast<Button*>(&element)) btn->setFontSize(fs);
        else if (auto* lbl = dynamic_cast<Label*>(&element)) lbl->setFontSize(fs);
        else if (auto* tf = dynamic_cast<TextField*>(&element)) tf->setFontSize(fs);
    }

    if (auto* val = el->Attribute("textColor")) {
        Color4 c = parseColor(val);
        if (auto* btn = dynamic_cast<Button*>(&element)) btn->setTextColor(c);
        else if (auto* lbl = dynamic_cast<Label*>(&element)) lbl->setTextColor(c);
        else if (auto* tf = dynamic_cast<TextField*>(&element)) tf->setTextColor(c);
    }

    if (auto* val = el->Attribute("bgColor")) {
        Color4 c = parseColor(val);
        if (auto* btn = dynamic_cast<Button*>(&element)) btn->setBackgroundColor(c);
        else if (auto* cont = dynamic_cast<Container*>(&element)) cont->setBackgroundColor(c);
        else if (auto* sv = dynamic_cast<ScrollView*>(&element)) sv->setBackgroundColor(c);
        else if (auto* pb = dynamic_cast<ProgressBar*>(&element)) pb->setBackgroundColor(c);
        else if (auto* tf = dynamic_cast<TextField*>(&element)) tf->setBackgroundColor(c);
    }

    if (auto* val = el->Attribute("src")) {
        if (auto* img = dynamic_cast<ImageWidget*>(&element)) img->setSource(val);
    }

    if (auto* val = el->Attribute("value")) {
        if (auto* pb = dynamic_cast<ProgressBar*>(&element)) pb->setValue(std::stof(val));
    }

    if (auto* val = el->Attribute("placeholder")) {
        if (auto* tf = dynamic_cast<TextField*>(&element)) tf->setPlaceholder(val);
    }

    if (auto* val = el->Attribute("barColor")) {
        if (auto* pb = dynamic_cast<ProgressBar*>(&element)) pb->setBarColor(parseColor(val));
    }

    if (auto* val = el->Attribute("hoverColor")) {
        if (auto* btn = dynamic_cast<Button*>(&element)) btn->setHoverColor(parseColor(val));
    }

    if (auto* val = el->Attribute("cornerRadius")) {
        float cr = std::stof(val);
        if (auto* btn = dynamic_cast<Button*>(&element)) btn->setCornerRadius(cr);
        else if (auto* cont = dynamic_cast<Container*>(&element)) cont->setCornerRadius(cr);
        else if (auto* pb = dynamic_cast<ProgressBar*>(&element)) pb->setCornerRadius(cr);
        else if (auto* tf = dynamic_cast<TextField*>(&element)) tf->setCornerRadius(cr);
    }

    static const char* callbackAttrs[] = {
        "onClick", "onPress", "onRelease", "onHover",
        "onFocus", "onBlur", "onChange", "onScroll",
        nullptr
    };

    for (int i = 0; callbackAttrs[i]; ++i) {
        if (auto* val = el->Attribute(callbackAttrs[i])) {
            element.setCallback(callbackAttrs[i], val);
        }
    }
}

void XMLParser::applyProfileToElement(const std::string& profileName, UIElement& element) {
    auto props = profiles_.resolveAllProperties(profileName);

    for (const auto& [name, value] : props) {
        if (name == "width") {
            element.setSizeValue(parseSizeValue(value), element.getHeightValue());
        } else if (name == "height") {
            element.setSizeValue(element.getWidthValue(), parseSizeValue(value));
        } else if (name == "fontSize") {
            float fs = std::stof(value);
            if (auto* btn = dynamic_cast<Button*>(&element)) btn->setFontSize(fs);
            else if (auto* lbl = dynamic_cast<Label*>(&element)) lbl->setFontSize(fs);
            else if (auto* tf = dynamic_cast<TextField*>(&element)) tf->setFontSize(fs);
        } else if (name == "textColor") {
            Color4 c = parseColor(value);
            if (auto* btn = dynamic_cast<Button*>(&element)) btn->setTextColor(c);
            else if (auto* lbl = dynamic_cast<Label*>(&element)) lbl->setTextColor(c);
        } else if (name == "bgColor") {
            Color4 c = parseColor(value);
            if (auto* btn = dynamic_cast<Button*>(&element)) btn->setBackgroundColor(c);
            else if (auto* cont = dynamic_cast<Container*>(&element)) cont->setBackgroundColor(c);
        } else if (name == "cornerRadius") {
            float cr = std::stof(value);
            if (auto* btn = dynamic_cast<Button*>(&element)) btn->setCornerRadius(cr);
            else if (auto* cont = dynamic_cast<Container*>(&element)) cont->setCornerRadius(cr);
        } else if (name == "spacing") {
            element.setSpacing(std::stof(value));
        } else if (name == "padding") {
            element.setPadding(parsePadding(value));
        } else if (name == "align") {
            element.setAlignment(parseAlignment(value), element.getAlignmentY());
        } else {
            element.setProperty(name, value);
        }
    }
}

SizeValue XMLParser::parseSizeValue(const std::string& str) const {
    SizeValue sv;
    if (str.empty()) return sv;

    std::string s = str;
    if (s.back() == '%') {
        s.pop_back();
        sv.mode = SizeMode::Percent;
        sv.value = std::stof(s) / 100.0f;
    } else if (s.size() > 2 && s.substr(s.size() - 2) == "px") {
        s = s.substr(0, s.size() - 2);
        sv.mode = SizeMode::Absolute;
        sv.value = std::stof(s);
    } else {
        float val = std::stof(s);
        if (val <= 1.0f && val >= 0.0f) {
            sv.mode = SizeMode::Percent;
            sv.value = val;
        } else {
            sv.mode = SizeMode::Absolute;
            sv.value = val;
        }
    }

    return sv;
}

Alignment XMLParser::parseAlignment(const std::string& str) const {
    if (str == "center") return Alignment::Center;
    if (str == "end" || str == "right" || str == "bottom") return Alignment::End;
    return Alignment::Start;
}

Padding XMLParser::parsePadding(const std::string& str) const {
    Padding p;
    std::istringstream iss(str);
    float values[4] = {0, 0, 0, 0};
    int count = 0;
    while (iss >> values[count] && count < 4) count++;

    if (count == 1) {
        p.top = p.right = p.bottom = p.left = values[0];
    } else if (count == 2) {
        p.top = p.bottom = values[0];
        p.left = p.right = values[1];
    } else if (count == 4) {
        p.top = values[0];
        p.right = values[1];
        p.bottom = values[2];
        p.left = values[3];
    }

    return p;
}

Color4 XMLParser::parseColor(const std::string& str) const {
    Color4 c;
    if (str.empty()) return c;

    if (str[0] == '#') {
        unsigned int hex = 0;
        std::sscanf(str.c_str() + 1, "%x", &hex);
        if (str.size() == 7) {
            c.r = ((hex >> 16) & 0xFF) / 255.0f;
            c.g = ((hex >> 8) & 0xFF) / 255.0f;
            c.b = (hex & 0xFF) / 255.0f;
            c.a = 1.0f;
        } else if (str.size() == 9) {
            c.r = ((hex >> 24) & 0xFF) / 255.0f;
            c.g = ((hex >> 16) & 0xFF) / 255.0f;
            c.b = ((hex >> 8) & 0xFF) / 255.0f;
            c.a = (hex & 0xFF) / 255.0f;
        }
    } else {
        std::istringstream iss(str);
        iss >> c.r >> c.g >> c.b;
        if (!(iss >> c.a)) c.a = 1.0f;
    }

    return c;
}

} // namespace xscreen
