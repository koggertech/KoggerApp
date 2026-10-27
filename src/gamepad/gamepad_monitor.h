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
    bool active() const { return active_; }
    void setActive(bool active);
    QString error() const { return error_; }

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
    void errorChanged();
    void connectionChanged();
    void stateChanged();

private:
    void onWorkerError(const QString& error);
    void onWorkerConnection(bool connected, const QString& name, const QString& typeName, int availableButtons);
    void onWorkerState(int buttons, qreal leftX, qreal leftY, qreal rightX, qreal rightY, qreal leftTrigger, qreal rightTrigger);

    QThread thread_;
    GamepadWorker* worker_ = nullptr;
    bool active_ = false;
    QString error_;
    bool connected_ = false;
    QString name_;
    QString typeName_;
    int availableButtons_ = 0;
    int buttons_ = 0;
    std::array<qreal, 6> axes_{};
};
