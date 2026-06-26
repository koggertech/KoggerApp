#pragma once

#include <QElapsedTimer>

struct SmoothSpec {
    float smoothTime = 0.25f; // approx seconds to reach a (moving) target
    float maxSpeed   = 0.0f;  // units/sec cap; 0 = unlimited
    float deadband   = 0.0f;  // ignore target within this of current (units)
};

// Critically-damped follower (SmoothDamp). Tracks a moving target frame-rate
// independently, without overshoot — unlike a finite tween (see Animator), this
// has no fixed duration. Value is NOT stored: the caller passes the live current
// each step; only velocity (+ its own clock) is kept. One instance per channel.
class Smoother
{
public:
    void reset();                                                       // zero velocity + restart clock
    float step(float current, float target, const SmoothSpec& spec);    // linear scalar
    float stepAngle(float current, float target, const SmoothSpec& spec); // radians, shortest-arc

private:
    float tickDt();
    float smoothDamp(float current, float target, float dt, const SmoothSpec& spec);

    float velocity_ = 0.0f;
    QElapsedTimer tmr_;
};
