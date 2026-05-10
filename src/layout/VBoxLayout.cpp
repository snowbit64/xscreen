#include "VBoxLayout.h"
#include "../renderer/Renderer.h"

namespace xscreen {

void VBoxLayout::render(Renderer& renderer) {
    if (!isVisible()) return;
    UIElement::render(renderer);
}

std::string VBoxLayout::getTypeName() const { return "VBox"; }

} // namespace xscreen
