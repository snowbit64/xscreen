#include "Button.h"
#include "../renderer/Renderer.h"

namespace xscreen {

void Button::setText(const std::string& text) { text_ = text; }
const std::string& Button::getText() const { return text_; }

void Button::setFontSize(float size) { fontSize_ = size; }
float Button::getFontSize() const { return fontSize_; }

void Button::setTextColor(const Color4& color) { textColor_ = color; }
void Button::setBackgroundColor(const Color4& color) { bgColor_ = color; }
void Button::setHoverColor(const Color4& color) { hoverColor_ = color; }
void Button::setPressedColor(const Color4& color) { pressedColor_ = color; }
void Button::setCornerRadius(float radius) { cornerRadius_ = radius; }

bool Button::isHovered() const { return hovered_; }
bool Button::isPressed() const { return pressed_; }

void Button::render(Renderer& renderer) {
    if (!isVisible()) return;

    const auto& r = computedRect_;
    float alpha = getEffectiveAlpha();

    Color4 bg = pressed_ ? pressedColor_ : (hovered_ ? hoverColor_ : bgColor_);
    bg.a *= alpha;

    if (cornerRadius_ > 0.0f) {
        renderer.drawRoundedRect(r, bg, cornerRadius_);
    } else {
        renderer.drawRect(r, bg);
    }

    if (!text_.empty()) {
        Color4 tc = textColor_;
        tc.a *= alpha;
        renderer.setClipRect(r);
        Vec2 textSize = renderer.measureText(text_, fontSize_);
        float tx = r.x + (r.w - textSize.x) * 0.5f;
        float ty = r.y + (r.h - textSize.y) * 0.5f;
        renderer.drawText(text_, tx, ty, fontSize_, tc);
        renderer.clearClipRect();
    }

    UIElement::render(renderer);
}

bool Button::handleInput(const InputEvent& event) {
    if (!isVisible() || !isEnabled()) return false;

    const auto& r = computedRect_;
    bool inside = r.contains(event.x, event.y);

    if (event.action == InputAction::Move) {
        hovered_ = inside;
        return false;
    }

    if (event.action == InputAction::Press && inside) {
        pressed_ = true;
        return true;
    }

    if (event.action == InputAction::Release && pressed_) {
        pressed_ = false;
        if (inside) {
            return true;
        }
    }

    return false;
}

std::string Button::getTypeName() const { return "Button"; }

} // namespace xscreen
