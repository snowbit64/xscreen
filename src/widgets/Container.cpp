#include "Container.h"
#include "../renderer/Renderer.h"

namespace xscreen {

void Container::setBackgroundColor(const Color4& color) { bgColor_ = color; }
void Container::setCornerRadius(float radius) { cornerRadius_ = radius; }
void Container::setBorderColor(const Color4& color) { borderColor_ = color; }
void Container::setBorderWidth(float width) { borderWidth_ = width; }

void Container::render(Renderer& renderer) {
    if (!isVisible()) return;

    const auto& r = computedRect_;
    float alpha = getEffectiveAlpha();

    if (bgColor_.a > 0.0f) {
        Color4 bg = bgColor_;
        bg.a *= alpha;
        if (cornerRadius_ > 0.0f) {
            renderer.drawRoundedRect(r, bg, cornerRadius_);
        } else {
            renderer.drawRect(r, bg);
        }
    }

    if (borderWidth_ > 0.0f && borderColor_.a > 0.0f) {
        Color4 bc = borderColor_;
        bc.a *= alpha;
        renderer.drawRectOutline(r, bc, borderWidth_);
    }

    UIElement::render(renderer);
}

std::string Container::getTypeName() const { return "Container"; }

} // namespace xscreen
