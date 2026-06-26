#pragma once

#include "animation_spec.h"
#include "smoother.h"

// Single source of camera/scene animation timing for C++. Tune motion here, in
// one place. The QML UI keeps its own domain in qml/kqml_types/Anim.qml.

inline constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

// Finite tweens (Animator): fixed start→target over a fixed duration.
namespace anim {

inline constexpr AnimationSpec Heading { 350, QEasingCurve::OutCubic };
inline constexpr AnimationSpec Follow  { 900, QEasingCurve::OutCubic };
inline constexpr AnimationSpec MapReset{ 500, QEasingCurve::OutCubic };
inline constexpr AnimationSpec Zoom    { 220, QEasingCurve::OutCubic };
inline constexpr AnimationSpec VScale  { 280, QEasingCurve::OutCubic };

} // namespace anim

// Continuous followers (Smoother): track a moving target. {smoothTime(s), maxSpeed(units/s, 0=∞), deadband(units)}.
namespace smooth {

inline constexpr SmoothSpec FollowYaw  { 0.50f, 0.0f, 0.5f * kDegToRad };
inline constexpr SmoothSpec FollowPitch{ 0.40f, 0.0f, 0.0f };

} // namespace smooth
