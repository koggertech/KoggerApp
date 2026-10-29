#pragma once

#include <array>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVariantList>

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
    Q_PROPERTY(int padId READ padId NOTIFY connectionChanged)
    Q_PROPERTY(QVariantList pads READ pads NOTIFY padsChanged)
    Q_PROPERTY(QString name READ name NOTIFY connectionChanged)
    Q_PROPERTY(QString typeName READ typeName NOTIFY connectionChanged)
    Q_PROPERTY(QString family READ family NOTIFY connectionChanged)
    Q_PROPERTY(QStringList faceLabels READ faceLabels NOTIFY connectionChanged)
    Q_PROPERTY(int availableButtons READ availableButtons NOTIFY connectionChanged)
    Q_PROPERTY(int buttons READ buttons NOTIFY stateChanged)
    Q_PROPERTY(qreal leftX READ leftX NOTIFY stateChanged)
    Q_PROPERTY(qreal leftY READ leftY NOTIFY stateChanged)
    Q_PROPERTY(qreal rightX READ rightX NOTIFY stateChanged)
    Q_PROPERTY(qreal rightY READ rightY NOTIFY stateChanged)
    Q_PROPERTY(qreal leftTrigger READ leftTrigger NOTIFY stateChanged)
    Q_PROPERTY(qreal rightTrigger READ rightTrigger NOTIFY stateChanged)
    Q_PROPERTY(qreal rumbleLow READ rumbleLow NOTIFY rumbleChanged)
    Q_PROPERTY(qreal rumbleHigh READ rumbleHigh NOTIFY rumbleChanged)

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

    /**
     * Attaches an SDL virtual gamepad while the subsystem is up; no-op unless allowed. A non-zero USB
     * @p vendorId / @p productId lets SDL type it like that real pad (e.g. 0x054C / 0x09CC for a DualShock 4).
     */
    Q_INVOKABLE void attachVirtualPad(int vendorId = 0, int productId = 0);
    Q_INVOKABLE void detachVirtualPad();
    /** Sets the virtual pad state: @p buttons as in `buttons`, sticks −1…1, triggers 0…1. */
    Q_INVOKABLE void setVirtualInput(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);

    bool connected() const { return connected_; }
    /** SDL instance id of the open pad; 0 while none is open. */
    int padId() const { return padId_; }
    /**
     * Every gamepad SDL sees, in connection order: {id, name, label}; `label` is the name, numbered
     * ("Xbox Controller 1", "… 2") when several pads share it. Empty while the subsystem is down.
     */
    QVariantList pads() const { return pads_; }
    /**
     * Makes @p id the pad to open while no session holds the current one; 0 restores "the newest pad wins".
     * Kept for this app run until that pad disconnects; a session in progress keeps its pad and the choice
     * applies once it ends.
     */
    Q_INVOKABLE void selectPad(int id);
    QString name() const { return name_; }
    QString typeName() const { return typeName_; }
    /** Layout family of the open pad: "xbox", "ps4", "ps5", "steam" or "generic"; empty while none is open. */
    QString family() const { return family_; }
    /** SDL labels of the south, east, west and north buttons: "A"…"Y", "cross", "circle", "square", "triangle" or empty. */
    QStringList faceLabels() const { return faceLabels_; }
    int availableButtons() const { return availableButtons_; }

    int buttons() const { return buttons_; }
    qreal leftX() const { return axes_[0]; }
    qreal leftY() const { return axes_[1]; }
    qreal rightX() const { return axes_[2]; }
    qreal rightY() const { return axes_[3]; }
    qreal leftTrigger() const { return axes_[4]; }
    qreal rightTrigger() const { return axes_[5]; }

    /**
     * Vibrates the open pad: low- and high-frequency motor, 0…1 each. A non-zero request lasts a quarter
     * of a second, so the caller repeats it while it holds; 0, 0 stops at once and is sent only once.
     */
    void setRumble(qreal low, qreal high);
    qreal rumbleLow() const { return rumble_[0]; }
    qreal rumbleHigh() const { return rumble_[1]; }

signals:
    void activeChanged();
    void runningChanged();
    void readyChanged();
    void errorChanged();
    void connectionChanged();
    void stateChanged();
    void rumbleChanged();
    void padsChanged();

private:
    void updateOwners();
    void onWorkerError(const QString& error);
    void onWorkerReady(bool ready);
    void onWorkerConnection(bool connected, int id, const QString& name, const QString& typeName, const QString& family,
                            const QStringList& faceLabels, int availableButtons);
    void onWorkerPads(const QVariantList& pads);
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
    int padId_ = 0;
    QVariantList pads_;
    QString name_;
    QString typeName_;
    QString family_;
    QStringList faceLabels_;
    int availableButtons_ = 0;
    int buttons_ = 0;
    std::array<qreal, 6> axes_{};
    std::array<qreal, 2> rumble_{};
};
