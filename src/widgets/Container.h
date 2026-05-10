#pragma once

#include "../core/UIElement.h"

namespace xscreen {

class Container : public UIElement {
public:
    void setBackgroundColor(const Color4& color);
    void setCornerRadius(float radius);
    void setBorderColor(const Color4& color);
    void setBorderWidth(float width);

    void render(Renderer& renderer) override;
    std::string getTypeName() const override;

private:
    Color4 bgColor_ = {0.0f, 0.0f, 0.0f, 0.0f};
    Color4 borderColor_ = {0.0f, 0.0f, 0.0f, 0.0f};
    float cornerRadius_ = 0.0f;
    float borderWidth_ = 0.0f;
};

} // namespace xscreen
