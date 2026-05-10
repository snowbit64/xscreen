#include "ImageWidget.h"

namespace xscreen {

void ImageWidget::setSource(const std::string& path) { source_ = path; loaded_ = false; }
const std::string& ImageWidget::getSource() const { return source_; }

void ImageWidget::setTint(const Color4& tint) { tint_ = tint; }
void ImageWidget::setPreserveAspect(bool preserve) { preserveAspect_ = preserve; }

void ImageWidget::render(Renderer& renderer) {
    if (!isVisible()) return;

    if (!loaded_ && !source_.empty()) {
        texture_ = renderer.loadTexture(source_);
        loaded_ = true;
    }

    if (texture_ != INVALID_TEXTURE) {
        Color4 t = tint_;
        t.a *= getEffectiveAlpha();
        renderer.drawTexture(texture_, computedRect_, t);
    }

    UIElement::render(renderer);
}

std::string ImageWidget::getTypeName() const { return "Image"; }

} // namespace xscreen
