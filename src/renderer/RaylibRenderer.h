#pragma once

#include "Renderer.h"
#include <unordered_map>

namespace xscreen {

class RaylibRenderer : public Renderer {
public:
    RaylibRenderer();
    ~RaylibRenderer() override;

    bool init(int width, int height, const std::string& title) override;
    void shutdown() override;

    void beginFrame() override;
    void endFrame() override;

    bool shouldClose() override;

    int getScreenWidth() const override;
    int getScreenHeight() const override;

    void drawRect(const Rect& rect, const Color4& color) override;
    void drawRectOutline(const Rect& rect, const Color4& color, float thickness = 1.0f) override;
    void drawRoundedRect(const Rect& rect, const Color4& color, float radius) override;

    void drawText(const std::string& text, float x, float y,
                  float fontSize, const Color4& color, FontId font = INVALID_FONT) override;
    Vec2 measureText(const std::string& text, float fontSize, FontId font = INVALID_FONT) override;

    TextureId loadTexture(const std::string& path) override;
    void drawTexture(TextureId tex, const Rect& dest, const Color4& tint) override;
    void unloadTexture(TextureId tex) override;

    FontId loadFont(const std::string& path, int size) override;
    void unloadFont(FontId font) override;

    void setClipRect(const Rect& rect) override;
    void clearClipRect() override;

    float getDeltaTime() const override;

private:
    int width_ = 0;
    int height_ = 0;
    uint32_t nextTextureId_ = 1;
    uint32_t nextFontId_ = 1;

    struct TextureEntry {
        void* data = nullptr;
    };

    struct FontEntry {
        void* data = nullptr;
        int size = 0;
    };

    std::unordered_map<TextureId, TextureEntry> textures_;
    std::unordered_map<FontId, FontEntry> fonts_;
};

} // namespace xscreen
