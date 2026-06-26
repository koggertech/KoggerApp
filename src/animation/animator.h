#pragma once

#include <functional>
#include <QHash>
#include <QObject>

#include "animation_spec.h"

class QVariantAnimation;

// Per-view animation engine. Drives eased tweens (0..1) on named channels via
// QVariantAnimation on the GUI thread. Starting a channel cancels any animation
// already running on it. Durations/easing come from animation_specs.h.
class Animator : public QObject
{
public:
    using Tick = std::function<void(qreal)>; // eased progress 0..1
    using Done = std::function<void()>;      // natural completion only (not on cancel)

    explicit Animator(QObject* parent = nullptr);

    void start(int channel, const AnimationSpec& spec, Tick onTick, Done onFinished = {});
    void cancel(int channel);
    void cancelAll();
    bool isActive(int channel) const;

private:
    QHash<int, QVariantAnimation*> channels_;
};
