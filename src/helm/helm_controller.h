#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "helm_bindings.h"

class GamepadMonitor;
class DeviceManagerWrapper;
class Notifications;

/**
 * Drives an ArduRover vehicle from the gamepad: streams MANUAL_CONTROL over the autopilot link
 * while a session is on. A session starts only by engage(), checks the vehicle parameters first,
 * switches the vehicle to Manual, and ends in Loiter (Hold when Loiter is refused) on disengage()
 * or pad loss; a link loss or a mode change outside Manual / Loiter / Hold ends it at once.
 */
class HelmController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool background READ background NOTIFY backgroundChanged)
    Q_PROPERTY(qreal steering READ steering NOTIFY commandChanged)
    Q_PROPERTY(qreal throttle READ throttle NOTIFY commandChanged)
    Q_PROPERTY(qreal forwardLimit READ forwardLimit NOTIFY commandChanged)
    Q_PROPERTY(QString vehicleStatus READ vehicleStatus NOTIFY vehicleStatusChanged)
    Q_PROPERTY(qreal stickDeadzone READ stickDeadzone WRITE setStickDeadzone NOTIFY tuningChanged)
    Q_PROPERTY(qreal triggerDeadzone READ triggerDeadzone WRITE setTriggerDeadzone NOTIFY tuningChanged)
    Q_PROPERTY(qreal throttleExpo READ throttleExpo WRITE setThrottleExpo NOTIFY tuningChanged)
    Q_PROPERTY(qreal startForwardLimit READ startForwardLimit WRITE setStartForwardLimit NOTIFY tuningChanged)
    Q_PROPERTY(qreal reverseLimit READ reverseLimit WRITE setReverseLimit NOTIFY tuningChanged)
    Q_PROPERTY(int rateHz READ rateHz WRITE setRateHz NOTIFY tuningChanged)
    Q_PROPERTY(QVariantList bindings READ bindings NOTIFY bindingsChanged)
    Q_PROPERTY(QVariantList bindingInputs READ bindingInputs WRITE setBindingInputs NOTIFY bindingsChanged)

public:
    enum State {
        Off,
        Checking,
        Starting,
        Driving,
        Stopping
    };
    Q_ENUM(State)

    HelmController(GamepadMonitor* monitor, DeviceManagerWrapper* vehicle, Notifications* notifications, QObject* parent = nullptr);
    ~HelmController() override;

    /** A pad is connected and an autopilot is online. */
    bool available() const;
    int state() const { return state_; }
    bool active() const { return state_ != Off; }
    /** Desktop window without focus during a session: control goes on. */
    bool background() const { return background_; }
    qreal steering() const { return command_.steering; }
    qreal throttle() const { return command_.throttle; }
    qreal forwardLimit() const { return forwardLimit_; }
    /** Result of the last parameter check, one line per finding; empty before the first check. */
    QString vehicleStatus() const { return vehicleStatus_; }

    qreal stickDeadzone() const { return tuning_.stickDeadzone; }
    void setStickDeadzone(qreal value);
    qreal triggerDeadzone() const { return tuning_.triggerDeadzone; }
    void setTriggerDeadzone(qreal value);
    qreal throttleExpo() const { return tuning_.throttleExpo; }
    void setThrottleExpo(qreal value);
    qreal startForwardLimit() const { return startForwardLimit_; }
    void setStartForwardLimit(qreal value);
    qreal reverseLimit() const { return tuning_.reverseLimit; }
    void setReverseLimit(qreal value);
    int rateHz() const { return rateHz_; }
    void setRateHz(int value);

    /** One entry per action in UI order: action (int), key (string), kind ("button", "throttle", "steering"), input (int, −1 = none). */
    QVariantList bindings() const;
    /** Input code per action, indexed by action; a write that breaks the binding rules is ignored. */
    QVariantList bindingInputs() const;
    void setBindingInputs(const QVariantList& inputs);

    /**
     * Binds a gamepad input to an action; an action that held it takes the old input or is left without one.
     * Returns {ok, swapped: [action], cleared: [action]}. Refused while a session is on or when the input does not fit.
     */
    Q_INVOKABLE QVariantMap assignBinding(int action, int input);
    Q_INVOKABLE void resetBindings();

    /** Starts a session: parameter check, Manual, then streaming. Refused with a toast when unsafe. */
    Q_INVOKABLE void engage();
    /** Ends the session: Loiter, Hold when Loiter is refused, then the stream stops. */
    Q_INVOKABLE void disengage();

signals:
    void availableChanged();
    void stateChanged();
    void backgroundChanged();
    void commandChanged();
    void vehicleStatusChanged();
    void tuningChanged();
    void bindingsChanged();

private:
    enum class ModeIntent {
        None,
        Engage,
        Toggle,
        StopLoiter,
        StopHold
    };

    void onVehicleChanged();
    void onPadConnection();
    void onPadState();
    void onParam(const QString& name, float value);
    void onCommandAcked(int command, int result);
    void onApplicationStateChanged(Qt::ApplicationState appState);
    void onSendTick();
    void onCheckDeadline();
    void pumpParams();
    void requestParams(const QStringList& names);
    bool checkComplete() const;
    void finishCheck();
    void requestMode(int mode, ModeIntent intent);
    void onModeResult(bool ok);
    void safeStop(const QString& reason);
    void finish(const QString& reason);
    void setState(State state);
    void setBackground(bool background);
    void sendCommand(const helm::Command& command);
    void notify(const QString& text);
    void loadTuning();
    void saveTuning();
    void loadBindings();
    void saveBindings();
    helm::PadState padState() const;

    GamepadMonitor* monitor_ = nullptr;
    DeviceManagerWrapper* vehicle_ = nullptr;
    Notifications* notifications_ = nullptr;

    State state_ = Off;
    bool available_ = false;
    bool background_ = false;
    helm::Tuning tuning_;
    helm::Bindings bindings_ = helm::defaultBindings();
    qreal startForwardLimit_ = 1.0;
    qreal forwardLimit_ = 1.0;
    int rateHz_ = 20;
    helm::Command command_;
    double steeringNeutral_ = 0.0;
    double throttleNeutral_ = 0.0;

    QTimer sendTimer_;
    QTimer paramTimer_;
    QTimer checkTimer_;
    QTimer modeTimer_;
    QElapsedTimer clock_;
    qint64 lastSampleMs_ = -1;
    qint64 checkStartMs_ = -1;
    bool checkRetried_ = false;
    int previousButtons_ = 0;

    QStringList paramQueue_;
    QSet<QString> wantedParams_;
    QHash<QString, double> params_;
    QString stopReason_;
    QString vehicleStatus_;

    int pendingMode_ = -1;
    ModeIntent modeIntent_ = ModeIntent::None;
};
