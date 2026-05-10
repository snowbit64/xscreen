#include "HBoxLayout.h"
#include "../renderer/Renderer.h"

namespace xscreen {

void HBoxLayout::render(Renderer& renderer) {
    if (!isVisible()) return;
    UIElement::render(renderer);
}

std::string HBoxLayout::getTypeName() const { return "HBox"; }

} // namespace xscreen
