#pragma once

#include "../core/UIElement.h"

namespace xscreen {

class ScrollView : public UIElement {
public:
    void setScrollOffset(float offset);
    float getScrollOffset() const;

    void setContentHeight(float height);
    float getContentHeight() const;

    void setScrollBarColor(const Color4& color);
    void setBackgroundColor(const Color4& color);

    void render(Renderer& renderer) override;
    bool handleInput(const InputEvent& event) override;
    void update(float dt) override;
    std::string getTypeName() const override;

private:
    float scrollOffset_ = 0.0f;
    float contentHeight_ = 0.0f;
    float scrollVelocity_ = 0.0f;
    Color4 scrollBarColor_ = {1.0f, 1.0f, 1.0f, 0.4f};
    Color4 bgColor_ = {0.0f, 0.0f, 0.0f, 0.0f};
    bool dragging_ = false;
    float dragStartY_ = 0.0f;
    float dragStartOffset_ = 0.0f;
};

} // namespace xscreen
