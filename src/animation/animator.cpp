#include "animator.h"

#include <QVariantAnimation>

Animator::Animator(QObject* parent) :
    QObject(parent)
{}

void Animator::start(int channel, const AnimationSpec& spec, Tick onTick, Done onFinished)
{
    cancel(channel);

    auto* anim = new QVariantAnimation(this);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setDuration(spec.durationMs);
    anim->setEasingCurve(spec.easing);

    if (onTick) {
        QObject::connect(anim, &QVariantAnimation::valueChanged, this,
                         [onTick](const QVariant& value) -> void {
            onTick(value.toDouble());
        });
    }

    QObject::connect(anim, &QVariantAnimation::finished, this,
                     [this, channel, anim, onFinished]() -> void {
        if (channels_.value(channel) == anim) {
            channels_.remove(channel);
        }
        anim->deleteLater();
        if (onFinished) {
            onFinished();
        }
    });

    channels_.insert(channel, anim);
    anim->start();
}

void Animator::cancel(int channel)
{
    auto it = channels_.find(channel);
    if (it == channels_.end()) {
        return;
    }

    QVariantAnimation* anim = it.value();
    channels_.erase(it);
    if (anim) {
        anim->stop();
        anim->deleteLater();
    }
}

void Animator::cancelAll()
{
    const auto anims = channels_.values();
    channels_.clear();
    for (auto* anim : anims) {
        if (anim) {
            anim->stop();
            anim->deleteLater();
        }
    }
}

bool Animator::isActive(int channel) const
{
    return channels_.contains(channel);
}
