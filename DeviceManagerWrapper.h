#pragma once

#include <QObject>
#include <QThread>
#include <memory>

#include "DeviceManager.h"


class DeviceManagerWrapper : public QObject
{
    Q_OBJECT

public:
    /*methods*/
    DeviceManagerWrapper(QObject* parent = 0);
    ~DeviceManagerWrapper();

    Q_PROPERTY(QList<DevQProperty*> devs                READ    getDevList              NOTIFY devChanged)
    Q_PROPERTY(bool                 protoBinConsoled    WRITE   setProtoBinConsoled)
    Q_PROPERTY(StreamListModel*     streamsList         READ    streamsList             NOTIFY streamChanged)
    Q_PROPERTY(float                vruVoltage          READ    vruVoltage              NOTIFY vruChanged)
    Q_PROPERTY(float                vruCurrent          READ    vruCurrent              NOTIFY vruChanged)
    Q_PROPERTY(float                vruVelocityH        READ    vruVelocityH            NOTIFY vruChanged)
    Q_PROPERTY(int                  pilotArmState       READ    pilotArmState           NOTIFY vruChanged)
    Q_PROPERTY(int                  pilotModeState      READ    pilotModeState          NOTIFY vruChanged)

    // MotorControl on DeviceManager
    Q_PROPERTY(int countMotorDevices READ getMotorCountDevices NOTIFY motorDeviceChanged)

    DeviceManager* getWorker();

    /*QML*/
    QList<DevQProperty*> getDevList         ()           { return getWorker()->getDevList();        }
    StreamListModel*     streamsList        ()           { return getWorker()->streamsList();       }
    float                vruVoltage         ()           { return getWorker()->vruVoltage();        }
    float                vruCurrent         ()           { return getWorker()->vruCurrent();        }
    float                vruVelocityH       ()           { return getWorker()->vruVelocityH();      }
    int                 pilotArmState      ()           { return getWorker()->pilotArmState();     }
    int                 pilotModeState     ()           { return getWorker()->pilotModeState();    }
    int getMotorCountDevices () { if (getWorker()->isMotorControlCreated()) { return 1; } else { return 0; } }
    void                 setProtoBinConsoled(bool state) { getWorker()->setProtoBinConsoled(state); }

public slots:
    Q_INVOKABLE bool isCreatedId(int id) { return getWorker()->isCreatedId(id); };

private slots:


signals:
    void sendOpenFile(QString path);
    void sendCloseFile();

    void devChanged();
    void streamChanged();
    void vruChanged();
    void motorDeviceChanged();

private:
    std::unique_ptr<QThread> workerThread_;
    std::unique_ptr<DeviceManager> workerObject_;

}; // class DeviceWrapper
