#include "ProgressBar.h"
#include "../renderer/Renderer.h"
#include <algorithm>

namespace xscreen {

void ProgressBar::setValue(float value) { value_ = value; }
float ProgressBar::getValue() const { return value_; }

void ProgressBar::setMinValue(float min) { minValue_ = min; }
void ProgressBar::setMaxValue(float max) { maxValue_ = max; }

void ProgressBar::setBarColor(const Color4& color) { barColor_ = color; }
void ProgressBar::setBackgroundColor(const Color4& color) { bgColor_ = color; }
void ProgressBar::setCornerRadius(float radius) { cornerRadius_ = radius; }

void ProgressBar::render(Renderer& renderer) {
    if (!isVisible()) return;

    const auto& r = computedRect_;
    float alpha = getEffectiveAlpha();

    Color4 bg = bgColor_;
    bg.a *= alpha;
    renderer.drawRoundedRect(r, bg, cornerRadius_);

    float range = maxValue_ - minValue_;
    float normalizedValue = (range > 0.0f)
        ? std::clamp((value_ - minValue_) / range, 0.0f, 1.0f)
        : 0.0f;

    if (normalizedValue > 0.0f) {
        Color4 bar = barColor_;
        bar.a *= alpha;
        Rect fillRect = {r.x, r.y, r.w * normalizedValue, r.h};
        renderer.drawRoundedRect(fillRect, bar, cornerRadius_);
    }

    UIElement::render(renderer);
}

std::string ProgressBar::getTypeName() const { return "ProgressBar"; }

} // namespace xscreen
