#pragma once

#include "../core/UIElement.h"

namespace xscreen {

class Label : public UIElement {
public:
    void setText(const std::string& text);
    const std::string& getText() const;

    void setFontSize(float size);
    float getFontSize() const;

    void setTextColor(const Color4& color);
    const Color4& getTextColor() const;

    void setTextAlignment(Alignment align);
    Alignment getTextAlignment() const;

    void render(Renderer& renderer) override;
    std::string getTypeName() const override;

private:
    std::string text_;
    float fontSize_ = 20.0f;
    Color4 textColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
    Alignment textAlign_ = Alignment::Start;
};

} // namespace xscreen
