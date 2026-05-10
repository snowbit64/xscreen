#pragma once

#include "../core/UIElement.h"
#include "../renderer/Renderer.h"

namespace xscreen {

class ImageWidget : public UIElement {
public:
    void setSource(const std::string& path);
    const std::string& getSource() const;

    void setTint(const Color4& tint);
    void setPreserveAspect(bool preserve);

    void render(Renderer& renderer) override;
    std::string getTypeName() const override;

private:
    std::string source_;
    Color4 tint_ = {1.0f, 1.0f, 1.0f, 1.0f};
    TextureId texture_ = INVALID_TEXTURE;
    bool loaded_ = false;
    bool preserveAspect_ = true;
};

} // namespace xscreen
