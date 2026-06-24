#include "smoother.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;

float wrapPi(float a)
{
    const float twoPi = 2.0f * kPi;
    a = std::fmod(a + kPi, twoPi);
    if (a < 0.0f) {
        a += twoPi;
    }
    return a - kPi;
}

float shortestDiff(float from, float to)
{
    return wrapPi(to - from);
}

} // namespace

void Smoother::reset()
{
    velocity_ = 0.0f;
    tmr_.start();
}

float Smoother::tickDt()
{
    if (!tmr_.isValid()) {
        tmr_.start();
        return 0.016f;
    }
    double d = tmr_.restart() / 1000.0;
    if (d <= 0.0 || d > 0.5) {
        d = 0.016; // gap/first frame → assume ~60 Hz
    }
    return static_cast<float>(d);
}

float Smoother::smoothDamp(float current, float target, float dt, const SmoothSpec& spec)
{
    const float smoothTime = std::max(0.0001f, spec.smoothTime);
    const float omega = 2.0f / smoothTime;
    const float x = omega * dt;
    const float e = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x); // rational approx of exp(-x)

    float change = current - target;
    const float originalTo = target;

    if (spec.maxSpeed > 0.0f) {
        const float maxChange = spec.maxSpeed * smoothTime;
        change = std::clamp(change, -maxChange, maxChange);
    }

    const float clampedTarget = current - change;
    const float temp = (velocity_ + omega * change) * dt;
    velocity_ = (velocity_ - omega * temp) * e;
    float output = clampedTarget + (change + temp) * e;

    if ((originalTo - current > 0.0f) == (output > originalTo)) { // anti-overshoot
        output = originalTo;
        velocity_ = (output - originalTo) / dt;
    }
    return output;
}

float Smoother::step(float current, float target, const SmoothSpec& spec)
{
    const float dt = tickDt();
    if (spec.deadband > 0.0f && std::fabs(target - current) < spec.deadband) {
        velocity_ = 0.0f;
        return current;
    }
    return smoothDamp(current, target, dt, spec);
}

float Smoother::stepAngle(float current, float target, const SmoothSpec& spec)
{
    const float dt = tickDt();
    const float diff = shortestDiff(current, target);
    if (spec.deadband > 0.0f && std::fabs(diff) < spec.deadband) {
        velocity_ = 0.0f;
        return current;
    }
    return wrapPi(smoothDamp(current, current + diff, dt, spec));
}
