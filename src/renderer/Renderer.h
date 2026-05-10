#pragma once

#include "../core/Types.h"
#include <string>
#include <cstdint>

namespace xscreen {

using TextureId = uint32_t;
using FontId = uint32_t;

constexpr TextureId INVALID_TEXTURE = 0;
constexpr FontId INVALID_FONT = 0;

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool init(int width, int height, const std::string& title) = 0;
    virtual void shutdown() = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual bool shouldClose() = 0;

    virtual int getScreenWidth() const = 0;
    virtual int getScreenHeight() const = 0;

    virtual void drawRect(const Rect& rect, const Color4& color) = 0;
    virtual void drawRectOutline(const Rect& rect, const Color4& color, float thickness = 1.0f) = 0;
    virtual void drawRoundedRect(const Rect& rect, const Color4& color, float radius) = 0;

    virtual void drawText(const std::string& text, float x, float y,
                          float fontSize, const Color4& color, FontId font = INVALID_FONT) = 0;
    virtual Vec2 measureText(const std::string& text, float fontSize, FontId font = INVALID_FONT) = 0;

    virtual TextureId loadTexture(const std::string& path) = 0;
    virtual void drawTexture(TextureId tex, const Rect& dest, const Color4& tint) = 0;
    virtual void unloadTexture(TextureId tex) = 0;

    virtual FontId loadFont(const std::string& path, int size) = 0;
    virtual void unloadFont(FontId font) = 0;

    virtual void setClipRect(const Rect& rect) = 0;
    virtual void clearClipRect() = 0;

    virtual float getDeltaTime() const = 0;
};

} // namespace xscreen
