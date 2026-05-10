#pragma once

#include "../core/UIElement.h"
#include <string>
#include <memory>
#include <functional>

namespace xscreen {

enum class AnimationType {
    Fade,
    Slide,
    Scale,
    Custom
};

enum class EaseFunction {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut
};

class Animation {
public:
    Animation();
    virtual ~Animation() = default;

    void setTarget(std::shared_ptr<UIElement> target);
    void setDuration(float duration);
    void setDelay(float delay);
    void setEase(EaseFunction ease);
    void setLoop(bool loop);

    void start();
    void stop();
    void pause();
    void resume();

    bool isPlaying() const;
    bool isFinished() const;

    void update(float dt);

    void setOnComplete(std::function<void()> callback);

protected:
    virtual void apply(float t) = 0;

    float ease(float t) const;

    std::shared_ptr<UIElement> target_;
    float duration_ = 0.5f;
    float delay_ = 0.0f;
    float elapsed_ = 0.0f;
    EaseFunction easeFunc_ = EaseFunction::EaseInOut;
    bool playing_ = false;
    bool finished_ = false;
    bool loop_ = false;
    std::function<void()> onComplete_;
};

class FadeAnimation : public Animation {
public:
    void setFromAlpha(float from);
    void setToAlpha(float to);

protected:
    void apply(float t) override;

private:
    float fromAlpha_ = 0.0f;
    float toAlpha_ = 1.0f;
};

class SlideAnimation : public Animation {
public:
    void setFromPosition(const Vec2& from);
    void setToPosition(const Vec2& to);

protected:
    void apply(float t) override;

private:
    Vec2 fromPos_;
    Vec2 toPos_;
};

class ScaleAnimation : public Animation {
public:
    void setFromScale(const Vec2& from);
    void setToScale(const Vec2& to);

protected:
    void apply(float t) override;

private:
    Vec2 fromScale_ = {1.0f, 1.0f};
    Vec2 toScale_ = {1.0f, 1.0f};
};

} // namespace xscreen
