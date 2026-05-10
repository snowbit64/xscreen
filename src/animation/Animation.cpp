#include "Animation.h"
#include <cmath>
#include <algorithm>

namespace xscreen {

Animation::Animation() = default;

void Animation::setTarget(std::shared_ptr<UIElement> target) { target_ = std::move(target); }
void Animation::setDuration(float duration) { duration_ = duration; }
void Animation::setDelay(float delay) { delay_ = delay; }
void Animation::setEase(EaseFunction ease) { easeFunc_ = ease; }
void Animation::setLoop(bool loop) { loop_ = loop; }

void Animation::start() {
    elapsed_ = -delay_;
    playing_ = true;
    finished_ = false;
}

void Animation::stop() {
    playing_ = false;
    finished_ = true;
}

void Animation::pause() { playing_ = false; }
void Animation::resume() { if (!finished_) playing_ = true; }

bool Animation::isPlaying() const { return playing_; }
bool Animation::isFinished() const { return finished_; }

void Animation::update(float dt) {
    if (!playing_ || finished_) return;

    elapsed_ += dt;

    if (elapsed_ < 0.0f) return;

    float t = (duration_ > 0.0f) ? std::clamp(elapsed_ / duration_, 0.0f, 1.0f) : 1.0f;
    float eased = ease(t);

    apply(eased);

    if (t >= 1.0f) {
        if (loop_) {
            elapsed_ = 0.0f;
        } else {
            finished_ = true;
            playing_ = false;
            if (onComplete_) onComplete_();
        }
    }
}

void Animation::setOnComplete(std::function<void()> callback) {
    onComplete_ = std::move(callback);
}

float Animation::ease(float t) const {
    switch (easeFunc_) {
        case EaseFunction::EaseIn:
            return t * t;
        case EaseFunction::EaseOut:
            return t * (2.0f - t);
        case EaseFunction::EaseInOut:
            return (t < 0.5f) ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        default:
            return t;
    }
}

void FadeAnimation::setFromAlpha(float from) { fromAlpha_ = from; }
void FadeAnimation::setToAlpha(float to) { toAlpha_ = to; }

void FadeAnimation::apply(float t) {
    if (target_) {
        target_->setAlpha(fromAlpha_ + (toAlpha_ - fromAlpha_) * t);
    }
}

void SlideAnimation::setFromPosition(const Vec2& from) { fromPos_ = from; }
void SlideAnimation::setToPosition(const Vec2& to) { toPos_ = to; }

void SlideAnimation::apply(float t) {
    if (target_) {
        Vec2 pos = {
            fromPos_.x + (toPos_.x - fromPos_.x) * t,
            fromPos_.y + (toPos_.y - fromPos_.y) * t
        };
        target_->setPosition(pos);
    }
}

void ScaleAnimation::setFromScale(const Vec2& from) { fromScale_ = from; }
void ScaleAnimation::setToScale(const Vec2& to) { toScale_ = to; }

void ScaleAnimation::apply(float t) {
    if (target_) {
        Vec2 scale = {
            fromScale_.x + (toScale_.x - fromScale_.x) * t,
            fromScale_.y + (toScale_.y - fromScale_.y) * t
        };
        target_->setSize(scale);
    }
}

} // namespace xscreen
