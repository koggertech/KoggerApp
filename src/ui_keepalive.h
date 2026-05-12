#pragma once

#include <QObject>
#include <QTimer>


class UiKeepalive : public QObject
{
    Q_OBJECT
public:
    explicit UiKeepalive(QObject* parent = nullptr);

    // Default: ~60 FPS (16 ms). Call once after QGuiApplication is ready.
    void start(int intervalMs = 16);
    void stop();

private:
    QTimer timer_;
};
