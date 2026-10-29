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

/**
 * Drives an ArduRover vehicle from the gamepad: streams MANUAL_CONTROL over the autopilot link
 * while a session is on. A session starts only by engage(), checks the vehicle parameters first,
 * switches the vehicle to Manual (a vehicle already in Acro stays there), and ends in Loiter (Hold when
 * Loiter is refused) on disengage() or pad loss; a link loss or a mode change outside Manual / Acro /
 * Loiter / Hold (and Auto or RTL the pad asked for, until the vehicle next enters one of those four) ends it
 * at once. The pad drives in Manual and
 * in Acro, where a released stick holds the heading; other modes get neutral frames. During a session
 * the pad's bound buttons pick Manual, Hold, Loiter, Auto and RTL and toggle course hold (Acro ↔
 * Manual), and LB + RB together toggle arm / disarm (arming only at zero throttle). The tuning, the
 * limits included, applies to the next frame of a running session.
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
    Q_PROPERTY(QString vehicleStatus READ vehicleStatus NOTIFY vehicleStatusChanged)
    Q_PROPERTY(qreal stickDeadzone READ stickDeadzone WRITE setStickDeadzone NOTIFY tuningChanged)
    Q_PROPERTY(qreal triggerDeadzone READ triggerDeadzone WRITE setTriggerDeadzone NOTIFY tuningChanged)
    Q_PROPERTY(qreal throttleExpo READ throttleExpo WRITE setThrottleExpo NOTIFY tuningChanged)
    Q_PROPERTY(qreal forwardLimit READ forwardLimit WRITE setForwardLimit NOTIFY tuningChanged)
    Q_PROPERTY(qreal reverseLimit READ reverseLimit WRITE setReverseLimit NOTIFY tuningChanged)
    Q_PROPERTY(qreal steeringLimit READ steeringLimit WRITE setSteeringLimit NOTIFY tuningChanged)
    Q_PROPERTY(int rateHz READ rateHz WRITE setRateHz NOTIFY tuningChanged)
    Q_PROPERTY(QString padFamily READ padFamily NOTIFY padFamilyChanged)
    Q_PROPERTY(QVariantList bindings READ bindings NOTIFY bindingsChanged)
    Q_PROPERTY(QVariantList bindingInputs READ bindingInputs WRITE setBindingInputs NOTIFY bindingsChanged)
    Q_PROPERTY(bool configuring READ configuring WRITE setConfiguring NOTIFY configuringChanged)
    Q_PROPERTY(bool layoutEditable READ layoutEditable NOTIFY layoutEditableChanged)
    Q_PROPERTY(int confirmation READ confirmation NOTIFY confirmationChanged)
    Q_PROPERTY(bool confirmationBusy READ confirmationBusy NOTIFY confirmationChanged)
    Q_PROPERTY(qreal overrideSeconds READ overrideSeconds NOTIFY confirmationChanged)

public:
    enum State {
        Off,
        Checking,
        Starting,
        Driving,
        Stopping
    };
    Q_ENUM(State)

    /** What engage() waits for the operator to accept before it starts. */
    enum Confirmation {
        ConfirmNone,
        ConfirmTransmitter,
        ConfirmLinkLossStop
    };
    Q_ENUM(Confirmation)

    HelmController(GamepadMonitor* monitor, DeviceManagerWrapper* vehicle, QObject* parent = nullptr);
    ~HelmController() override;

    /** A pad is connected and an autopilot is online. */
    bool available() const;
    int state() const { return state_; }
    bool active() const { return state_ != Off; }
    /** Desktop window without focus during a session: control goes on. */
    bool background() const { return background_; }
    qreal steering() const { return command_.steering; }
    qreal throttle() const { return command_.throttle; }
    /** Result of the last parameter check, one line per finding; empty before the first check. */
    QString vehicleStatus() const { return vehicleStatus_; }

    /**
     * Pending question of the last engage, a Confirmation value. Both vehicle failsafes off means a lost
     * link leaves the last command running unless the RC transmitter takes over: with an RC receiver the
     * operator confirms the transmitter is on (once per app run), without one the app offers to set
     * FS_GCS_ENABLE = 2. Cleared by confirm(), cancelConfirmation(), pad loss or link loss.
     */
    int confirmation() const { return confirmation_; }
    /**
     * The gamepad settings dialog is open: the operator tests and rebinds inputs, so nothing from the pad
     * reaches the vehicle. A driving session stays on and streams neutral frames, its pad buttons do
     * nothing, and after the dialog closes the pad drives again once the throttle is released; a
     * session still checking or starting ends the safe way. engage() is refused while it stays set.
     */
    bool configuring() const { return configuring_; }
    void setConfiguring(bool configuring);
    /** The layout and the pad profile can change: no session, or a driving session paused by configuring. */
    bool layoutEditable() const { return state_ == Off || (state_ == Driving && configuring_); }

    /** confirm() is writing FS_GCS_ENABLE and waits for the vehicle to read it back. */
    bool confirmationBusy() const { return confirmationBusy_; }
    /** RC_OVERRIDE_TIME from the last check: how long after the last frame the transmitter takes over. */
    qreal overrideSeconds() const { return overrideSeconds_; }
    /**
     * A throttle input of the pad is past its dead zone during a session, whatever is sent: outside Manual and
     * while paused the stream is neutral, so the sent throttle cannot tell whether arming is safe.
     */
    Q_INVOKABLE bool throttleHeld() const;
    /** Accepts the pending confirmation and starts the session (after the parameter write for ConfirmLinkLossStop). */
    Q_INVOKABLE void confirm();
    Q_INVOKABLE void cancelConfirmation();

    qreal stickDeadzone() const { return tuning_.stickDeadzone; }
    void setStickDeadzone(qreal value);
    qreal triggerDeadzone() const { return tuning_.triggerDeadzone; }
    void setTriggerDeadzone(qreal value);
    qreal throttleExpo() const { return tuning_.throttleExpo; }
    void setThrottleExpo(qreal value);
    qreal forwardLimit() const { return tuning_.forwardLimit; }
    void setForwardLimit(qreal value);
    qreal reverseLimit() const { return tuning_.reverseLimit; }
    void setReverseLimit(qreal value);
    qreal steeringLimit() const { return tuning_.steeringLimit; }
    void setSteeringLimit(qreal value);
    int rateHz() const { return rateHz_; }
    void setRateHz(int value);
    /** Restores the shared tuning: the throttle and steering limits, throttle expo and send rate; applies to a running session too. */
    Q_INVOKABLE void resetTuning();

    /**
     * Pad family whose profile is in use ("xbox", "ps4", "ps5", "steam", "generic"): the layout and both dead
     * zones are stored per family and follow the open pad while no session runs.
     */
    QString padFamily() const { return padFamily_; }

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
    /** Resets the layout and both dead zones of the current pad family; refused while a session is on. */
    Q_INVOKABLE void resetPadSettings();

    /** Starts a session: parameter check, Manual, then streaming. Refused with a toast when unsafe. */
    Q_INVOKABLE void engage();
    /**
     * Ends the session. A session started in Manual or Acro on a boat with an RC receiver, still in that mode,
     * stays in it and releases both channels to the transmitter at once; otherwise Loiter, Hold when Loiter is
     * refused, then the stream stops.
     */
    Q_INVOKABLE void disengage();

signals:
    void availableChanged();
    void stateChanged();
    void backgroundChanged();
    void commandChanged();
    void vehicleStatusChanged();
    void tuningChanged();
    void bindingsChanged();
    void padFamilyChanged();
    void confirmationChanged();
    void configuringChanged();
    void layoutEditableChanged();

private:
    static constexpr int kDefaultRateHz = 20;

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
    void onRcChannelCount(int count);
    void requestPadMode(int mode);
    void toggleArm();
    void onCommandAcked(int command, int result);
    void onWriteDeadline();
    void setConfirmation(int confirmation, bool busy);
    void onApplicationStateChanged(Qt::ApplicationState appState);
    void onSendTick();
    void onCheckDeadline();
    void pumpParams();
    void requestParams(const QStringList& names);
    bool checkComplete() const;
    void finishCheck();
    void requestMode(int mode, ModeIntent intent);
    void onModeResult(bool ok);
    void safeStop(const QString& reason, const QString& handBackReason);
    bool handsBackToTransmitter() const;
    void finish(const QString& reason);
    void setState(State state);
    void setBackground(bool background);
    void sendCommand(const helm::Command& command);
    void notify(const QString& text, int severity);
    void loadTuning();
    void saveTuning();
    void loadProfile();
    void saveProfile();
    QString profileGroup() const;
    void saveBindings();
    helm::PadState padState() const;

    GamepadMonitor* monitor_ = nullptr;
    DeviceManagerWrapper* vehicle_ = nullptr;

    State state_ = Off;
    bool available_ = false;
    bool background_ = false;
    helm::Tuning tuning_;
    helm::Bindings bindings_ = helm::defaultBindings();
    QString padFamily_;
    int rateHz_ = kDefaultRateHz;
    helm::Command command_;
    double steeringNeutral_ = 0.0;
    double throttleNeutral_ = 0.0;

    QTimer sendTimer_;
    QTimer paramTimer_;
    QTimer checkTimer_;
    QTimer modeTimer_;
    QTimer writeTimer_;
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
    int rcChannelCount_ = -1;
    int modeBeforeEngage_ = -1;
    bool configuring_ = false;
    bool holdUntilReleased_ = false;
    int padMissionModes_ = 0;
    int seenMode_ = -1;
    bool bumpersSpent_ = false;
    int confirmation_ = ConfirmNone;
    bool confirmationBusy_ = false;
    bool transmitterConfirmed_ = false;
    bool writeRetried_ = false;
    qreal overrideSeconds_ = 0.0;

    int pendingMode_ = -1;
    ModeIntent modeIntent_ = ModeIntent::None;
};
