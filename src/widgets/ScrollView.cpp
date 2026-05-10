#include "ScrollView.h"
#include "../renderer/Renderer.h"
#include <algorithm>

namespace xscreen {

void ScrollView::setScrollOffset(float offset) { scrollOffset_ = offset; }
float ScrollView::getScrollOffset() const { return scrollOffset_; }

void ScrollView::setContentHeight(float height) { contentHeight_ = height; }
float ScrollView::getContentHeight() const { return contentHeight_; }

void ScrollView::setScrollBarColor(const Color4& color) { scrollBarColor_ = color; }
void ScrollView::setBackgroundColor(const Color4& color) { bgColor_ = color; }

void ScrollView::update(float dt) {
    if (!dragging_ && std::abs(scrollVelocity_) > 0.1f) {
        scrollOffset_ += scrollVelocity_ * dt;
        scrollVelocity_ *= 0.92f;
    }

    float maxScroll = std::max(0.0f, contentHeight_ - computedRect_.h);
    scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScroll);

    UIElement::update(dt);
}

void ScrollView::render(Renderer& renderer) {
    if (!isVisible()) return;

    const auto& r = computedRect_;
    float alpha = getEffectiveAlpha();

    if (bgColor_.a > 0.0f) {
        Color4 bg = bgColor_;
        bg.a *= alpha;
        renderer.drawRect(r, bg);
    }

    renderer.setClipRect(r);

    for (auto& child : children_) {
        if (child->isVisible()) {
            Rect cr = child->getComputedRect();
            cr.y -= scrollOffset_;
            Rect adjusted = cr;
            child->setComputedRect(adjusted);
            child->render(renderer);
            child->setComputedRect(cr);
        }
    }

    renderer.clearClipRect();

    if (contentHeight_ > r.h) {
        float viewRatio = r.h / contentHeight_;
        float barHeight = std::max(20.0f, r.h * viewRatio);
        float maxScroll = contentHeight_ - r.h;
        float barY = r.y + (scrollOffset_ / maxScroll) * (r.h - barHeight);

        Color4 sc = scrollBarColor_;
        sc.a *= alpha;
        renderer.drawRoundedRect({r.x + r.w - 6.0f, barY, 4.0f, barHeight}, sc, 2.0f);
    }
}

bool ScrollView::handleInput(const InputEvent& event) {
    if (!isVisible() || !isEnabled()) return false;

    const auto& r = computedRect_;

    if (event.action == InputAction::Scroll && r.contains(event.x, event.y)) {
        scrollOffset_ -= event.scrollDelta * 30.0f;
        float maxScroll = std::max(0.0f, contentHeight_ - r.h);
        scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScroll);
        return true;
    }

    if (event.action == InputAction::Press && r.contains(event.x, event.y)) {
        dragging_ = true;
        dragStartY_ = event.y;
        dragStartOffset_ = scrollOffset_;
        scrollVelocity_ = 0.0f;
    }

    if (event.action == InputAction::Move && dragging_) {
        float delta = dragStartY_ - event.y;
        scrollOffset_ = dragStartOffset_ + delta;
        float maxScroll = std::max(0.0f, contentHeight_ - r.h);
        scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScroll);
        return true;
    }

    if (event.action == InputAction::Release && dragging_) {
        dragging_ = false;
    }

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->isVisible() && (*it)->isEnabled()) {
            InputEvent adjusted = event;
            adjusted.y += scrollOffset_;
            if ((*it)->handleInput(adjusted)) return true;
        }
    }

    return false;
}

std::string ScrollView::getTypeName() const { return "ScrollView"; }

} // namespace xscreen
