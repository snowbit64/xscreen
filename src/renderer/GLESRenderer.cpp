#include "GLESRenderer.h"

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "xscreen", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "xscreen", __VA_ARGS__)
#else
#include <GLES3/gl3.h>
#include <cstdio>
#define LOGI(...) std::printf(__VA_ARGS__)
#define LOGE(...) std::fprintf(stderr, __VA_ARGS__)
#endif

#include <cstring>
#include <cmath>
#include <cstdlib>

namespace xscreen {

static const char* rectVertSrc = R"(#version 300 es
precision mediump float;
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
)";

static const char* rectFragSrc = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = uColor;
}
)";

static const char* roundedVertSrc = R"(#version 300 es
precision mediump float;
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
)";

static const char* roundedFragSrc = R"(#version 300 es
precision mediump float;
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
)";

static const char* texVertSrc = R"(#version 300 es
precision mediump float;
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
)";

static const char* texFragSrc = R"(#version 300 es
precision mediump float;
uniform sampler2D uTexture;
uniform vec4 uTint;
in vec2 vTexCoord;
out vec4 fragColor;
void main() {
    fragColor = texture(uTexture, vTexCoord) * uTint;
}
)";

static const char* fontVertSrc = R"(#version 300 es
precision mediump float;
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
)";

static const char* fontFragSrc = R"(#version 300 es
precision mediump float;
uniform sampler2D uTexture;
uniform vec4 uColor;
in vec2 vTexCoord;
out vec4 fragColor;
void main() {
    float a = texture(uTexture, vTexCoord).r;
    fragColor = vec4(uColor.rgb, uColor.a * a);
}
)";

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

GLESRenderer::GLESRenderer() = default;
GLESRenderer::~GLESRenderer() { shutdown(); }

bool GLESRenderer::init(int width, int height, const std::string& title) {
    width_ = width;
    height_ = height;
    initShaders();
    initFontAtlas();

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

void GLESRenderer::initFontAtlas() {
    const int ATLAS_W = 512;
    const int ATLAS_H = 512;
    const int FONT_SIZE = 32;
    const int CELL_W = ATLAS_W / 16;
    const int CELL_H = ATLAS_H / 8;

    std::vector<unsigned char> atlas(ATLAS_W * ATLAS_H, 0);

    FontEntry fe{};
    fe.atlasW = ATLAS_W;
    fe.atlasH = ATLAS_H;
    fe.fontSize = FONT_SIZE;

    for (int c = 32; c < 127; c++) {
        int col = (c - 32) % 16;
        int row = (c - 32) / 16;
        int cellX = col * CELL_W;
        int cellY = row * CELL_H;

        int glyphW = FONT_SIZE / 2;
        int glyphH = FONT_SIZE;

        for (int gy = 0; gy < glyphH && gy < CELL_H; gy++) {
            for (int gx = 0; gx < glyphW && gx < CELL_W; gx++) {
                int px = cellX + gx;
                int py = cellY + gy;
                if (px < ATLAS_W && py < ATLAS_H) {
                    bool pixel = false;

                    float nx = (float)gx / glyphW;
                    float ny = (float)gy / glyphH;

                    switch (c) {
                        case ' ': break;
                        default: {
                            float cx = 0.5f, cy = 0.5f;
                            float dist = std::sqrt((nx - cx) * (nx - cx) + (ny - cy) * (ny - cy));
                            pixel = dist < 0.45f;
                            break;
                        }

                        case 'A': case 'a': pixel = (ny > 0.3f || (nx > 0.3f && nx < 0.7f)) && (nx < 0.15f || nx > 0.85f || (ny > 0.45f && ny < 0.55f)); break;
                        case 'B': case 'b': pixel = nx < 0.15f || (ny < 0.1f || ny > 0.9f || (ny > 0.45f && ny < 0.55f)) && nx > 0.6f; break;
                        case 'C': case 'c': pixel = nx < 0.15f || ((ny < 0.15f || ny > 0.85f) && nx < 0.8f); break;
                        case 'D': case 'd': pixel = nx < 0.15f || (nx > 0.7f && ny > 0.15f && ny < 0.85f) || ny < 0.1f || ny > 0.9f; break;
                        case 'E': case 'e': pixel = nx < 0.15f || ny < 0.1f || ny > 0.9f || (ny > 0.45f && ny < 0.55f && nx < 0.7f); break;
                        case 'F': case 'f': pixel = nx < 0.15f || ny < 0.1f || (ny > 0.45f && ny < 0.55f && nx < 0.6f); break;
                        case 'G': case 'g': pixel = nx < 0.15f || ny < 0.1f || ny > 0.9f || (nx > 0.7f && ny > 0.5f); break;
                        case 'H': case 'h': pixel = nx < 0.15f || nx > 0.85f || (ny > 0.45f && ny < 0.55f); break;
                        case 'I': case 'i': pixel = (nx > 0.35f && nx < 0.65f) || ny < 0.1f || ny > 0.9f; break;
                        case 'J': case 'j': pixel = (nx > 0.6f && nx < 0.75f) || (ny > 0.85f && nx > 0.15f && nx < 0.75f) || (nx < 0.2f && ny > 0.7f); break;
                        case 'K': case 'k': {
                            float diag1 = std::abs(ny - 0.5f + (nx - 0.15f) * 1.0f);
                            float diag2 = std::abs(ny - 0.5f - (nx - 0.15f) * 1.0f);
                            pixel = nx < 0.15f || diag1 < 0.12f || diag2 < 0.12f;
                            break;
                        }
                        case 'L': case 'l': pixel = nx < 0.15f || ny > 0.9f; break;
                        case 'M': case 'm': pixel = nx < 0.1f || nx > 0.9f || (ny < 0.4f && (std::abs(nx - ny * 0.5f) < 0.1f || std::abs(nx - 1.0f + ny * 0.5f) < 0.1f)); break;
                        case 'N': case 'n': pixel = nx < 0.1f || nx > 0.9f || std::abs(nx - ny) < 0.12f; break;
                        case 'O': case 'o': pixel = (nx < 0.15f || nx > 0.85f) && ny > 0.1f && ny < 0.9f || (ny < 0.15f || ny > 0.85f) && nx > 0.1f && nx < 0.9f; break;
                        case 'P': case 'p': pixel = nx < 0.15f || (ny < 0.1f || (ny > 0.45f && ny < 0.55f)) && nx < 0.8f || (nx > 0.7f && ny < 0.55f && ny > 0.05f); break;
                        case 'Q': case 'q': pixel = ((nx < 0.15f || nx > 0.85f) && ny > 0.1f && ny < 0.9f) || ((ny < 0.15f || ny > 0.85f) && nx > 0.1f && nx < 0.9f) || (ny > 0.7f && std::abs(nx - ny) < 0.15f); break;
                        case 'R': case 'r': pixel = nx < 0.15f || (ny < 0.1f || (ny > 0.45f && ny < 0.55f)) && nx < 0.8f || (nx > 0.7f && ny < 0.55f) || (ny > 0.5f && std::abs(nx - (ny - 0.5f) * 1.5f - 0.15f) < 0.1f); break;
                        case 'S': case 's': pixel = (ny < 0.15f || ny > 0.85f || (ny > 0.43f && ny < 0.57f)) && (nx > 0.1f && nx < 0.9f) || (nx < 0.15f && ny < 0.5f) || (nx > 0.85f && ny > 0.5f); break;
                        case 'T': case 't': pixel = ny < 0.1f || (nx > 0.4f && nx < 0.6f); break;
                        case 'U': case 'u': pixel = (nx < 0.15f || nx > 0.85f) && ny < 0.85f || (ny > 0.85f && nx > 0.1f && nx < 0.9f); break;
                        case 'V': case 'v': pixel = std::abs(nx - 0.5f - (0.5f - ny) * 0.5f) < 0.08f || std::abs(nx - 0.5f + (0.5f - ny) * 0.5f) < 0.08f; break;
                        case 'W': case 'w': pixel = nx < 0.1f || nx > 0.9f || (ny > 0.6f && (std::abs(nx - 0.5f + (ny - 0.6f)) < 0.08f || std::abs(nx - 0.5f - (ny - 0.6f)) < 0.08f)); break;
                        case 'X': case 'x': pixel = std::abs(nx - ny) < 0.12f || std::abs(nx + ny - 1.0f) < 0.12f; break;
                        case 'Y': case 'y': pixel = (ny < 0.5f && (std::abs(nx - ny) < 0.1f || std::abs(nx + ny - 1.0f) < 0.1f)) || (ny >= 0.5f && nx > 0.4f && nx < 0.6f); break;
                        case 'Z': case 'z': pixel = ny < 0.1f || ny > 0.9f || std::abs(nx - (1.0f - ny)) < 0.12f; break;

                        case '0': pixel = ((nx < 0.15f || nx > 0.85f) && ny > 0.1f && ny < 0.9f) || ((ny < 0.15f || ny > 0.85f) && nx > 0.1f && nx < 0.9f); break;
                        case '1': pixel = (nx > 0.4f && nx < 0.6f) || ny > 0.9f || (ny < 0.15f && nx < 0.5f && nx > 0.25f); break;
                        case '2': pixel = ny < 0.1f || ny > 0.9f || (ny > 0.45f && ny < 0.55f) || (nx > 0.8f && ny < 0.5f) || (nx < 0.15f && ny > 0.5f); break;
                        case '3': pixel = (ny < 0.1f || ny > 0.9f || (ny > 0.45f && ny < 0.55f)) || (nx > 0.8f); break;
                        case '4': pixel = (nx > 0.7f) || (ny > 0.45f && ny < 0.55f) || (nx < 0.15f && ny < 0.55f); break;
                        case '5': pixel = ny < 0.1f || ny > 0.9f || (ny > 0.45f && ny < 0.55f) || (nx < 0.15f && ny < 0.5f) || (nx > 0.8f && ny > 0.5f); break;
                        case '6': pixel = ((nx < 0.15f) && ny > 0.1f) || ((ny < 0.15f || ny > 0.85f || (ny > 0.45f && ny < 0.55f)) && nx < 0.85f) || (nx > 0.8f && ny > 0.5f); break;
                        case '7': pixel = ny < 0.1f || (nx > 0.7f && ny < 0.3f) || std::abs(nx - 0.7f + (ny - 0.1f) * 0.5f) < 0.08f; break;
                        case '8': pixel = ((nx < 0.15f || nx > 0.85f) && ny > 0.1f && ny < 0.9f) || ((ny < 0.15f || ny > 0.85f || (ny > 0.45f && ny < 0.55f)) && nx > 0.1f && nx < 0.9f); break;
                        case '9': pixel = ((nx > 0.85f) && ny < 0.9f) || ((ny < 0.15f || ny > 0.85f || (ny > 0.45f && ny < 0.55f)) && nx > 0.1f) || (nx < 0.15f && ny < 0.5f); break;

                        case '.': pixel = ny > 0.85f && nx > 0.35f && nx < 0.65f; break;
                        case ',': pixel = ny > 0.8f && nx > 0.3f && nx < 0.6f; break;
                        case ':': pixel = (ny > 0.25f && ny < 0.35f && nx > 0.35f && nx < 0.65f) || (ny > 0.7f && ny < 0.8f && nx > 0.35f && nx < 0.65f); break;
                        case ';': pixel = (ny > 0.25f && ny < 0.35f && nx > 0.35f && nx < 0.65f) || (ny > 0.7f && nx > 0.3f && nx < 0.6f); break;
                        case '!': pixel = (nx > 0.4f && nx < 0.6f && ny < 0.7f) || (ny > 0.8f && nx > 0.4f && nx < 0.6f); break;
                        case '?': pixel = ny < 0.1f || (nx > 0.8f && ny < 0.5f) || (ny > 0.45f && ny < 0.55f && nx > 0.35f && nx < 0.65f) || (ny > 0.8f && nx > 0.4f && nx < 0.6f); break;
                        case '-': pixel = ny > 0.45f && ny < 0.55f && nx > 0.15f && nx < 0.85f; break;
                        case '+': pixel = (ny > 0.45f && ny < 0.55f) || (nx > 0.45f && nx < 0.55f); break;
                        case '=': pixel = (ny > 0.35f && ny < 0.45f) || (ny > 0.55f && ny < 0.65f); break;
                        case '/': pixel = std::abs(nx - (1.0f - ny)) < 0.1f; break;
                        case '\\': pixel = std::abs(nx - ny) < 0.1f; break;
                        case '(': pixel = std::abs(nx - 0.6f + std::cos((ny - 0.5f) * 3.14f) * 0.3f) < 0.08f; break;
                        case ')': pixel = std::abs(nx - 0.4f - std::cos((ny - 0.5f) * 3.14f) * 0.3f) < 0.08f; break;
                        case '[': pixel = nx < 0.3f || (ny < 0.08f || ny > 0.92f) && nx < 0.7f; break;
                        case ']': pixel = nx > 0.7f || (ny < 0.08f || ny > 0.92f) && nx > 0.3f; break;
                        case '{': pixel = std::abs(nx - 0.5f + (ny < 0.5f ? std::cos((ny - 0.25f) * 6.28f) * 0.15f : -std::cos((ny - 0.75f) * 6.28f) * 0.15f)) < 0.08f; break;
                        case '}': pixel = std::abs(nx - 0.5f - (ny < 0.5f ? std::cos((ny - 0.25f) * 6.28f) * 0.15f : -std::cos((ny - 0.75f) * 6.28f) * 0.15f)) < 0.08f; break;
                        case '_': pixel = ny > 0.9f; break;
                        case '"': pixel = ny < 0.3f && ((nx > 0.25f && nx < 0.4f) || (nx > 0.6f && nx < 0.75f)); break;
                        case '\'': pixel = ny < 0.3f && nx > 0.4f && nx < 0.6f; break;
                        case '<': pixel = std::abs(nx - 0.2f - std::abs(ny - 0.5f) * 1.2f) < 0.08f; break;
                        case '>': pixel = std::abs(nx - 0.8f + std::abs(ny - 0.5f) * 1.2f) < 0.08f; break;
                        case '#': pixel = (nx > 0.25f && nx < 0.35f) || (nx > 0.65f && nx < 0.75f) || (ny > 0.3f && ny < 0.4f) || (ny > 0.6f && ny < 0.7f); break;
                        case '%': pixel = (ny < 0.3f && nx < 0.3f && (nx - 0.15f) * (nx - 0.15f) + (ny - 0.15f) * (ny - 0.15f) < 0.02f) || (ny > 0.7f && nx > 0.7f && (nx - 0.85f) * (nx - 0.85f) + (ny - 0.85f) * (ny - 0.85f) < 0.02f) || std::abs(nx - (1.0f - ny)) < 0.08f; break;
                        case '@': pixel = ((nx < 0.15f || nx > 0.85f) && ny > 0.1f && ny < 0.9f) || ((ny < 0.15f || ny > 0.85f) && nx > 0.1f && nx < 0.9f) || (nx > 0.4f && nx < 0.7f && ny > 0.3f && ny < 0.7f && !(nx > 0.5f && nx < 0.6f && ny > 0.4f && ny < 0.6f)); break;
                        case '&': pixel = (std::abs(nx - 0.5f) + std::abs(ny - 0.3f) < 0.25f && std::abs(nx - 0.5f) + std::abs(ny - 0.3f) > 0.15f) || (ny > 0.5f && (nx < 0.15f || std::abs(nx - ny + 0.2f) < 0.1f)); break;
                        case '*': pixel = (std::abs(nx - 0.5f) < 0.05f || std::abs(ny - 0.5f) < 0.05f || std::abs(nx - ny) < 0.07f || std::abs(nx + ny - 1.0f) < 0.07f) && ny > 0.2f && ny < 0.8f; break;
                        case '$': pixel = (ny < 0.15f || ny > 0.85f || (ny > 0.43f && ny < 0.57f)) || (nx < 0.15f && ny < 0.5f) || (nx > 0.85f && ny > 0.5f) || (nx > 0.45f && nx < 0.55f); break;
                        case '^': pixel = ny < 0.35f && std::abs(nx - 0.5f) < (0.35f - ny) * 1.0f && std::abs(nx - 0.5f) > (0.25f - ny) * 1.0f; break;
                        case '~': pixel = ny > 0.4f && ny < 0.6f && std::abs(ny - 0.5f - std::sin(nx * 6.28f) * 0.05f) < 0.04f; break;
                        case '`': pixel = ny < 0.2f && nx > 0.3f && nx < 0.6f; break;
                    }

                    if (pixel) {
                        atlas[py * ATLAS_W + px] = 255;
                    }
                }
            }
        }

        fe.glyphs[c].u0 = (float)cellX / ATLAS_W;
        fe.glyphs[c].v0 = (float)cellY / ATLAS_H;
        fe.glyphs[c].u1 = (float)(cellX + glyphW) / ATLAS_W;
        fe.glyphs[c].v1 = (float)(cellY + glyphH) / ATLAS_H;
        fe.glyphs[c].width = (float)glyphW;
        fe.glyphs[c].height = (float)glyphH;
        fe.glyphs[c].bearingX = 0;
        fe.glyphs[c].bearingY = 0;
        fe.glyphs[c].advance = (float)glyphW;
    }

    unsigned int texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS_W, ATLAS_H, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());

    fe.atlasTexId = texId;
    builtinFont_ = nextFontId_++;
    fonts_[builtinFont_] = fe;
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
}

void GLESRenderer::endFrame() {
    glFlush();
}

bool GLESRenderer::shouldClose() { return shouldClose_; }
int GLESRenderer::getScreenWidth() const { return width_; }
int GLESRenderer::getScreenHeight() const { return height_; }
void GLESRenderer::setScreenSize(int w, int h) { width_ = w; height_ = h; }
void GLESRenderer::setDeltaTime(float dt) { deltaTime_ = dt; }
float GLESRenderer::getDeltaTime() const { return deltaTime_; }

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

void GLESRenderer::drawText(const std::string& text, float x, float y,
                             float fontSize, const Color4& color, FontId font) {
    FontId fid = (font != INVALID_FONT && fonts_.count(font)) ? font : builtinFont_;
    auto it = fonts_.find(fid);
    if (it == fonts_.end()) return;

    auto& fe = it->second;
    float scale = fontSize / (float)fe.fontSize;

    glUseProgram(fontProgram_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fe.atlasTexId);
    glUniform1i(glGetUniformLocation(fontProgram_, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(fontProgram_, "uScreen"), (float)width_, (float)height_);
    glUniform4f(glGetUniformLocation(fontProgram_, "uColor"), color.r, color.g, color.b, color.a);

    float cx = x;
    for (unsigned char c : text) {
        if (c >= 128 || c < 32) c = '?';
        auto& g = fe.glyphs[c];
        float gw = g.width * scale;
        float gh = g.height * scale;

        glUniform4f(glGetUniformLocation(fontProgram_, "uRect"), cx, y, gw, gh);
        glUniform4f(glGetUniformLocation(fontProgram_, "uTexRect"), g.u0, g.v0, g.u1 - g.u0, g.v1 - g.v0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cx += g.advance * scale;
    }
}

Vec2 GLESRenderer::measureText(const std::string& text, float fontSize, FontId font) {
    FontId fid = (font != INVALID_FONT && fonts_.count(font)) ? font : builtinFont_;
    auto it = fonts_.find(fid);
    if (it == fonts_.end()) return {0, fontSize};

    auto& fe = it->second;
    float scale = fontSize / (float)fe.fontSize;
    float w = 0;
    for (unsigned char c : text) {
        if (c >= 128 || c < 32) c = '?';
        w += fe.glyphs[c].advance * scale;
    }
    return {w, fontSize};
}

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

    GLenum fmt = (channels == 4) ? GL_RGBA : (channels == 3 ? GL_RGB : GL_LUMINANCE);
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

FontId GLESRenderer::loadFont(const std::string& path, int size) {
    return builtinFont_;
}

void GLESRenderer::unloadFont(FontId font) {
}

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
