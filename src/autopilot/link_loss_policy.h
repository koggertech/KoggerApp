#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

namespace autopilot {

class AutopilotParams;

/**
 * What an ArduRover boat does when the ground-station link is lost, read from and written to the
 * vehicle's GCS failsafe parameters (FS_GCS_ENABLE, FS_ACTION) through AutopilotParams.
 *
 * Reads only its own parameters by name when an ArduPilot rover or boat comes online; any later
 * PARAM_VALUE of them (parameters window, helm, another ground station) updates it. "Nothing"
 * turns the GCS failsafe off and leaves FS_ACTION alone, since the radio failsafe shares it. A
 * change writes FS_ACTION first and FS_GCS_ENABLE only once the vehicle reports the new FS_ACTION,
 * so a refused action never leaves the failsafe on with the old one; it is done when both read
 * back as written.
 */
class LinkLossPolicy : public QObject
{
    Q_OBJECT
    /** An ArduPilot rover or boat is online. */
    Q_PROPERTY(bool available READ available NOTIFY changed)
    /** FS_GCS_ENABLE and FS_ACTION have been read. */
    Q_PROPERTY(bool known READ known NOTIFY changed)
    Q_PROPERTY(bool reading READ reading NOTIFY changed)
    /** A change is being written and not yet read back. */
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    /** Why the last read or change failed; empty otherwise. */
    Q_PROPERTY(QString error READ error NOTIFY changed)
    /** Choice of the vehicle's current values; ChoiceUnknown until known. */
    Q_PROPERTY(int choice READ choice NOTIFY changed)
    /** FS_GCS_ENABLE = 2; while the failsafe is off, the value the next change will use. */
    Q_PROPERTY(bool continueMission READ continueMission NOTIFY changed)
    /** FS_ACTION and FS_GCS_ENABLE as read; -1 while unknown. */
    Q_PROPERTY(int fsAction READ fsAction NOTIFY changed)
    Q_PROPERTY(int gcsEnable READ gcsEnable NOTIFY changed)
    /** The label of the current choice; for ChoiceOther the metadata label of FS_ACTION. */
    Q_PROPERTY(QString choiceLabel READ choiceLabel NOTIFY changed)
    /** A short word for FS_ACTION itself ("Hold", "RTL", …), also while the GCS failsafe is off; empty while unknown. */
    Q_PROPERTY(QString actionText READ actionText NOTIFY changed)
    /** { choice, label, enabled, note } per choice the vehicle's firmware offers. */
    Q_PROPERTY(QVariantList options READ options NOTIFY changed)
    /** FS_GCS_TIMEOUT + FS_TIMEOUT in seconds; negative when not both are known. */
    Q_PROPERTY(double delaySeconds READ delaySeconds NOTIFY changed)
    /** FS_THR_ENABLE is on, so FS_ACTION also acts on RC loss. */
    Q_PROPERTY(bool sharedWithRc READ sharedWithRc NOTIFY changed)
    /** FS_OPTIONS bit 0: the action is also taken in Hold. */
    Q_PROPERTY(bool activeInHold READ activeInHold NOTIFY changed)
    /** Another ground station behind the legacy proxy may keep the failsafe from firing. */
    Q_PROPERTY(bool proxyMasks READ proxyMasks NOTIFY changed)
    /** What a link loss leads to in the current mode (Outcome). */
    Q_PROPERTY(int outcome READ outcome NOTIFY changed)
    /** outcome as a short phrase: "→ RTL", "stays in Hold", "mission goes on", "no action", "…". */
    Q_PROPERTY(QString outcomeText READ outcomeText NOTIFY changed)

public:
    enum Choice {
        ChoiceUnknown = -1,
        ChoiceNothing = 0,
        ChoiceHold,
        ChoiceLoiter,
        ChoiceRtl,
        ChoiceSmartRtl,
        ChoiceOther
    };
    Q_ENUM(Choice)

    enum Outcome {
        OutcomeUnknown = 0,
        OutcomeNothing,
        OutcomeMission,
        OutcomeAction,
        OutcomeAlready
    };
    Q_ENUM(Outcome)

    static constexpr int kConfirmMarginMs = 2000;

    explicit LinkLossPolicy(AutopilotParams* params, QObject* parent = nullptr);

    bool available() const { return available_; }
    bool known() const { return fsAction_ >= 0 && gcsEnable_ >= 0; }
    bool reading() const { return reading_; }
    bool busy() const { return busy_; }
    QString error() const { return error_; }
    int choice() const;
    bool continueMission() const { return gcsEnable_ > 0 ? gcsEnable_ == kGcsContinueMission : continuePreferred_; }
    int fsAction() const { return fsAction_; }
    int gcsEnable() const { return gcsEnable_; }
    QString choiceLabel() const;
    QString actionText() const { return fsAction_ >= 0 ? actionWord(fsAction_) : QString(); }
    QVariantList options() const;
    double delaySeconds() const;
    bool sharedWithRc() const { return thrEnable_ > 0; }
    bool activeInHold() const { return fsOptions_ > 0 && (fsOptions_ & 1) != 0; }
    bool proxyMasks() const { return proxyTraffic_; }
    int outcome() const;
    QString outcomeText() const;

    /** Writes @p choice (Choice) with @p continueMission; false with error when refused at once. */
    Q_INVOKABLE bool apply(int choice, bool continueMission);
    /** Reads the parameters again. */
    Q_INVOKABLE void reread();

public slots:
    /**
     * The latched vehicle from HEARTBEAT: online, system / component id, MAV_AUTOPILOT, MAV_TYPE,
     * Rover mode number, legacy proxy traffic seen. Another system or component id while online
     * counts as a new vehicle: its values are read again.
     */
    void setVehicle(bool online, int systemId, int componentId, int autopilotKind, int vehicleType, int mode, bool proxyTraffic);

signals:
    void changed();

private:
    static constexpr int kGcsOff = 0;
    static constexpr int kGcsOn = 1;
    static constexpr int kGcsContinueMission = 2;

    void onParamChanged(const QString& name);
    void onRequestFinished(const QStringList& missing);
    void onWriteFailed(const QString& name, const QString& actual);
    void onConfirmTimeout();
    void readValue(const QString& name);
    void checkConfirmed();
    void finishBusy(const QString& error);
    void startConfirmTimer();
    bool writeGcsEnable();
    void clearValues();
    bool offersLoiter() const;
    static int actionFor(int choice);
    QString actionLabel(int fsAction) const;
    QString actionWord(int fsAction) const;

    AutopilotParams* params_;
    QTimer confirmTimer_{ this };
    bool available_ = false;
    bool reading_ = false;
    bool busy_ = false;
    bool proxyTraffic_ = false;
    bool continuePreferred_ = true;
    int mode_ = -1;
    int systemId_ = -1;
    int componentId_ = -1;
    int fsAction_ = -1;
    int gcsEnable_ = -1;
    int thrEnable_ = -1;
    int fsOptions_ = -1;
    int srtlPoints_ = -1;
    double gcsTimeout_ = -1.0;
    double fsTimeout_ = -1.0;
    int targetAction_ = -1;
    int targetGcs_ = -1;
    bool gcsDeferred_ = false;
    QString error_;
};

} // namespace autopilot
