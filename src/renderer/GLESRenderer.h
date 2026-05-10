#pragma once

#include "Renderer.h"
#include <unordered_map>
#include <vector>

namespace xscreen {

class GLESRenderer : public Renderer {
public:
    GLESRenderer();
    ~GLESRenderer() override;

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

    void setScreenSize(int w, int h);
    void setDeltaTime(float dt);

    TextureId loadTextureFromMemory(const unsigned char* data, int width, int height, int channels);

private:
    void initShaders();
    void initFontAtlas();
    void drawRectInternal(float x, float y, float w, float h,
                          float r, float g, float b, float a);
    void drawRoundedRectInternal(float x, float y, float w, float h,
                                 float r, float g, float b, float a, float radius);

    int width_ = 0;
    int height_ = 0;
    float deltaTime_ = 0.016f;
    bool shouldClose_ = false;

    unsigned int rectProgram_ = 0;
    unsigned int roundedProgram_ = 0;
    unsigned int texProgram_ = 0;
    unsigned int fontProgram_ = 0;

    uint32_t nextTextureId_ = 1;
    uint32_t nextFontId_ = 1;

    struct TextureEntry {
        unsigned int glId = 0;
        int texW = 0;
        int texH = 0;
    };

    struct GlyphInfo {
        float u0, v0, u1, v1;
        float width, height;
        float bearingX, bearingY;
        float advance;
    };

    struct FontEntry {
        unsigned int atlasTexId = 0;
        int atlasW = 0;
        int atlasH = 0;
        int fontSize = 0;
        GlyphInfo glyphs[128];
    };

    std::unordered_map<TextureId, TextureEntry> textures_;
    std::unordered_map<FontId, FontEntry> fonts_;

    FontId builtinFont_ = INVALID_FONT;
};

} // namespace xscreen
