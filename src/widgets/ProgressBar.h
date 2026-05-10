#pragma once

#include "../core/UIElement.h"

namespace xscreen {

class ProgressBar : public UIElement {
public:
    void setValue(float value);
    float getValue() const;

    void setMinValue(float min);
    void setMaxValue(float max);

    void setBarColor(const Color4& color);
    void setBackgroundColor(const Color4& color);
    void setCornerRadius(float radius);

    void render(Renderer& renderer) override;
    std::string getTypeName() const override;

private:
    float value_ = 0.0f;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    Color4 barColor_ = {0.2f, 0.6f, 0.2f, 1.0f};
    Color4 bgColor_ = {0.15f, 0.15f, 0.15f, 1.0f};
    float cornerRadius_ = 4.0f;
};

} // namespace xscreen
