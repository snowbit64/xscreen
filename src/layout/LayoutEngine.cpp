#include "LayoutEngine.h"
#include "VBoxLayout.h"
#include "HBoxLayout.h"
#include "../widgets/ScrollView.h"
#include <algorithm>

namespace xscreen {

void LayoutEngine::compute(UIElement& root, float screenWidth, float screenHeight) {
    Rect screenRect = {0, 0, screenWidth, screenHeight};
    root.setComputedRect(screenRect);
    computeElement(root, screenRect);
}

void LayoutEngine::computeElement(UIElement& element, const Rect& parentRect) {
    Rect contentRect = applyPadding(parentRect, element.getPadding());

    float w = element.getWidthValue().resolve(parentRect.w);
    float h = element.getHeightValue().resolve(parentRect.h);

    if (w <= 0.0f) w = parentRect.w;
    if (h <= 0.0f) h = parentRect.h;

    Rect myRect = {parentRect.x, parentRect.y, w, h};

    switch (element.getAlignmentX()) {
        case Alignment::Center:
            myRect.x = parentRect.x + (parentRect.w - w) * 0.5f;
            break;
        case Alignment::End:
            myRect.x = parentRect.x + parentRect.w - w;
            break;
        default:
            break;
    }

    switch (element.getAlignmentY()) {
        case Alignment::Center:
            myRect.y = parentRect.y + (parentRect.h - h) * 0.5f;
            break;
        case Alignment::End:
            myRect.y = parentRect.y + parentRect.h - h;
            break;
        default:
            break;
    }

    element.setComputedRect(myRect);

    if (dynamic_cast<VBoxLayout*>(&element)) {
        computeVBox(element, myRect);
    } else if (dynamic_cast<HBoxLayout*>(&element)) {
        computeHBox(element, myRect);
    } else {
        computeDefault(element, myRect);
    }
}

void LayoutEngine::computeVBox(UIElement& element, const Rect& parentRect) {
    Rect content = applyPadding(parentRect, element.getPadding());
    float spacing = element.getSpacing();

    float totalFixedHeight = 0.0f;
    int visibleCount = 0;
    for (auto& child : element.getChildren()) {
        if (child->getVisibility() == Visibility::Gone) continue;
        if (!child->isVisible()) continue;

        float ch = child->getHeightValue().resolve(content.h);
        if (ch > 0.0f) totalFixedHeight += ch;
        visibleCount++;
    }

    float totalSpacing = (visibleCount > 1) ? spacing * (visibleCount - 1) : 0.0f;
    float remaining = content.h - totalFixedHeight - totalSpacing;
    float autoHeight = 0.0f;

    int autoCount = 0;
    for (auto& child : element.getChildren()) {
        if (child->getVisibility() == Visibility::Gone) continue;
        if (!child->isVisible()) continue;
        float ch = child->getHeightValue().resolve(content.h);
        if (ch <= 0.0f) autoCount++;
    }
    if (autoCount > 0 && remaining > 0.0f) {
        autoHeight = remaining / autoCount;
    }

    float y = content.y;

    Alignment parentAlignY = element.getAlignmentY();
    if (parentAlignY == Alignment::Center) {
        float totalH = totalFixedHeight + totalSpacing + autoHeight * autoCount;
        y = content.y + (content.h - totalH) * 0.5f;
    } else if (parentAlignY == Alignment::End) {
        float totalH = totalFixedHeight + totalSpacing + autoHeight * autoCount;
        y = content.y + content.h - totalH;
    }

    for (auto& child : element.getChildren()) {
        if (child->getVisibility() == Visibility::Gone) continue;
        if (!child->isVisible()) continue;

        float cw = child->getWidthValue().resolve(content.w);
        float ch = child->getHeightValue().resolve(content.h);

        if (cw <= 0.0f) cw = content.w;
        if (ch <= 0.0f) ch = autoHeight;

        float cx = content.x;
        Alignment childAlignX = child->getAlignmentX();
        if (childAlignX == Alignment::Center) {
            cx = content.x + (content.w - cw) * 0.5f;
        } else if (childAlignX == Alignment::End) {
            cx = content.x + content.w - cw;
        }

        Rect childRect = {cx, y, cw, ch};
        child->setComputedRect(childRect);

        computeElement(*child, childRect);

        y += ch + spacing;
    }

    auto* scrollView = dynamic_cast<ScrollView*>(&element);
    if (scrollView) {
        scrollView->setContentHeight(y - content.y);
    }
}

void LayoutEngine::computeHBox(UIElement& element, const Rect& parentRect) {
    Rect content = applyPadding(parentRect, element.getPadding());
    float spacing = element.getSpacing();

    float totalFixedWidth = 0.0f;
    int visibleCount = 0;
    for (auto& child : element.getChildren()) {
        if (child->getVisibility() == Visibility::Gone) continue;
        if (!child->isVisible()) continue;

        float cw = child->getWidthValue().resolve(content.w);
        if (cw > 0.0f) totalFixedWidth += cw;
        visibleCount++;
    }

    float totalSpacing = (visibleCount > 1) ? spacing * (visibleCount - 1) : 0.0f;
    float remaining = content.w - totalFixedWidth - totalSpacing;
    float autoWidth = 0.0f;

    int autoCount = 0;
    for (auto& child : element.getChildren()) {
        if (child->getVisibility() == Visibility::Gone) continue;
        if (!child->isVisible()) continue;
        float cw = child->getWidthValue().resolve(content.w);
        if (cw <= 0.0f) autoCount++;
    }
    if (autoCount > 0 && remaining > 0.0f) {
        autoWidth = remaining / autoCount;
    }

    float x = content.x;

    Alignment parentAlignX = element.getAlignmentX();
    if (parentAlignX == Alignment::Center) {
        float totalW = totalFixedWidth + totalSpacing + autoWidth * autoCount;
        x = content.x + (content.w - totalW) * 0.5f;
    } else if (parentAlignX == Alignment::End) {
        float totalW = totalFixedWidth + totalSpacing + autoWidth * autoCount;
        x = content.x + content.w - totalW;
    }

    for (auto& child : element.getChildren()) {
        if (child->getVisibility() == Visibility::Gone) continue;
        if (!child->isVisible()) continue;

        float cw = child->getWidthValue().resolve(content.w);
        float ch = child->getHeightValue().resolve(content.h);

        if (cw <= 0.0f) cw = autoWidth;
        if (ch <= 0.0f) ch = content.h;

        float cy = content.y;
        Alignment childAlignY = child->getAlignmentY();
        if (childAlignY == Alignment::Center) {
            cy = content.y + (content.h - ch) * 0.5f;
        } else if (childAlignY == Alignment::End) {
            cy = content.y + content.h - ch;
        }

        Rect childRect = {x, cy, cw, ch};
        child->setComputedRect(childRect);

        computeElement(*child, childRect);

        x += cw + spacing;
    }
}

void LayoutEngine::computeDefault(UIElement& element, const Rect& parentRect) {
    Rect content = applyPadding(parentRect, element.getPadding());

    for (auto& child : element.getChildren()) {
        if (child->getVisibility() == Visibility::Gone) continue;
        if (!child->isVisible()) continue;

        computeElement(*child, content);
    }
}

Rect LayoutEngine::applyPadding(const Rect& rect, const Padding& padding) const {
    return {
        rect.x + padding.left,
        rect.y + padding.top,
        std::max(0.0f, rect.w - padding.left - padding.right),
        std::max(0.0f, rect.h - padding.top - padding.bottom)
    };
}

} // namespace xscreen
