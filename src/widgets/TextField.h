#pragma once

#include "../core/UIElement.h"
#include <functional>

namespace xscreen {

class TextField : public UIElement {
public:
    void setText(const std::string& text);
    const std::string& getText() const;

    void setPlaceholder(const std::string& placeholder);
    const std::string& getPlaceholder() const;

    void setFontSize(float size);
    void setTextColor(const Color4& color);
    void setBackgroundColor(const Color4& color);
    void setPlaceholderColor(const Color4& color);
    void setCornerRadius(float radius);
    void setMaxLength(int maxLen);

    bool isFocused() const;
    void setFocused(bool focused);

    void render(Renderer& renderer) override;
    bool handleInput(const InputEvent& event) override;
    void update(float dt) override;
    std::string getTypeName() const override;

private:
    std::string text_;
    std::string placeholder_;
    float fontSize_ = 20.0f;
    Color4 textColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
    Color4 bgColor_ = {0.12f, 0.12f, 0.12f, 1.0f};
    Color4 placeholderColor_ = {0.5f, 0.5f, 0.5f, 1.0f};
    float cornerRadius_ = 4.0f;
    int maxLength_ = 256;
    bool focused_ = false;
    float cursorBlink_ = 0.0f;
    int cursorPos_ = 0;
};

} // namespace xscreen
