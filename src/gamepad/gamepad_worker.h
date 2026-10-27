#pragma once

#include <array>

#include <QObject>
#include <QString>
#include <QTimer>

struct SDL_Gamepad;

class GamepadWorker : public QObject
{
    Q_OBJECT

public:
    explicit GamepadWorker(QObject* parent = nullptr);
    ~GamepadWorker() override;

    void start();
    void stop();

signals:
    void errorChanged(const QString& error);
    void connectionChanged(bool connected, const QString& name, const QString& typeName, int availableButtons);
    void stateChanged(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);

private:
    void poll();
    void openFirstPad();
    void closePad();
    void readState(bool force);

    QTimer timer_;
    SDL_Gamepad* pad_ = nullptr;
    bool subsystemUp_ = false;
    bool sdlUsed_ = false;
    int buttons_ = 0;
    std::array<qreal, 6> axes_{};
};
