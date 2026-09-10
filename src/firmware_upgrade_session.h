#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class DevQProperty;

class FirmwareUpgradeSession : public QObject
{
    Q_OBJECT

public:
    explicit FirmwareUpgradeSession(QObject* parent = nullptr);

    bool isActive() const { return active_; }

    bool start(const QString& firmwarePath, DevQProperty* dev);
    void handleStatus(int status);

private:
    void begin(const QString& deviceLabel, const QString& fileName, uint32_t serialNumber);
    void publish();
    void finish(bool success, int status);
    void onTick();

    QTimer  ticker_;
    QString tag_;
    QString deviceLabel_;
    QString fileName_;
    qint64  lastActivityMs_ = 0;
    int     percent_        = 0;
    bool    active_         = false;
};
