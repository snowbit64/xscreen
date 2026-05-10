#include "RaylibRenderer.h"
#include "raylib.h"
#include "rlgl.h"
#include <cstring>

namespace xscreen {

namespace {

::Color toRayColor(const Color4& c) {
    return {
        static_cast<unsigned char>(c.r * 255),
        static_cast<unsigned char>(c.g * 255),
        static_cast<unsigned char>(c.b * 255),
        static_cast<unsigned char>(c.a * 255)
    };
}

} // namespace

RaylibRenderer::RaylibRenderer() = default;
RaylibRenderer::~RaylibRenderer() { shutdown(); }

bool RaylibRenderer::init(int width, int height, const std::string& title) {
    width_ = width;
    height_ = height;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
    return IsWindowReady();
}

void RaylibRenderer::shutdown() {
    for (auto& [id, entry] : textures_) {
        if (entry.data) {
            auto* tex = static_cast<Texture2D*>(entry.data);
            UnloadTexture(*tex);
            delete tex;
        }
    }
    textures_.clear();

    for (auto& [id, entry] : fonts_) {
        if (entry.data) {
            auto* font = static_cast<::Font*>(entry.data);
            UnloadFont(*font);
            delete font;
        }
    }
    fonts_.clear();

    if (IsWindowReady()) CloseWindow();
}

void RaylibRenderer::beginFrame() {
    BeginDrawing();
    ClearBackground({30, 30, 35, 255});
    width_ = GetScreenWidth();
    height_ = GetScreenHeight();
}

void RaylibRenderer::endFrame() {
    EndDrawing();
}

bool RaylibRenderer::shouldClose() {
    return WindowShouldClose();
}

int RaylibRenderer::getScreenWidth() const { return width_; }
int RaylibRenderer::getScreenHeight() const { return height_; }

void RaylibRenderer::drawRect(const Rect& rect, const Color4& color) {
    DrawRectangle(
        static_cast<int>(rect.x), static_cast<int>(rect.y),
        static_cast<int>(rect.w), static_cast<int>(rect.h),
        toRayColor(color));
}

void RaylibRenderer::drawRectOutline(const Rect& rect, const Color4& color, float thickness) {
    DrawRectangleLinesEx(
        {rect.x, rect.y, rect.w, rect.h},
        thickness,
        toRayColor(color));
}

void RaylibRenderer::drawRoundedRect(const Rect& rect, const Color4& color, float radius) {
    float roundness = (rect.w > 0 && rect.h > 0)
        ? radius / (std::min(rect.w, rect.h) * 0.5f)
        : 0.0f;
    DrawRectangleRounded({rect.x, rect.y, rect.w, rect.h}, roundness, 8, toRayColor(color));
}

void RaylibRenderer::drawText(const std::string& text, float x, float y,
                              float fontSize, const Color4& color, FontId font) {
    auto it = fonts_.find(font);
    if (it != fonts_.end() && it->second.data) {
        auto* f = static_cast<::Font*>(it->second.data);
        DrawTextEx(*f, text.c_str(), {x, y}, fontSize, 1.0f, toRayColor(color));
    } else {
        DrawText(text.c_str(), static_cast<int>(x), static_cast<int>(y),
                 static_cast<int>(fontSize), toRayColor(color));
    }
}

Vec2 RaylibRenderer::measureText(const std::string& text, float fontSize, FontId font) {
    auto it = fonts_.find(font);
    if (it != fonts_.end() && it->second.data) {
        auto* f = static_cast<::Font*>(it->second.data);
        Vector2 sz = MeasureTextEx(*f, text.c_str(), fontSize, 1.0f);
        return {sz.x, sz.y};
    }
    int w = MeasureText(text.c_str(), static_cast<int>(fontSize));
    return {static_cast<float>(w), fontSize};
}

TextureId RaylibRenderer::loadTexture(const std::string& path) {
    Texture2D tex = LoadTexture(path.c_str());
    if (tex.id == 0) return INVALID_TEXTURE;
    TextureId id = nextTextureId_++;
    auto* stored = new Texture2D(tex);
    textures_[id] = {stored};
    return id;
}

void RaylibRenderer::drawTexture(TextureId tex, const Rect& dest, const Color4& tint) {
    auto it = textures_.find(tex);
    if (it == textures_.end() || !it->second.data) return;
    auto* t = static_cast<Texture2D*>(it->second.data);
    Rectangle src = {0, 0, static_cast<float>(t->width), static_cast<float>(t->height)};
    Rectangle dst = {dest.x, dest.y, dest.w, dest.h};
    DrawTexturePro(*t, src, dst, {0, 0}, 0.0f, toRayColor(tint));
}

void RaylibRenderer::unloadTexture(TextureId tex) {
    auto it = textures_.find(tex);
    if (it != textures_.end() && it->second.data) {
        auto* t = static_cast<Texture2D*>(it->second.data);
        UnloadTexture(*t);
        delete t;
        textures_.erase(it);
    }
}

FontId RaylibRenderer::loadFont(const std::string& path, int size) {
    ::Font f = LoadFontEx(path.c_str(), size, nullptr, 0);
    if (f.texture.id == 0) return INVALID_FONT;
    FontId id = nextFontId_++;
    auto* stored = new ::Font(f);
    fonts_[id] = {stored, size};
    return id;
}

void RaylibRenderer::unloadFont(FontId font) {
    auto it = fonts_.find(font);
    if (it != fonts_.end() && it->second.data) {
        auto* f = static_cast<::Font*>(it->second.data);
        UnloadFont(*f);
        delete f;
        fonts_.erase(it);
    }
}

void RaylibRenderer::setClipRect(const Rect& rect) {
    BeginScissorMode(
        static_cast<int>(rect.x), static_cast<int>(rect.y),
        static_cast<int>(rect.w), static_cast<int>(rect.h));
}

void RaylibRenderer::clearClipRect() {
    EndScissorMode();
}

float RaylibRenderer::getDeltaTime() const {
    return GetFrameTime();
}

} // namespace xscreen
