#include "TextField.h"
#include "../renderer/Renderer.h"
#include <algorithm>

namespace xscreen {

void TextField::setText(const std::string& text) {
    text_ = text;
    cursorPos_ = static_cast<int>(text_.size());
}

const std::string& TextField::getText() const { return text_; }

void TextField::setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; }
const std::string& TextField::getPlaceholder() const { return placeholder_; }

void TextField::setFontSize(float size) { fontSize_ = size; }
void TextField::setTextColor(const Color4& color) { textColor_ = color; }
void TextField::setBackgroundColor(const Color4& color) { bgColor_ = color; }
void TextField::setPlaceholderColor(const Color4& color) { placeholderColor_ = color; }
void TextField::setCornerRadius(float radius) { cornerRadius_ = radius; }
void TextField::setMaxLength(int maxLen) { maxLength_ = maxLen; }

bool TextField::isFocused() const { return focused_; }
void TextField::setFocused(bool focused) { focused_ = focused; cursorBlink_ = 0.0f; }

void TextField::update(float dt) {
    if (focused_) {
        cursorBlink_ += dt;
        if (cursorBlink_ > 1.0f) cursorBlink_ -= 1.0f;
    }
    UIElement::update(dt);
}

void TextField::render(Renderer& renderer) {
    if (!isVisible()) return;

    const auto& r = computedRect_;
    float alpha = getEffectiveAlpha();

    Color4 bg = bgColor_;
    bg.a *= alpha;
    renderer.drawRoundedRect(r, bg, cornerRadius_);

    if (focused_) {
        Color4 focusBorder = {0.3f, 0.6f, 0.3f, alpha};
        renderer.drawRectOutline(r, focusBorder, 2.0f);
    }

    float textX = r.x + 8.0f;
    float textY = r.y + (r.h - fontSize_) * 0.5f;

    renderer.setClipRect(r);
    if (text_.empty() && !focused_) {
        Color4 ph = placeholderColor_;
        ph.a *= alpha;
        renderer.drawText(placeholder_, textX, textY, fontSize_, ph);
    } else {
        Color4 tc = textColor_;
        tc.a *= alpha;
        renderer.drawText(text_, textX, textY, fontSize_, tc);

        if (focused_ && cursorBlink_ < 0.5f) {
            std::string beforeCursor = text_.substr(0, cursorPos_);
            Vec2 cursorOffset = renderer.measureText(beforeCursor, fontSize_);
            Color4 cursorColor = {1.0f, 1.0f, 1.0f, alpha};
            renderer.drawRect({textX + cursorOffset.x, textY, 2.0f, fontSize_}, cursorColor);
        }
    }
    renderer.clearClipRect();

    UIElement::render(renderer);
}

bool TextField::handleInput(const InputEvent& event) {
    if (!isVisible() || !isEnabled()) return false;

    const auto& r = computedRect_;

    if (event.action == InputAction::Press) {
        bool inside = r.contains(event.x, event.y);
        bool wasFocused = focused_;
        focused_ = inside;
        cursorBlink_ = 0.0f;
        if (focused_ != wasFocused && focusCallback_) {
            focusCallback_(focused_);
        }
        if (inside) {
            cursorPos_ = static_cast<int>(text_.size());
            return true;
        }
    }

    if (focused_ && event.action == InputAction::Press && event.key != 0) {
        if (event.key == 259 && cursorPos_ > 0) {
            text_.erase(cursorPos_ - 1, 1);
            cursorPos_--;
        } else if (event.key == 261 && cursorPos_ < static_cast<int>(text_.size())) {
            text_.erase(cursorPos_, 1);
        } else if (event.key == 263 && cursorPos_ > 0) {
            cursorPos_--;
        } else if (event.key == 262 && cursorPos_ < static_cast<int>(text_.size())) {
            cursorPos_++;
        } else if (event.key >= 32 && event.key < 128 &&
                   static_cast<int>(text_.size()) < maxLength_) {
            text_.insert(cursorPos_, 1, static_cast<char>(event.key));
            cursorPos_++;
        }
        cursorBlink_ = 0.0f;
        return true;
    }

    return false;
}

std::string TextField::getTypeName() const { return "TextField"; }

} // namespace xscreen
