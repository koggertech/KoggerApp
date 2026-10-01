#pragma once

#include <QObject>
#include <QThread>
#include <memory>

#include "device_manager.h"
#include "mission_transfer.h"


class DeviceManagerWrapper : public QObject
{
    Q_OBJECT

public:
    /*methods*/
    DeviceManagerWrapper(QObject* parent = nullptr);
    ~DeviceManagerWrapper() override;

    Q_PROPERTY(QList<DevQProperty*> devs READ getDevList NOTIFY devChanged)
    Q_PROPERTY(bool standAvailable READ standAvailable NOTIFY standAvailableChanged)
    Q_PROPERTY(bool protoBinConsoled READ getProtoBinConsoled WRITE setProtoBinConsoled NOTIFY protoBinConsoledChanged)
    Q_PROPERTY(bool nmeaConsoled READ getNmeaConsoled WRITE setNmeaConsoled NOTIFY nmeaConsoledChanged)
    Q_PROPERTY(StreamListModel* streamsList READ streamsList NOTIFY streamChanged)
    Q_PROPERTY(float vruVoltage READ vruVoltage NOTIFY vruChanged)
    Q_PROPERTY(float vruCurrent READ vruCurrent NOTIFY vruChanged)
    Q_PROPERTY(float vruVelocityH READ vruVelocityH NOTIFY vruChanged)
    Q_PROPERTY(int vruBatteryPercent READ vruBatteryPercent NOTIFY vruChanged)
    Q_PROPERTY(int pilotArmState READ pilotArmState NOTIFY vruChanged)
    Q_PROPERTY(int pilotModeState READ pilotModeState NOTIFY vruChanged)
    Q_PROPERTY(bool autopilotOnline READ autopilotOnline NOTIFY vruChanged)
    Q_PROPERTY(QString autopilotModeName READ currentAutopilotModeName NOTIFY vruChanged)
    Q_PROPERTY(bool proxyLinkActive READ proxyLinkActive NOTIFY vruChanged)
    Q_PROPERTY(double vehicleHomeLat READ vehicleHomeLat NOTIFY vruChanged)
    Q_PROPERTY(int autopilotLinkQuality READ autopilotLinkQuality NOTIFY vruChanged)
    Q_PROPERTY(bool radioRssiValid READ radioRssiValid NOTIFY vruChanged)
    Q_PROPERTY(int radioRssi READ radioRssi NOTIFY vruChanged)
    Q_PROPERTY(bool echogramDeliveryKnown READ echogramDeliveryKnown NOTIFY chartLossesChanged)
    Q_PROPERTY(double vehicleHomeLon READ vehicleHomeLon NOTIFY vruChanged)
    Q_PROPERTY(bool missionTransferActive READ missionTransferActive NOTIFY missionTransferChanged)
    Q_PROPERTY(bool missionDownloading READ missionDownloading NOTIFY missionTransferChanged)
    Q_PROPERTY(qreal missionTransferProgress READ missionTransferProgress NOTIFY missionTransferChanged)
    Q_PROPERTY(int averageChartLosses READ getAverageChartLosses NOTIFY chartLossesChanged)
    Q_PROPERTY(bool isbeaconDirectQueueAsk READ getUSBLBeaconDirectAsk WRITE setUSBLBeaconDirectAsk NOTIFY USBLBeaconDirectAskChanged)

    DeviceManager* getWorker();
    QUuid getFileUuid() const;

    /*QML*/
    QList<DevQProperty*> getDevList     () { return getWorker()->getDevList();     }
    bool                 standAvailable () { return getWorker()->standAvailable(); }
    StreamListModel*     streamsList    () { return getWorker()->streamsList();    }
    float                vruVoltage     () const { return autopilotState_.voltage; }
    float                vruCurrent     () const { return autopilotState_.current; }
    float                vruVelocityH   () const { return autopilotState_.velocityH; }
    int                  vruBatteryPercent() const { return autopilotState_.batteryPercent; }
    int                  pilotArmState  () const { return autopilotState_.armState; }
    int                  pilotModeState () const { return autopilotState_.flightMode; }
    bool                 autopilotOnline() const { return autopilotState_.online; }
    QString              currentAutopilotModeName() { return modeNameFor(pilotModeState()); }
    bool                 proxyLinkActive() const { return autopilotState_.proxyTraffic; }
    double               vehicleHomeLat() const { return autopilotState_.homeLat; }
    int                  autopilotLinkQuality() const { return autopilotState_.linkQuality; }
    bool                 radioRssiValid() const { return autopilotState_.radioRssiValid; }
    int                  radioRssi() const { return autopilotState_.radioRssi; }
    bool                 echogramDeliveryKnown() const { return echogramDeliveryKnown_; }
    double               vehicleHomeLon() const { return autopilotState_.homeLon; }
    bool                 missionTransferActive() const { return missionTransferActive_; }
    bool                 missionDownloading() const { return missionDownloading_; }
    qreal                missionTransferProgress() const { return missionTransferProgress_; }

    Q_INVOKABLE static QString modeNameFor(int mode);
    Q_INVOKABLE void autopilotArm(bool arm);
    Q_INVOKABLE void autopilotArmForce(bool arm);
    Q_INVOKABLE void autopilotSetMode(int customMode);
    Q_INVOKABLE void autopilotStartMission();

    void startWorkerThread();
    void initStreamList();

    bool getProtoBinConsoled() const { return protoBinConsoledState_; };
    bool getNmeaConsoled() const { return nmeaConsoledState_; };
    bool getUSBLBeaconDirectAsk() const { return USBLBeaconDirectAskState_; };
    int getAverageChartLosses() const {
        return averageChartLosses_;
    };


public slots:
    Q_INVOKABLE bool isCreatedId(int id) { return getWorker()->isCreatedId(id); };
    Q_INVOKABLE void startStreamDownload(int id);
    Q_INVOKABLE void cancelStreamDownload(int id);
    Q_INVOKABLE void refreshStreamList();
    void calcAverageChartLosses();
    void uploadMission(const autopilot::MissionBatches& batches);
    Q_INVOKABLE void downloadMission();
    void setProtoBinConsoled(bool state) {
        const bool changed = (protoBinConsoledState_ != state);
        protoBinConsoledState_ = state;
        getWorker()->setProtoBinConsoled(protoBinConsoledState_);
        if (changed) {
            emit protoBinConsoledChanged();
        }
    }

    void setNmeaConsoled(bool state) {
        const bool changed = (nmeaConsoledState_ != state);
        nmeaConsoledState_ = state;
        getWorker()->setNmeaConsoled(nmeaConsoledState_);
        if (changed) {
            emit nmeaConsoledChanged();
        }
    }

    void setUSBLBeaconDirectAsk(bool is_ask) {
        const bool changed = (USBLBeaconDirectAskState_ != is_ask);
        USBLBeaconDirectAskState_ = is_ask;
        getWorker()->setUSBLBeaconDirectAsk(USBLBeaconDirectAskState_);
        if (changed) {
            emit USBLBeaconDirectAskChanged();
        }
    }

signals:
    void sendOpenFile(QString path);
#ifdef SEPARATE_READING
    void sendCloseFile(bool);
#else
    void sendCloseFile();
#endif

    void devChanged();
    void standAvailableChanged();
    void streamChanged();
    void vruChanged();
    void autopilotCommandAcked(int command, int result);
    void missionTransferChanged();
    void missionUploadFinished(bool ok, int result, int missionType);
    void missionDownloadFinished(bool ok, int result, int missionType);
    void missionDownloaded(const autopilot::MissionBatches& batches);
    void chartLossesChanged();
    void protoBinConsoledChanged();
    void nmeaConsoledChanged();
    void USBLBeaconDirectAskChanged();

private:
    void onAutopilotState(const AutopilotState& state);
    void onMissionTransferProgress(int done, int total);
    void onMissionUploadFinished(bool ok, int result, int missionType);
    void onMissionDownloadFinished(bool ok, int result, int missionType, const autopilot::MissionBatches& batches);
    bool beginMissionTransfer(bool download);
    void endMissionTransfer(bool ok);

    std::unique_ptr<DeviceManager> workerObject_;
    std::unique_ptr<QThread> missionThread_;
    autopilot::MissionTransfer* missionTransfer_ = nullptr;
    AutopilotState autopilotState_;
    bool echogramDeliveryKnown_ = false;
    bool missionTransferActive_ = false;
    bool missionDownloading_ = false;
    qreal missionTransferProgress_ = 0.0;
#ifdef SEPARATE_READING
    std::unique_ptr<QThread> workerThread_;
    QList<QMetaObject::Connection> deviceManagerConnections_;
#endif

    int averageChartLosses_;
    bool protoBinConsoledState_;
    bool nmeaConsoledState_;
    bool USBLBeaconDirectAskState_;
}; // class DeviceWrapper
