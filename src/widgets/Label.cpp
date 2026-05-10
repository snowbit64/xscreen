#include "Label.h"
#include "../renderer/Renderer.h"

namespace xscreen {

void Label::setText(const std::string& text) { text_ = text; }
const std::string& Label::getText() const { return text_; }

void Label::setFontSize(float size) { fontSize_ = size; }
float Label::getFontSize() const { return fontSize_; }

void Label::setTextColor(const Color4& color) { textColor_ = color; }
const Color4& Label::getTextColor() const { return textColor_; }

void Label::setTextAlignment(Alignment align) { textAlign_ = align; }
Alignment Label::getTextAlignment() const { return textAlign_; }

void Label::render(Renderer& renderer) {
    if (!isVisible()) return;

    const auto& r = computedRect_;
    Color4 color = textColor_;
    color.a *= getEffectiveAlpha();

    Vec2 textSize = renderer.measureText(text_, fontSize_);
    float tx = r.x;
    float ty = r.y + (r.h - textSize.y) * 0.5f;

    switch (textAlign_) {
        case Alignment::Center:
            tx = r.x + (r.w - textSize.x) * 0.5f;
            break;
        case Alignment::End:
            tx = r.x + r.w - textSize.x;
            break;
        default:
            break;
    }

    renderer.setClipRect(r);
    renderer.drawText(text_, tx, ty, fontSize_, color);
    renderer.clearClipRect();
    UIElement::render(renderer);
}

std::string Label::getTypeName() const { return "Label"; }

} // namespace xscreen
