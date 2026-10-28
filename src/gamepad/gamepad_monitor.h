#pragma once

#include <array>

#include <QObject>
#include <QString>
#include <QThread>

class GamepadWorker;

class GamepadMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool supported READ supported CONSTANT)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
    Q_PROPERTY(QString name READ name NOTIFY connectionChanged)
    Q_PROPERTY(QString typeName READ typeName NOTIFY connectionChanged)
    Q_PROPERTY(int availableButtons READ availableButtons NOTIFY connectionChanged)
    Q_PROPERTY(int buttons READ buttons NOTIFY stateChanged)
    Q_PROPERTY(qreal leftX READ leftX NOTIFY stateChanged)
    Q_PROPERTY(qreal leftY READ leftY NOTIFY stateChanged)
    Q_PROPERTY(qreal rightX READ rightX NOTIFY stateChanged)
    Q_PROPERTY(qreal rightY READ rightY NOTIFY stateChanged)
    Q_PROPERTY(qreal leftTrigger READ leftTrigger NOTIFY stateChanged)
    Q_PROPERTY(qreal rightTrigger READ rightTrigger NOTIFY stateChanged)

public:
    explicit GamepadMonitor(QObject* parent = nullptr);
    ~GamepadMonitor() override;

    bool supported() const;
    /** Preview owner (the test dialog): live input routed to SDL while true. */
    bool active() const { return active_; }
    void setActive(bool active);
    /** True while any owner keeps the SDL gamepad subsystem up. */
    bool running() const { return running_; }
    /** True once the gamepad thread has the SDL gamepad subsystem up; false again after it quit. */
    bool ready() const { return ready_; }
    QString error() const { return error_; }

    /** Presence owner: SDL watches for pads without taking controller input away from Qt. */
    void setPresenceActive(bool active);
    /** Helm owner: live input routed to SDL and a state sample on every poll tick. */
    void setHelmActive(bool active);
    /** Enables the virtual pad invokables; called only when the control channel is on. */
    void setVirtualPadAllowed(bool allowed) { virtualPadAllowed_ = allowed; }

    /** Attaches an SDL virtual gamepad while the subsystem is up; no-op unless allowed. */
    Q_INVOKABLE void attachVirtualPad();
    Q_INVOKABLE void detachVirtualPad();
    /** Sets the virtual pad state: @p buttons as in `buttons`, sticks −1…1, triggers 0…1. */
    Q_INVOKABLE void setVirtualInput(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);

    bool connected() const { return connected_; }
    QString name() const { return name_; }
    QString typeName() const { return typeName_; }
    int availableButtons() const { return availableButtons_; }

    int buttons() const { return buttons_; }
    qreal leftX() const { return axes_[0]; }
    qreal leftY() const { return axes_[1]; }
    qreal rightX() const { return axes_[2]; }
    qreal rightY() const { return axes_[3]; }
    qreal leftTrigger() const { return axes_[4]; }
    qreal rightTrigger() const { return axes_[5]; }

signals:
    void activeChanged();
    void runningChanged();
    void readyChanged();
    void errorChanged();
    void connectionChanged();
    void stateChanged();

private:
    void updateOwners();
    void onWorkerError(const QString& error);
    void onWorkerReady(bool ready);
    void onWorkerConnection(bool connected, const QString& name, const QString& typeName, int availableButtons);
    void onWorkerState(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);

    QThread thread_;
    GamepadWorker* worker_ = nullptr;
    bool active_ = false;
    bool presenceActive_ = false;
    bool helmActive_ = false;
    bool running_ = false;
    bool ready_ = false;
    bool routing_ = false;
    bool continuous_ = false;
    bool virtualPadAllowed_ = false;
    QString error_;
    bool connected_ = false;
    QString name_;
    QString typeName_;
    int availableButtons_ = 0;
    int buttons_ = 0;
    std::array<qreal, 6> axes_{};
};
