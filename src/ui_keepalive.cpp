#include "ui_keepalive.h"

UiKeepalive::UiKeepalive(QObject* parent)
    : QObject(parent)
{
    timer_.setSingleShot(false);
    QObject::connect(&timer_, &QTimer::timeout, this, []{
        // intentionally empty — see class comment
    });
}

void UiKeepalive::start(int intervalMs)
{
    timer_.setInterval(intervalMs);
    timer_.start();
}

void UiKeepalive::stop()
{
    timer_.stop();
}
