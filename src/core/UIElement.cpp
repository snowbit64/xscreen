#include "UIElement.h"
#include "../renderer/Renderer.h"

namespace xscreen {

UIElement::UIElement() = default;
UIElement::~UIElement() = default;

void UIElement::setId(const std::string& id) { id_ = id; }
const std::string& UIElement::getId() const { return id_; }

void UIElement::setProfile(const std::string& profile) { profile_ = profile; }
const std::string& UIElement::getProfile() const { return profile_; }

void UIElement::setParent(UIElement* parent) { parent_ = parent; }
UIElement* UIElement::getParent() const { return parent_; }

void UIElement::addChild(std::shared_ptr<UIElement> child) {
    child->setParent(this);
    children_.push_back(std::move(child));
}

void UIElement::removeChild(const std::shared_ptr<UIElement>& child) {
    child->setParent(nullptr);
    children_.erase(
        std::remove(children_.begin(), children_.end(), child),
        children_.end());
}

const std::vector<std::shared_ptr<UIElement>>& UIElement::getChildren() const {
    return children_;
}

std::shared_ptr<UIElement> UIElement::findChildById(const std::string& id, bool recursive) const {
    for (auto& child : children_) {
        if (child->getId() == id) return child;
        if (recursive) {
            auto found = child->findChildById(id, true);
            if (found) return found;
        }
    }
    return nullptr;
}

void UIElement::setPosition(const Vec2& pos) { position_ = pos; }
const Vec2& UIElement::getPosition() const { return position_; }

void UIElement::setSize(const Vec2& size) { size_ = size; }
const Vec2& UIElement::getSize() const { return size_; }

void UIElement::setSizeValue(const SizeValue& w, const SizeValue& h) {
    widthValue_ = w;
    heightValue_ = h;
}

const SizeValue& UIElement::getWidthValue() const { return widthValue_; }
const SizeValue& UIElement::getHeightValue() const { return heightValue_; }

void UIElement::setComputedRect(const Rect& rect) { computedRect_ = rect; }
const Rect& UIElement::getComputedRect() const { return computedRect_; }

void UIElement::setVisible(bool visible) {
    visibility_ = visible ? Visibility::Visible : Visibility::Hidden;
}

bool UIElement::isVisible() const {
    return visibility_ == Visibility::Visible;
}

void UIElement::setVisibility(Visibility vis) { visibility_ = vis; }
Visibility UIElement::getVisibility() const { return visibility_; }

void UIElement::setEnabled(bool enabled) { enabled_ = enabled; }
bool UIElement::isEnabled() const { return enabled_; }

void UIElement::setAlpha(float alpha) { alpha_ = alpha; }
float UIElement::getAlpha() const { return alpha_; }

float UIElement::getEffectiveAlpha() const {
    float a = alpha_;
    if (parent_) a *= parent_->getEffectiveAlpha();
    return a;
}

void UIElement::setAlignment(Alignment alignX, Alignment alignY) {
    alignX_ = alignX;
    alignY_ = alignY;
}

Alignment UIElement::getAlignmentX() const { return alignX_; }
Alignment UIElement::getAlignmentY() const { return alignY_; }

void UIElement::setPadding(const Padding& padding) { padding_ = padding; }
const Padding& UIElement::getPadding() const { return padding_; }

void UIElement::setSpacing(float spacing) { spacing_ = spacing; }
float UIElement::getSpacing() const { return spacing_; }

void UIElement::setProperty(const std::string& name, const std::string& value) {
    properties_[name] = value;
}

std::string UIElement::getProperty(const std::string& name, const std::string& fallback) const {
    auto it = properties_.find(name);
    return (it != properties_.end()) ? it->second : fallback;
}

bool UIElement::hasProperty(const std::string& name) const {
    return properties_.find(name) != properties_.end();
}

void UIElement::setCallback(const std::string& event, const std::string& funcName) {
    callbacks_[event] = funcName;
}

std::string UIElement::getCallback(const std::string& event) const {
    auto it = callbacks_.find(event);
    return (it != callbacks_.end()) ? it->second : "";
}

void UIElement::update(float dt) {
    for (auto& child : children_) {
        if (child->isVisible()) {
            child->update(dt);
        }
    }
}

void UIElement::render(Renderer& renderer) {
    if (!isVisible()) return;
    for (auto& child : children_) {
        child->render(renderer);
    }
}

bool UIElement::handleInput(const InputEvent& event) {
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->isVisible() && (*it)->isEnabled()) {
            if ((*it)->handleInput(event)) return true;
        }
    }
    return false;
}

std::string UIElement::getTypeName() const { return "UIElement"; }

} // namespace xscreen
