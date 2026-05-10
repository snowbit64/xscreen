#pragma once

#include "../core/UIElement.h"

namespace xscreen {

class VBoxLayout : public UIElement {
public:
    void render(Renderer& renderer) override;
    std::string getTypeName() const override;
};

} // namespace xscreen
