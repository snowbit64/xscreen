#pragma once

#include <string>
#include <cstdint>
#include <cmath>

namespace xscreen {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }

    bool contains(const Vec2& point, const Vec2& size) const {
        return point.x >= x && point.x <= x + size.x &&
               point.y >= y && point.y <= y + size.y;
    }
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

struct Color4 {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    Color4() = default;
    Color4(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
};

struct Padding {
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;
};

enum class Alignment {
    Start,
    Center,
    End
};

enum class Visibility {
    Visible,
    Hidden,
    Gone
};

enum class SizeMode {
    Absolute,
    Percent
};

struct SizeValue {
    float value = 0.0f;
    SizeMode mode = SizeMode::Percent;

    float resolve(float parentSize) const {
        return (mode == SizeMode::Percent) ? value * parentSize : value;
    }
};

enum class InputAction {
    Press,
    Release,
    Move,
    Scroll
};

struct InputEvent {
    InputAction action = InputAction::Press;
    float x = 0.0f;
    float y = 0.0f;
    float scrollDelta = 0.0f;
    int button = 0;
    int key = 0;
    bool isTouch = false;
    bool isGamepad = false;
};

} // namespace xscreen
