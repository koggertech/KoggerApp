#pragma once

#include "animation_spec.h"

// Single source of camera/scene animation timing for C++. Tune motion here, in
// one place. The QML UI keeps its own domain in qml/kqml_types/Anim.qml.
namespace anim {

inline constexpr AnimationSpec Heading { 350, QEasingCurve::OutCubic };
inline constexpr AnimationSpec Follow  { 900, QEasingCurve::OutCubic };
inline constexpr AnimationSpec MapReset{ 500, QEasingCurve::OutCubic };
inline constexpr AnimationSpec Zoom    { 220, QEasingCurve::OutCubic };
inline constexpr AnimationSpec VScale  { 280, QEasingCurve::OutCubic };

} // namespace anim
