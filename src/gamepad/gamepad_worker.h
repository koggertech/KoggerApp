#pragma once

#include <array>

#include <QObject>
#include <QString>
#include <QTimer>

struct SDL_Gamepad;
struct SDL_Joystick;

class GamepadWorker : public QObject
{
    Q_OBJECT

public:
    explicit GamepadWorker(QObject* parent = nullptr);
    ~GamepadWorker() override;

    void start();
    void stop();
    void setRouting(bool routing);
    void setContinuous(bool continuous);
    void attachVirtualPad();
    void detachVirtualPad();
    void setVirtualInput(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);

signals:
    void errorChanged(const QString& error);
    void readyChanged(bool ready);
    void connectionChanged(bool connected, const QString& name, const QString& typeName, int availableButtons);
    void stateChanged(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);

private:
    void poll();
    void openFirstPad();
    void closePad();
    void readState(bool force);
    void applyRouting();

    QTimer timer_;
    SDL_Gamepad* pad_ = nullptr;
    SDL_Joystick* virtualPad_ = nullptr;
    bool subsystemUp_ = false;
    bool sdlUsed_ = false;
    bool routing_ = false;
    bool continuous_ = false;
    int buttons_ = 0;
    std::array<qreal, 6> axes_{};
};
