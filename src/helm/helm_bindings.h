#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "helm_shaping.h"

namespace helm {

/** Helm actions a gamepad input can be bound to; the order is the storage and UI order. */
enum Action : int {
    ActionThrottleForward,
    ActionThrottleReverse,
    ActionSteering,
    ActionManual,
    ActionHold,
    ActionLoiter,
    ActionAuto,
    ActionCourseHold,
    ActionRtl,
    ActionCount
};

/**
 * What an action accepts: a button, a trigger or stick half, or a stick axis. Guide is never bindable:
 * the OS takes it (Game Bar, Home), and on Windows a pad also reports it for View + Menu. LB + RB
 * together arm / disarm, so an action on a bumper acts when that bumper is released without the other.
 */
enum class ActionKind {
    Button,
    Throttle,
    Steering
};

/**
 * Input codes. 0…kButtonCount−1 is the SDL button index (bit n of GamepadMonitor::buttons).
 * A stick half is kInputStickFirst + 2·axis + (1 for the negative half), axis 0…3 = LX, LY, RX, RY;
 * for steering it names the half that steers right, so the other half of that axis steers left.
 */
constexpr int kNoInput = -1;
constexpr int kButtonCount = 21;
constexpr int kInputStickFirst = 100;
constexpr int kStickAxes = 4;
constexpr int kInputStickLast = kInputStickFirst + 2 * kStickAxes - 1;
constexpr int kInputLeftTrigger = 110;
constexpr int kInputRightTrigger = 111;

constexpr int kButtonSouth = 0;
constexpr int kButtonEast = 1;
constexpr int kButtonWest = 2;
constexpr int kButtonNorth = 3;
constexpr int kButtonGuide = 5;
constexpr int kButtonLeftShoulder = 9;
constexpr int kButtonRightShoulder = 10;

using Bindings = std::array<int, ActionCount>;

/** One gamepad sample: button bitmask, sticks −1…1 (Y down positive), triggers 0…1. */
struct PadState
{
    int buttons = 0;
    double leftX = 0.0;
    double leftY = 0.0;
    double rightX = 0.0;
    double rightY = 0.0;
    double leftTrigger = 0.0;
    double rightTrigger = 0.0;
};

/** Outcome of assign(): the actions whose input moved to the old one, and those left without input. */
struct AssignResult
{
    bool ok = false;
    std::vector<int> swapped;
    std::vector<int> cleared;
};

inline Bindings defaultBindings()
{
    Bindings b{};
    b[ActionThrottleForward] = kInputRightTrigger;
    b[ActionThrottleReverse] = kInputLeftTrigger;
    b[ActionSteering] = kInputStickFirst;
    b[ActionManual] = kButtonWest;
    b[ActionHold] = kButtonSouth;
    b[ActionLoiter] = kButtonEast;
    b[ActionAuto] = kButtonNorth;
    b[ActionCourseHold] = kButtonLeftShoulder;
    b[ActionRtl] = kButtonRightShoulder;
    return b;
}

/** Bits of the fixed arm / disarm combination: both bumpers held together. */
constexpr int kArmComboMask = (1 << kButtonLeftShoulder) | (1 << kButtonRightShoulder);

inline bool isReservedButton(int input)
{
    return input == kButtonGuide;
}

/**
 * Buttons that act this sample: presses off the bumpers, and bumper releases unless both bumpers were
 * held together since the last time neither was down (@p spent, updated here).
 */
inline int firedButtons(int previous, int buttons, bool& spent)
{
    if ((buttons & kArmComboMask) == kArmComboMask) {
        spent = true;
    }
    const int fired = (buttons & ~previous & ~kArmComboMask) | (spent ? 0 : previous & ~buttons & kArmComboMask);
    if ((buttons & kArmComboMask) == 0) {
        spent = false;
    }
    return fired;
}

inline ActionKind actionKind(int action)
{
    switch (action) {
    case ActionThrottleForward:
    case ActionThrottleReverse:
        return ActionKind::Throttle;
    case ActionSteering:
        return ActionKind::Steering;
    default:
        return ActionKind::Button;
    }
}

inline bool isStickHalf(int input)
{
    return input >= kInputStickFirst && input <= kInputStickLast;
}

inline bool isTrigger(int input)
{
    return input == kInputLeftTrigger || input == kInputRightTrigger;
}

inline bool accepts(int action, int input)
{
    if (action < 0 || action >= ActionCount) {
        return false;
    }
    switch (actionKind(action)) {
    case ActionKind::Button:
        return input >= 0 && input < kButtonCount && !isReservedButton(input);
    case ActionKind::Throttle:
        return isStickHalf(input) || isTrigger(input);
    case ActionKind::Steering:
        return isStickHalf(input);
    }
    return false;
}

/** Inputs an action holds: steering takes both halves of its stick axis. */
inline std::vector<int> occupied(int action, int input)
{
    if (input == kNoInput) {
        return {};
    }
    if (actionKind(action) == ActionKind::Steering) {
        const int first = input - ((input - kInputStickFirst) & 1);
        return { first, first + 1 };
    }
    return { input };
}

inline bool overlaps(int actionA, int inputA, int actionB, int inputB)
{
    const std::vector<int> a = occupied(actionA, inputA);
    const std::vector<int> b = occupied(actionB, inputB);
    return std::any_of(a.cbegin(), a.cend(), [&b](int x) { return std::find(b.cbegin(), b.cend(), x) != b.cend(); });
}

/** No two actions share an input and every input fits its action. */
inline bool valid(const Bindings& bindings)
{
    for (int a = 0; a < ActionCount; ++a) {
        if (bindings[a] != kNoInput && !accepts(a, bindings[a])) {
            return false;
        }
        for (int b = a + 1; b < ActionCount; ++b) {
            if (overlaps(a, bindings[a], b, bindings[b])) {
                return false;
            }
        }
    }
    return true;
}

/**
 * Binds @p input to @p action. An action that held the input takes the old input of @p action when it
 * fits and is free; otherwise it is left without input.
 */
inline AssignResult assign(Bindings& bindings, int action, int input)
{
    AssignResult result;
    if (!accepts(action, input)) {
        return result;
    }
    result.ok = true;
    const int previous = bindings[action];
    if (previous == input) {
        return result;
    }
    bindings[action] = input;

    for (int other = 0; other < ActionCount; ++other) {
        if (other == action || !overlaps(action, input, other, bindings[other])) {
            continue;
        }
        bindings[other] = kNoInput;
        bool free = accepts(other, previous);
        for (int a = 0; free && a < ActionCount; ++a) {
            free = a == other || !overlaps(other, previous, a, bindings[a]);
        }
        if (free) {
            bindings[other] = previous;
            result.swapped.push_back(other);
        }
        else {
            result.cleared.push_back(other);
        }
    }
    return result;
}

inline double stickAxis(const PadState& pad, int axis)
{
    switch (axis) {
    case 0:
        return pad.leftX;
    case 1:
        return pad.leftY;
    case 2:
        return pad.rightX;
    default:
        return pad.rightY;
    }
}

/** Component of a stick axis after a radial dead zone on that stick's vector. */
inline double stickComponent(const PadState& pad, int axis, double deadzone)
{
    const int partner = axis ^ 1;
    return radialDeadzoneX(stickAxis(pad, axis), stickAxis(pad, partner), deadzone);
}

/** 0…1 travel of a throttle input (trigger or stick half) after its dead zone. */
inline double halfValue(const PadState& pad, int input, const Tuning& tuning)
{
    if (input == kInputLeftTrigger) {
        return triggerDeadzone(pad.leftTrigger, tuning.triggerDeadzone);
    }
    if (input == kInputRightTrigger) {
        return triggerDeadzone(pad.rightTrigger, tuning.triggerDeadzone);
    }
    if (!isStickHalf(input)) {
        return 0.0;
    }
    const int offset = input - kInputStickFirst;
    const double sign = (offset & 1) ? -1.0 : 1.0;
    return std::max(0.0, sign * stickComponent(pad, offset / 2, tuning.stickDeadzone));
}

/** −1…1 steering from a stick half bound to "steer right", after the radial dead zone. */
inline double steeringValue(const PadState& pad, int input, const Tuning& tuning)
{
    if (!isStickHalf(input)) {
        return 0.0;
    }
    const int offset = input - kInputStickFirst;
    const double sign = (offset & 1) ? -1.0 : 1.0;
    return sign * stickComponent(pad, offset / 2, tuning.stickDeadzone);
}

inline int buttonMask(const Bindings& bindings, int action)
{
    const int input = bindings[action];
    return (input >= 0 && input < kButtonCount) ? (1 << input) : 0;
}

/** True when neither throttle input is past its dead zone. */
inline bool throttleReleased(const PadState& pad, const Bindings& bindings, const Tuning& tuning)
{
    return halfValue(pad, bindings[ActionThrottleForward], tuning) <= 0.0
           && halfValue(pad, bindings[ActionThrottleReverse], tuning) <= 0.0;
}

/** Steering and throttle = forward − reverse through the bound inputs, shaped and limited. */
inline Command shape(const PadState& pad, const Bindings& bindings, const Tuning& tuning)
{
    Command command;
    command.steering = expo(steeringValue(pad, bindings[ActionSteering], tuning), tuning.steeringExpo)
                       * std::clamp(tuning.steeringLimit, 0.0, 1.0);
    const double net = halfValue(pad, bindings[ActionThrottleForward], tuning) - halfValue(pad, bindings[ActionThrottleReverse], tuning);
    const double shaped = expo(net, tuning.throttleExpo);
    command.throttle = shaped >= 0.0 ? shaped * std::clamp(tuning.forwardLimit, 0.0, 1.0)
                                     : shaped * std::clamp(tuning.reverseLimit, 0.0, 1.0);
    return command;
}

} // namespace helm
