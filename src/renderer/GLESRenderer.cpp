#include "GLESRenderer.h"

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "xscreen", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "xscreen", __VA_ARGS__)
#else
#include <glad/gl.h>
#include <cstdio>
#define LOGI(...) std::printf(__VA_ARGS__)
#define LOGE(...) std::fprintf(stderr, __VA_ARGS__)
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <cstring>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <algorithm>

namespace xscreen {

// ─── Shaders ────────────────────────────────────────────────
// Shader bodies shared between GLES 3.0 (Android) and GL 3.3 (Desktop).
// SHADER_HEADER is prepended at compile time.

#ifdef __ANDROID__
#define SH(body) "#version 300 es\nprecision mediump float;\n" body
#else
#define SH(body) "#version 330 core\n" body
#endif

static const char* rectVertSrc = SH(R"(
uniform vec4 uRect;
uniform vec2 uScreen;
const vec2 pos[6] = vec2[6](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
    vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0)
);
void main() {
    vec2 p = pos[gl_VertexID];
    vec2 px = uRect.xy + p * uRect.zw;
    vec2 ndc = (px / uScreen) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)");

static const char* rectFragSrc = SH(R"(
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = uColor;
}
)");

static const char* roundedVertSrc = SH(R"(
uniform vec4 uRect;
uniform vec2 uScreen;
out vec2 vLocalPos;
const vec2 pos[6] = vec2[6](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
    vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0)
);
void main() {
    vec2 p = pos[gl_VertexID];
    vLocalPos = p * uRect.zw;
    vec2 px = uRect.xy + vLocalPos;
    vec2 ndc = (px / uScreen) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)");

static const char* roundedFragSrc = SH(R"(
uniform vec4 uColor;
uniform vec4 uRect;
uniform float uRadius;
in vec2 vLocalPos;
out vec4 fragColor;
void main() {
    vec2 sz = uRect.zw;
    float r = min(uRadius, min(sz.x, sz.y) * 0.5);
    vec2 p = vLocalPos;
    vec2 q = abs(p - sz * 0.5) - (sz * 0.5 - vec2(r));
    float d = length(max(q, 0.0)) - r;
    float a = 1.0 - smoothstep(-1.0, 1.0, d);
    fragColor = vec4(uColor.rgb, uColor.a * a);
}
)");

static const char* texVertSrc = SH(R"(
uniform vec4 uRect;
uniform vec2 uScreen;
out vec2 vTexCoord;
const vec2 pos[6] = vec2[6](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
    vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0)
);
void main() {
    vec2 p = pos[gl_VertexID];
    vTexCoord = vec2(p.x, p.y);
    vec2 px = uRect.xy + p * uRect.zw;
    vec2 ndc = (px / uScreen) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)");

static const char* texFragSrc = SH(R"(
uniform sampler2D uTexture;
uniform vec4 uTint;
in vec2 vTexCoord;
out vec4 fragColor;
void main() {
    fragColor = texture(uTexture, vTexCoord) * uTint;
}
)");

static const char* fontVertSrc = SH(R"(
uniform vec4 uRect;
uniform vec2 uScreen;
uniform vec4 uTexRect;
out vec2 vTexCoord;
const vec2 pos[6] = vec2[6](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
    vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0)
);
void main() {
    vec2 p = pos[gl_VertexID];
    vTexCoord = uTexRect.xy + p * uTexRect.zw;
    vec2 px = uRect.xy + p * uRect.zw;
    vec2 ndc = (px / uScreen) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)");

static const char* fontFragSrc = SH(R"(
uniform sampler2D uTexture;
uniform vec4 uColor;
in vec2 vTexCoord;
out vec4 fragColor;
void main() {
    float a = texture(uTexture, vTexCoord).r;
    fragColor = vec4(uColor.rgb, uColor.a * a);
}
)");

// ─── Shader Helpers ─────────────────────────────────────────

static unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        LOGE("Shader compile error: %s\n", log);
    }
    return s;
}

static unsigned int linkProgram(const char* vertSrc, const char* fragSrc) {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        LOGE("Program link error: %s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ─── GLESRenderer Implementation ────────────────────────────

GLESRenderer::GLESRenderer() = default;
GLESRenderer::~GLESRenderer() { shutdown(); }

bool GLESRenderer::init(int width, int height, const std::string& title) {
    width_ = width;
    height_ = height;
    initShaders();

    glGenVertexArrays(1, &dummyVAO_);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    LOGI("GLESRenderer initialized: %dx%d\n", width_, height_);
    return true;
}

void GLESRenderer::initShaders() {
    rectProgram_ = linkProgram(rectVertSrc, rectFragSrc);
    roundedProgram_ = linkProgram(roundedVertSrc, roundedFragSrc);
    texProgram_ = linkProgram(texVertSrc, texFragSrc);
    fontProgram_ = linkProgram(fontVertSrc, fontFragSrc);
}

void GLESRenderer::shutdown() {
    for (auto& [id, entry] : textures_) {
        if (entry.glId) glDeleteTextures(1, &entry.glId);
    }
    textures_.clear();

    for (auto& [id, entry] : fonts_) {
        if (entry.atlasTexId) glDeleteTextures(1, &entry.atlasTexId);
    }
    fonts_.clear();
    defaultFont_ = INVALID_FONT;

    if (dummyVAO_) { glDeleteVertexArrays(1, &dummyVAO_); dummyVAO_ = 0; }
    if (rectProgram_) glDeleteProgram(rectProgram_);
    if (roundedProgram_) glDeleteProgram(roundedProgram_);
    if (texProgram_) glDeleteProgram(texProgram_);
    if (fontProgram_) glDeleteProgram(fontProgram_);
    rectProgram_ = roundedProgram_ = texProgram_ = fontProgram_ = 0;
}

void GLESRenderer::beginFrame() {
    glViewport(0, 0, width_, height_);
    glClearColor(0.118f, 0.118f, 0.137f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(dummyVAO_);
}

void GLESRenderer::endFrame() {
    glBindVertexArray(0);
    glFlush();
}

bool GLESRenderer::shouldClose() { return shouldClose_; }
int GLESRenderer::getScreenWidth() const { return width_; }
int GLESRenderer::getScreenHeight() const { return height_; }
void GLESRenderer::setScreenSize(int w, int h) { width_ = w; height_ = h; }
void GLESRenderer::setDeltaTime(float dt) { deltaTime_ = dt; }
float GLESRenderer::getDeltaTime() const { return deltaTime_; }

// ─── Drawing Primitives ─────────────────────────────────────

void GLESRenderer::drawRectInternal(float x, float y, float w, float h,
                                     float r, float g, float b, float a) {
    glUseProgram(rectProgram_);
    glUniform4f(glGetUniformLocation(rectProgram_, "uRect"), x, y, w, h);
    glUniform2f(glGetUniformLocation(rectProgram_, "uScreen"), (float)width_, (float)height_);
    glUniform4f(glGetUniformLocation(rectProgram_, "uColor"), r, g, b, a);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void GLESRenderer::drawRect(const Rect& rect, const Color4& color) {
    drawRectInternal(rect.x, rect.y, rect.w, rect.h, color.r, color.g, color.b, color.a);
}

void GLESRenderer::drawRectOutline(const Rect& rect, const Color4& color, float thickness) {
    drawRectInternal(rect.x, rect.y, rect.w, thickness, color.r, color.g, color.b, color.a);
    drawRectInternal(rect.x, rect.y + rect.h - thickness, rect.w, thickness, color.r, color.g, color.b, color.a);
    drawRectInternal(rect.x, rect.y, thickness, rect.h, color.r, color.g, color.b, color.a);
    drawRectInternal(rect.x + rect.w - thickness, rect.y, thickness, rect.h, color.r, color.g, color.b, color.a);
}

void GLESRenderer::drawRoundedRect(const Rect& rect, const Color4& color, float radius) {
    if (radius <= 0.5f) {
        drawRect(rect, color);
        return;
    }
    glUseProgram(roundedProgram_);
    glUniform4f(glGetUniformLocation(roundedProgram_, "uRect"), rect.x, rect.y, rect.w, rect.h);
    glUniform2f(glGetUniformLocation(roundedProgram_, "uScreen"), (float)width_, (float)height_);
    glUniform4f(glGetUniformLocation(roundedProgram_, "uColor"), color.r, color.g, color.b, color.a);
    glUniform1f(glGetUniformLocation(roundedProgram_, "uRadius"), radius);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// ─── Font Loading with stb_truetype ─────────────────────────

FontId GLESRenderer::createFontAtlas(const unsigned char* fontData, int dataSize, int pixelSize) {
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, fontData, stbtt_GetFontOffsetForIndex(fontData, 0))) {
        LOGE("stb_truetype: failed to init font\n");
        return INVALID_FONT;
    }

    float scale = stbtt_ScaleForPixelHeight(&fontInfo, static_cast<float>(pixelSize));

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    const int FIRST_CHAR = 32;
    const int NUM_CHARS = 224;
    int atlasW = 1024;
    int atlasH = 1024;

    if (pixelSize > 48) {
        atlasW = 2048;
        atlasH = 2048;
    }

    std::vector<unsigned char> atlasBitmap(atlasW * atlasH, 0);
    std::vector<stbtt_bakedchar> charData(NUM_CHARS);

    int result = stbtt_BakeFontBitmap(fontData, 0, static_cast<float>(pixelSize),
                                       atlasBitmap.data(), atlasW, atlasH,
                                       FIRST_CHAR, NUM_CHARS, charData.data());
    if (result <= 0) {
        LOGE("stb_truetype: BakeFontBitmap failed (result=%d), trying larger atlas\n", result);
        atlasW = 2048;
        atlasH = 2048;
        atlasBitmap.resize(atlasW * atlasH, 0);
        result = stbtt_BakeFontBitmap(fontData, 0, static_cast<float>(pixelSize),
                                       atlasBitmap.data(), atlasW, atlasH,
                                       FIRST_CHAR, NUM_CHARS, charData.data());
        if (result <= 0) {
            LOGE("stb_truetype: BakeFontBitmap failed even with 2048 atlas\n");
            return INVALID_FONT;
        }
    }

    unsigned int texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasW, atlasH, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlasBitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    FontEntry fe{};
    fe.atlasTexId = texId;
    fe.atlasW = atlasW;
    fe.atlasH = atlasH;
    fe.fontSize = pixelSize;
    fe.ascent = ascent * scale;
    fe.descent = descent * scale;
    fe.lineGap = lineGap * scale;

    for (int i = 0; i < NUM_CHARS; i++) {
        int c = FIRST_CHAR + i;
        if (c >= 256) break;
        auto& bc = charData[i];
        auto& g = fe.glyphs[c];

        g.u0 = static_cast<float>(bc.x0) / atlasW;
        g.v0 = static_cast<float>(bc.y0) / atlasH;
        g.u1 = static_cast<float>(bc.x1) / atlasW;
        g.v1 = static_cast<float>(bc.y1) / atlasH;
        g.width = static_cast<float>(bc.x1 - bc.x0);
        g.height = static_cast<float>(bc.y1 - bc.y0);
        g.bearingX = bc.xoff;
        g.bearingY = bc.yoff;
        g.advance = bc.xadvance;
    }

    FontId fid = nextFontId_++;
    fe.fontData.assign(fontData, fontData + dataSize);
    fonts_[fid] = std::move(fe);

    LOGI("Font loaded: id=%u, size=%dpx, atlas=%dx%d\n", fid, pixelSize, atlasW, atlasH);
    return fid;
}

FontId GLESRenderer::loadFont(const std::string& path, int size) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOGE("Failed to open font file: %s\n", path.c_str());
        return INVALID_FONT;
    }

    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    return createFontAtlas(data.data(), static_cast<int>(data.size()), size);
}

FontId GLESRenderer::loadFontFromMemory(const unsigned char* data, int dataSize, int pixelSize) {
    if (!data || dataSize <= 0) return INVALID_FONT;
    return createFontAtlas(data, dataSize, pixelSize);
}

void GLESRenderer::unloadFont(FontId font) {
    auto it = fonts_.find(font);
    if (it != fonts_.end()) {
        if (it->second.atlasTexId) glDeleteTextures(1, &it->second.atlasTexId);
        fonts_.erase(it);
        if (defaultFont_ == font) defaultFont_ = INVALID_FONT;
    }
}

// ─── Text Rendering ─────────────────────────────────────────

void GLESRenderer::drawText(const std::string& text, float x, float y,
                             float fontSize, const Color4& color, FontId font) {
    FontId fid = (font != INVALID_FONT && fonts_.count(font)) ? font : defaultFont_;
    auto it = fonts_.find(fid);
    if (it == fonts_.end()) return;

    auto& fe = it->second;
    float scale = fontSize / static_cast<float>(fe.fontSize);

    glUseProgram(fontProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fe.atlasTexId);
    glUniform1i(glGetUniformLocation(fontProgram_, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(fontProgram_, "uScreen"), (float)width_, (float)height_);
    glUniform4f(glGetUniformLocation(fontProgram_, "uColor"), color.r, color.g, color.b, color.a);

    float baseline = y + fe.ascent * scale;
    float cx = x;

    for (unsigned char c : text) {
        if (c < 32 || c >= 256) c = '?';
        auto& g = fe.glyphs[c];
        if (g.width <= 0 && g.height <= 0) {
            cx += g.advance * scale;
            continue;
        }

        float gx = cx + g.bearingX * scale;
        float gy = baseline + g.bearingY * scale;
        float gw = g.width * scale;
        float gh = g.height * scale;

        glUniform4f(glGetUniformLocation(fontProgram_, "uRect"), gx, gy, gw, gh);
        glUniform4f(glGetUniformLocation(fontProgram_, "uTexRect"),
                    g.u0, g.v0, g.u1 - g.u0, g.v1 - g.v0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cx += g.advance * scale;
    }
}

Vec2 GLESRenderer::measureText(const std::string& text, float fontSize, FontId font) {
    FontId fid = (font != INVALID_FONT && fonts_.count(font)) ? font : defaultFont_;
    auto it = fonts_.find(fid);
    if (it == fonts_.end()) return {0, fontSize};

    auto& fe = it->second;
    float scale = fontSize / static_cast<float>(fe.fontSize);
    float w = 0;
    for (unsigned char c : text) {
        if (c < 32 || c >= 256) c = '?';
        w += fe.glyphs[c].advance * scale;
    }
    return {w, fontSize};
}

// ─── Textures ───────────────────────────────────────────────

TextureId GLESRenderer::loadTexture(const std::string& path) {
    return INVALID_TEXTURE;
}

TextureId GLESRenderer::loadTextureFromMemory(const unsigned char* data, int width, int height, int channels) {
    if (!data) return INVALID_TEXTURE;
    unsigned int texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#ifdef __ANDROID__
    GLenum fmt = (channels == 4) ? GL_RGBA : (channels == 3 ? GL_RGB : GL_LUMINANCE);
#else
    GLenum fmt = (channels == 4) ? GL_RGBA : (channels == 3 ? GL_RGB : GL_RED);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, data);

    TextureId id = nextTextureId_++;
    textures_[id] = {texId, width, height};
    return id;
}

void GLESRenderer::drawTexture(TextureId tex, const Rect& dest, const Color4& tint) {
    auto it = textures_.find(tex);
    if (it == textures_.end()) return;

    glUseProgram(texProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, it->second.glId);
    glUniform1i(glGetUniformLocation(texProgram_, "uTexture"), 0);
    glUniform4f(glGetUniformLocation(texProgram_, "uRect"), dest.x, dest.y, dest.w, dest.h);
    glUniform2f(glGetUniformLocation(texProgram_, "uScreen"), (float)width_, (float)height_);
    glUniform4f(glGetUniformLocation(texProgram_, "uTint"), tint.r, tint.g, tint.b, tint.a);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void GLESRenderer::unloadTexture(TextureId tex) {
    auto it = textures_.find(tex);
    if (it != textures_.end()) {
        if (it->second.glId) glDeleteTextures(1, &it->second.glId);
        textures_.erase(it);
    }
}

// ─── Clipping ───────────────────────────────────────────────

void GLESRenderer::setClipRect(const Rect& rect) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(
        static_cast<int>(rect.x),
        height_ - static_cast<int>(rect.y + rect.h),
        static_cast<int>(rect.w),
        static_cast<int>(rect.h));
}

void GLESRenderer::clearClipRect() {
    glDisable(GL_SCISSOR_TEST);
}

} // namespace xscreen
