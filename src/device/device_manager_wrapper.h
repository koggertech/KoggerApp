#pragma once

#include <QObject>
#include <QThread>
#include <QVariantList>
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
    /** Mission execution reported by the latched vehicle; see MissionTelemetry for the meaning of each value. */
    Q_PROPERTY(int missionCurrentSeq READ missionCurrentSeq NOTIFY vruChanged)
    Q_PROPERTY(int missionTotal READ missionTotal NOTIFY vruChanged)
    Q_PROPERTY(int missionState READ missionState NOTIFY vruChanged)
    Q_PROPERTY(int missionReachedSeq READ missionReachedSeq NOTIFY vruChanged)
    Q_PROPERTY(bool navigationValid READ navigationValid NOTIFY vruChanged)
    Q_PROPERTY(double waypointDistance READ waypointDistance NOTIFY vruChanged)
    Q_PROPERTY(double crossTrackError READ crossTrackError NOTIFY vruChanged)
    /** Vehicle position from GLOBAL_POSITION_INT of the latched vehicle, NaN while unknown. */
    /** GPS of the latched vehicle (GPS_RAW_INT): MAVLink GPS_FIX_TYPE or -1, satellites or -1, HDOP or NaN. */
    Q_PROPERTY(int gpsFixType READ gpsFixType NOTIFY vruChanged)
    Q_PROPERTY(int gpsSatellites READ gpsSatellites NOTIFY vruChanged)
    Q_PROPERTY(double gpsHdop READ gpsHdop NOTIFY vruChanged)
    Q_PROPERTY(double vehicleLat READ vehicleLat NOTIFY vruChanged)
    Q_PROPERTY(double vehicleLon READ vehicleLon NOTIFY vruChanged)
    /** Last STATUSTEXT messages of the vehicle, newest first: { severity, text, time (ms since epoch) }. */
    Q_PROPERTY(QVariantList autopilotMessages READ autopilotMessages NOTIFY autopilotMessagesChanged)
    Q_PROPERTY(bool missionTransferActive READ missionTransferActive NOTIFY missionTransferChanged)
    Q_PROPERTY(bool missionDownloading READ missionDownloading NOTIFY missionTransferChanged)
    /** True while the active transfer is a background read (readMissionSilently); the editor shows no progress for it. */
    Q_PROPERTY(bool missionTransferSilent READ missionTransferSilent NOTIFY missionTransferChanged)
    /** Restart the mission (MAV_CMD_DO_SET_MISSION_CURRENT 0) after a successful upload while the vehicle is not in Auto; see uploadMission. */
    Q_PROPERTY(bool restartMissionAfterUpload READ restartMissionAfterUpload WRITE setRestartMissionAfterUpload NOTIFY restartMissionAfterUploadChanged)
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
    int                  missionCurrentSeq() const { return autopilotState_.mission.seq; }
    int                  missionTotal() const { return autopilotState_.mission.total; }
    int                  missionState() const { return autopilotState_.mission.state; }
    int                  missionReachedSeq() const { return autopilotState_.mission.reachedSeq; }
    bool                 navigationValid() const { return autopilotState_.mission.navValid; }
    double               waypointDistance() const { return autopilotState_.mission.waypointDistance; }
    double               crossTrackError() const { return autopilotState_.mission.crossTrackError; }
    int                  gpsFixType() const { return autopilotState_.gpsFixType; }
    int                  gpsSatellites() const { return autopilotState_.gpsSatellites; }
    double               gpsHdop() const { return autopilotState_.gpsHdop; }
    double               vehicleLat() const { return autopilotState_.latitude; }
    double               vehicleLon() const { return autopilotState_.longitude; }
    QVariantList         autopilotMessages() const { return autopilotMessages_; }
    const AutopilotState& autopilotState() const { return autopilotState_; }
    bool                 missionTransferActive() const { return missionTransferActive_; }
    bool                 missionDownloading() const { return missionDownloading_; }
    bool                 missionTransferSilent() const { return missionTransferSilent_; }
    bool                 restartMissionAfterUpload() const { return restartMissionAfterUpload_; }
    void                 setRestartMissionAfterUpload(bool restart);
    qreal                missionTransferProgress() const { return missionTransferProgress_; }

    Q_INVOKABLE static QString modeNameFor(int mode);
    Q_INVOKABLE void autopilotArm(bool arm);
    Q_INVOKABLE void autopilotArmForce(bool arm);
    Q_INVOKABLE void autopilotSetMode(int customMode);
    Q_INVOKABLE void autopilotStartMission();
    /** Makes vehicle mission item @p seq current (skip ahead or back); see DeviceManager::autopilotSetMissionCurrent. */
    Q_INVOKABLE void autopilotSetMissionCurrent(int seq);

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
    /**
     * Uploads the lists. Once the route is on the vehicle (the whole upload succeeded, or only the
     * fence / rally part failed), the vehicle is not in Auto and restartMissionAfterUpload is set,
     * the mission is restarted
     * (MAV_CMD_DO_SET_MISSION_CURRENT 0): ArduPilot keeps its current item across an upload and,
     * with MIS_RESTART 0, would otherwise resume the new mission at the old item number.
     */
    void uploadMission(const autopilot::MissionBatches& batches);
    Q_INVOKABLE void downloadMission();
    /**
     * Reads route, fence and rally in the background when no transfer runs and an autopilot is
     * online. The result goes only to vehicleMissionRead(silent = true); a user upload or download
     * started meanwhile cancels it, and the one finish that read still produces (cancelled, or a
     * normal result already on its way) is swallowed.
     */
    void readMissionSilently();
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
    void autopilotStatusText(int severity, const QString& text);
    void autopilotMissionItemReached(int seq);
    void autopilotMessagesChanged();
    void missionTransferChanged();
    void restartMissionAfterUploadChanged();
    void missionUploadFinished(bool ok, int result, int missionType);
    void missionDownloadFinished(bool ok, int result, int missionType);
    void missionDownloaded(const autopilot::MissionBatches& batches);
    void vehicleMissionRead(const autopilot::MissionBatches& batches, bool ok, bool silent);
    void chartLossesChanged();
    void protoBinConsoledChanged();
    void nmeaConsoledChanged();
    void USBLBeaconDirectAskChanged();

private:
    void onAutopilotState(const AutopilotState& state);
    void onAutopilotStatusText(int severity, const QString& text);
    void onMissionTransferProgress(int done, int total);
    void onMissionUploadFinished(bool ok, int result, int missionType);
    void onMissionDownloadFinished(bool ok, int result, int missionType, const autopilot::MissionBatches& batches);
    bool beginMissionTransfer(bool download, bool silent);
    void endMissionTransfer(bool ok);
    void startWorkerDownload();
    void cancelSilentTransfer();

    std::unique_ptr<DeviceManager> workerObject_;
    std::unique_ptr<QThread> missionThread_;
    autopilot::MissionTransfer* missionTransfer_ = nullptr;
    AutopilotState autopilotState_;
    QVariantList autopilotMessages_;
    bool echogramDeliveryKnown_ = false;
    bool missionTransferActive_ = false;
    bool missionDownloading_ = false;
    bool missionTransferSilent_ = false;
    bool restartMissionAfterUpload_ = true;
    int swallowSilentFinishes_ = 0;
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
