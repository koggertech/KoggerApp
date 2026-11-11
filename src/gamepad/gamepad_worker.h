#pragma once

#include <array>

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

struct SDL_Gamepad;
struct SDL_Joystick;

class GamepadWorker : public QObject
{
    Q_OBJECT

public:
    /** Why the open pad closed: SDL removed it, the virtual pad was detached, or the subsystem stopped. */
    enum LossReason {
        LossRemoved,
        LossDetached,
        LossStopped
    };

    explicit GamepadWorker(QObject* parent = nullptr);
    ~GamepadWorker() override;

    void start();
    void stop();
    void setRouting(bool routing);
    void setContinuous(bool continuous);
    void attachVirtualPad(int vendorId, int productId);
    void detachVirtualPad();
    void setVirtualInput(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);
    void setRumble(qreal low, qreal high);
    void setPreferredPad(int id);

signals:
    void errorChanged(const QString& error);
    void readyChanged(bool ready);
    void connectionChanged(bool connected, int id, const QString& name, const QString& typeName, const QString& family,
                           const QStringList& faceLabels, int availableButtons);
    void padsChanged(const QVariantList& pads);
    void batteryChanged(int percent, const QString& state);
    void stateChanged(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);
    /**
     * The open pad is about to close: @p reason a LossReason, @p connection "wired" / "wireless" / empty,
     * @p batteryPercent the last SDL estimate (−1 unknown), @p idleMs since its input last changed. Comes
     * before the connectionChanged(false) of the same close.
     */
    void padLost(int reason, const QString& name, const QString& connection, int batteryPercent, qint64 idleMs);
    /** A pad with the GUID of the last lost one was added again, @p afterMs after the loss. */
    void padReturned(const QString& name, qint64 afterMs);

private:
    void poll();
    void selectLatestPad();
    void emitConnection();
    void emitPads();
    void emitBattery();
    void closePad(LossReason reason);
    void readState(bool force);
    void applyRouting();
    void rememberPad();
    void reportError(const char* where);
    void reportStall(qint64 pollGapMs, qint64 pumpMs);

    QTimer timer_;
    SDL_Gamepad* pad_ = nullptr;
    SDL_Joystick* virtualPad_ = nullptr;
    bool subsystemUp_ = false;
    bool sdlUsed_ = false;
    bool routing_ = false;
    bool continuous_ = false;
    int preferredId_ = 0;
    int buttons_ = 0;
    std::array<qreal, 6> axes_{};
    QElapsedTimer clock_;
    qint64 lastInputChangeMs_ = -1;
    qint64 lastPollMs_ = -1;
    qint64 lastStallLogMs_ = -1;
    int stallsSuppressed_ = 0;
    QString padName_;
    QString padGuid_;
    QString padConnection_;
    int padBattery_ = -1;
    QString lostGuid_;
    qint64 lostAtMs_ = -1;
};
