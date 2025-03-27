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
    DeviceManagerWrapper(QObject* parent = nullptr);
    ~DeviceManagerWrapper();

    Q_PROPERTY(QList<DevQProperty*> devs                READ    getDevList              NOTIFY devChanged)
    Q_PROPERTY(bool                 protoBinConsoled    WRITE   setProtoBinConsoled)
    Q_PROPERTY(StreamListModel*     streamsList         READ    streamsList             NOTIFY streamChanged)
    Q_PROPERTY(float                vruVoltage          READ    vruVoltage              NOTIFY vruChanged)
    Q_PROPERTY(float                vruCurrent          READ    vruCurrent              NOTIFY vruChanged)
    Q_PROPERTY(float                vruVelocityH        READ    vruVelocityH            NOTIFY vruChanged)
    Q_PROPERTY(int                  pilotArmState       READ    pilotArmState           NOTIFY vruChanged)
    Q_PROPERTY(int                  pilotModeState      READ    pilotModeState          NOTIFY vruChanged)
    Q_PROPERTY(int averageChartLosses  READ getAverageChartLosses      NOTIFY chartLossesChanged)

    Q_PROPERTY(bool                 isbeaconDirectQueueAsk    WRITE   setUSBLBeaconDirectAsk)

#ifdef MOTOR
    // MotorControl on DeviceManager
    Q_PROPERTY(int countMotorDevices READ getMotorCountDevices NOTIFY motorDeviceChanged)
    Q_PROPERTY(float fAngle READ getFAngle NOTIFY angleChanged)
    Q_PROPERTY(float sAngle READ getSAngle NOTIFY angleChanged)
    Q_PROPERTY(float currFAngle READ getCurrFAngle NOTIFY enginesStopped)
    Q_PROPERTY(float currSAngle READ getCurrSAngle NOTIFY enginesStopped)
    Q_PROPERTY(float taskFAngle READ getTaskFAngle NOTIFY enginesStopped)
    Q_PROPERTY(float taskSAngle READ getTaskSAngle NOTIFY enginesStopped)
    int                  getMotorCountDevices() { if (getWorker()->isMotorControlCreated()) { return 1; } else { return 0; } }
    float                getFAngle()            { return getWorker()->getFAngle(); }
    float                getSAngle()            { return getWorker()->getSAngle(); }
    float                getCurrFAngle()        { return currFAngle_; }
    float                getCurrSAngle()        { return currSAngle_; }
    float                getTaskFAngle()        { return taskFAngle_; }
    float                getTaskSAngle()        { return taskSAngle_; }
#endif

    DeviceManager* getWorker();

    /*QML*/
    QList<DevQProperty*> getDevList        ()           { return getWorker()->getDevList();        }
    StreamListModel*     streamsList       ()           { return getWorker()->streamsList();       }
    float                vruVoltage        ()           { return getWorker()->vruVoltage();        }
    float                vruCurrent        ()           { return getWorker()->vruCurrent();        }
    float                vruVelocityH      ()           { return getWorker()->vruVelocityH();      }
    int                 pilotArmState      ()           { return getWorker()->pilotArmState();     }
    int                 pilotModeState     ()           { return getWorker()->pilotModeState();    }

    void                 setProtoBinConsoled(bool state) { getWorker()->setProtoBinConsoled(state); }

    void                 setUSBLBeaconDirectAsk(bool is_ask) { getWorker()->setUSBLBeaconDirectAsk(is_ask); }

    int getAverageChartLosses() const {
        return averageChartLosses_;
    };

public slots:
    Q_INVOKABLE bool isCreatedId(int id) { return getWorker()->isCreatedId(id); };
    void calcAverageChartLosses();

private slots:

#ifdef MOTOR
    void posIsConstant(float currFAngle, float taskFAngle, float currSAngle, float taskSAngle);
#endif

signals:
    void sendOpenFile(QString path);
#ifdef SEPARATE_READING
    void sendCloseFile(bool);
#else
    void sendCloseFile();
#endif

    void devChanged();
    void streamChanged();
    void vruChanged();
    void chartLossesChanged();

#ifdef MOTOR
    void sendOpenCsvFile(QString path);
    void sendClearTasks();
    void motorDeviceChanged();
    void angleChanged();
    void enginesStopped();
    void sendReturnToZero(int id);
    void sendRunSteps(int id, int val, int angle);
#endif

private:
    std::unique_ptr<DeviceManager> workerObject_;
#ifdef SEPARATE_READING
    std::unique_ptr<QThread> workerThread_;
    QList<QMetaObject::Connection> deviceManagerConnections_;
#endif

#ifdef MOTOR
    float currFAngle_ = 0.0f;
    float taskFAngle_ = 0.0f;
    float currSAngle_ = 0.0f;
    float taskSAngle_ = 0.0f;
#endif
    int averageChartLosses_;
}; // class DeviceWrapper
