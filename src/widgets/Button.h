#pragma once

#include "../core/UIElement.h"
#include <functional>

namespace xscreen {

class Button : public UIElement {
public:
    void setText(const std::string& text);
    const std::string& getText() const;

    void setFontSize(float size);
    float getFontSize() const;

    void setTextColor(const Color4& color);
    void setBackgroundColor(const Color4& color);
    void setHoverColor(const Color4& color);
    void setPressedColor(const Color4& color);
    void setCornerRadius(float radius);

    bool isHovered() const;
    bool isPressed() const;

    void render(Renderer& renderer) override;
    bool handleInput(const InputEvent& event) override;
    std::string getTypeName() const override;

private:
    std::string text_;
    float fontSize_ = 20.0f;
    Color4 textColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
    Color4 bgColor_ = {0.2f, 0.4f, 0.2f, 1.0f};
    Color4 hoverColor_ = {0.3f, 0.5f, 0.3f, 1.0f};
    Color4 pressedColor_ = {0.15f, 0.3f, 0.15f, 1.0f};
    float cornerRadius_ = 4.0f;
    bool hovered_ = false;
    bool pressed_ = false;
};

} // namespace xscreen
