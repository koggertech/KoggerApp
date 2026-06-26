#pragma once

#include <QEasingCurve>

struct AnimationSpec {
    int durationMs = 250;
    QEasingCurve::Type easing = QEasingCurve::OutCubic;
};
