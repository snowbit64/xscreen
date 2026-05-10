#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace xscreen {

class Renderer;
class LayoutEngine;

class UIElement : public std::enable_shared_from_this<UIElement> {
public:
    UIElement();
    virtual ~UIElement();

    void setId(const std::string& id);
    const std::string& getId() const;

    void setProfile(const std::string& profile);
    const std::string& getProfile() const;

    void setParent(UIElement* parent);
    UIElement* getParent() const;

    void addChild(std::shared_ptr<UIElement> child);
    void removeChild(const std::shared_ptr<UIElement>& child);
    const std::vector<std::shared_ptr<UIElement>>& getChildren() const;
    std::shared_ptr<UIElement> findChildById(const std::string& id, bool recursive = true) const;

    void setPosition(const Vec2& pos);
    const Vec2& getPosition() const;

    void setSize(const Vec2& size);
    const Vec2& getSize() const;

    void setSizeValue(const SizeValue& w, const SizeValue& h);
    const SizeValue& getWidthValue() const;
    const SizeValue& getHeightValue() const;

    void setComputedRect(const Rect& rect);
    const Rect& getComputedRect() const;

    void setVisible(bool visible);
    bool isVisible() const;

    void setVisibility(Visibility vis);
    Visibility getVisibility() const;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setAlpha(float alpha);
    float getAlpha() const;

    float getEffectiveAlpha() const;

    void setAlignment(Alignment alignX, Alignment alignY);
    Alignment getAlignmentX() const;
    Alignment getAlignmentY() const;

    void setPadding(const Padding& padding);
    const Padding& getPadding() const;

    void setSpacing(float spacing);
    float getSpacing() const;

    void setProperty(const std::string& name, const std::string& value);
    std::string getProperty(const std::string& name, const std::string& fallback = "") const;
    bool hasProperty(const std::string& name) const;

    void setCallback(const std::string& event, const std::string& funcName);
    std::string getCallback(const std::string& event) const;

    virtual void update(float dt);
    virtual void render(Renderer& renderer);
    virtual bool handleInput(const InputEvent& event);

    virtual std::string getTypeName() const;

protected:
    std::string id_;
    std::string profile_;
    UIElement* parent_ = nullptr;
    std::vector<std::shared_ptr<UIElement>> children_;

    Vec2 position_;
    Vec2 size_;
    SizeValue widthValue_;
    SizeValue heightValue_;
    Rect computedRect_;

    Visibility visibility_ = Visibility::Visible;
    bool enabled_ = true;
    float alpha_ = 1.0f;

    Alignment alignX_ = Alignment::Start;
    Alignment alignY_ = Alignment::Start;

    Padding padding_;
    float spacing_ = 0.0f;

    std::unordered_map<std::string, std::string> properties_;
    std::unordered_map<std::string, std::string> callbacks_;
};

} // namespace xscreen
