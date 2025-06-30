#pragma once

#include <QObject>
#include <QThread>
#include <memory>

#include "device_manager.h"


class DeviceManagerWrapper : public QObject
{
    Q_OBJECT

public:
    /*methods*/
    DeviceManagerWrapper(QObject* parent = nullptr);
    ~DeviceManagerWrapper();

    Q_PROPERTY(QList<DevQProperty*> devs READ getDevList NOTIFY devChanged)
    Q_PROPERTY(bool protoBinConsoled WRITE setProtoBinConsoled)
    Q_PROPERTY(StreamListModel* streamsList READ streamsList NOTIFY streamChanged)
    Q_PROPERTY(float vruVoltage READ vruVoltage NOTIFY vruChanged)
    Q_PROPERTY(float vruCurrent READ vruCurrent NOTIFY vruChanged)
    Q_PROPERTY(float vruVelocityH READ vruVelocityH NOTIFY vruChanged)
    Q_PROPERTY(int pilotArmState READ pilotArmState NOTIFY vruChanged)
    Q_PROPERTY(int pilotModeState READ pilotModeState NOTIFY vruChanged)
    Q_PROPERTY(int averageChartLosses READ getAverageChartLosses NOTIFY chartLossesChanged)
    Q_PROPERTY(bool isbeaconDirectQueueAsk WRITE setUSBLBeaconDirectAsk)

    DeviceManager* getWorker();
    QUuid getFileUuid() const;

    /*QML*/
    QList<DevQProperty*> getDevList     () { return getWorker()->getDevList();     }
    StreamListModel*     streamsList    () { return getWorker()->streamsList();    }
    float                vruVoltage     () { return getWorker()->vruVoltage();     }
    float                vruCurrent     () { return getWorker()->vruCurrent();     }
    float                vruVelocityH   () { return getWorker()->vruVelocityH();   }
    int                  pilotArmState  () { return getWorker()->pilotArmState();  }
    int                  pilotModeState () { return getWorker()->pilotModeState(); }

    void                 setProtoBinConsoled(bool state) { getWorker()->setProtoBinConsoled(state); }

    void                 setUSBLBeaconDirectAsk(bool is_ask) { getWorker()->setUSBLBeaconDirectAsk(is_ask); }

    int getAverageChartLosses() const {
        return averageChartLosses_;
    };

public slots:
    Q_INVOKABLE bool isCreatedId(int id) { return getWorker()->isCreatedId(id); };
    void calcAverageChartLosses();

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

private:
    std::unique_ptr<DeviceManager> workerObject_;
#ifdef SEPARATE_READING
    std::unique_ptr<QThread> workerThread_;
    QList<QMetaObject::Connection> deviceManagerConnections_;
#endif

    int averageChartLosses_;
}; // class DeviceWrapper
