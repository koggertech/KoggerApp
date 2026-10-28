#pragma once

#include <algorithm>
#include <cmath>

namespace helm {

constexpr double kAxisRange = 1000.0;
constexpr double kMaxDeadzone = 0.9;

/** Shaping knobs of the gamepad helm; limits and dead zones are fractions of full travel. */
struct Tuning
{
    double stickDeadzone = 0.10;
    double triggerDeadzone = 0.05;
    double throttleExpo = 0.2;
    double steeringExpo = 0.0;
    double forwardLimit = 1.0;
    double reverseLimit = 0.5;
};

/** One helm output: steering and throttle in −1…1, positive = right / forward. */
struct Command
{
    double steering = 0.0;
    double throttle = 0.0;
};

/** Component @p x of a stick vector (x, y) after a radial dead zone, rescaled to start from 0 at the zone edge. */
inline double radialDeadzoneX(double x, double y, double deadzone)
{
    const double zone = std::clamp(deadzone, 0.0, kMaxDeadzone);
    const double magnitude = std::hypot(x, y);
    if (magnitude <= zone) {
        return 0.0;
    }
    const double scaled = std::min(1.0, (magnitude - zone) / (1.0 - zone));
    return std::clamp(x / magnitude * scaled, -1.0, 1.0);
}

/** A 0…1 trigger after a dead zone, rescaled to start from 0 at the zone edge. */
inline double triggerDeadzone(double value, double deadzone)
{
    const double zone = std::clamp(deadzone, 0.0, kMaxDeadzone);
    if (value <= zone) {
        return 0.0;
    }
    return std::clamp((value - zone) / (1.0 - zone), 0.0, 1.0);
}

/** Cubic expo: 0 = linear, 1 = pure cube. */
inline double expo(double value, double amount)
{
    const double a = std::clamp(amount, 0.0, 1.0);
    return (1.0 - a) * value + a * value * value * value;
}

/** MANUAL_CONTROL value that lands on RCn_TRIM, given the channel's MIN / TRIM / MAX. */
inline double neutralFromRc(double min, double trim, double max)
{
    if (!(max > min)) {
        return 0.0;
    }
    return std::clamp(2.0 * kAxisRange * (trim - min) / (max - min) - kAxisRange, -kAxisRange, kAxisRange);
}

/** Maps −1…1 onto −1000…1000 so that 0 lands on @p neutral and ±1 on the range ends. */
inline int toAxis(double value, double neutral)
{
    const double v = std::clamp(value, -1.0, 1.0);
    const double out = v >= 0.0 ? neutral + v * (kAxisRange - neutral) : neutral + v * (neutral + kAxisRange);
    return int(std::lround(out));
}

} // namespace helm
