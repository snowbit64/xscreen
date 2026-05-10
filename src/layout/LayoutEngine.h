#pragma once

#include "../core/UIElement.h"
#include "../core/Types.h"

namespace xscreen {

class LayoutEngine {
public:
    void compute(UIElement& root, float screenWidth, float screenHeight);

private:
    void computeElement(UIElement& element, const Rect& parentRect);
    void computeVBox(UIElement& element, const Rect& parentRect);
    void computeHBox(UIElement& element, const Rect& parentRect);
    void computeDefault(UIElement& element, const Rect& parentRect);
    void applyAlignment(UIElement& element, const Rect& parentRect);
    Rect applyPadding(const Rect& rect, const Padding& padding) const;
};

} // namespace xscreen
