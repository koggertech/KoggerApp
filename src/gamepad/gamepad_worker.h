#pragma once

#include <array>

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
    void stateChanged(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);

private:
    void poll();
    void selectLatestPad();
    void emitConnection();
    void emitPads();
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
    int preferredId_ = 0;
    int buttons_ = 0;
    std::array<qreal, 6> axes_{};
};
