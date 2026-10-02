#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QHash>
#include <QGeoPositionInfoSource>
#include <QUuid>
#include <QTimer>
#include <QElapsedTimer>
#include "link.h"
#include "stream_list.h"
#include "dev_q_property.h"
#include "proto_binnary.h"
#include "id_binnary.h"
#include "mission_transfer.h"


class LocationReader;
struct MAVLink_MSG_STATUSTEXT;

/**
 * Mission execution as the vehicle reports it. seq / total / state / missionId come from
 * MISSION_CURRENT: total is -1 when the vehicle does not report it, 65535 when it has no mission,
 * otherwise the MAVLink total (the last seq; home excluded when the autopilot keeps it in the
 * list); state is MAV MISSION_STATE (0 = not reported); missionId 0 = not supported; reports
 * counts the MISSION_CURRENT messages received, so a reader can tell a fresh report from a cached one.
 * reachedSeq is the last MISSION_ITEM_REACHED. waypointDistance (m) and crossTrackError (m) come
 * from NAV_CONTROLLER_OUTPUT and are valid only while navValid (reset after 3 s without one).
 */
struct MissionTelemetry
{
    int seq = -1;
    int total = -1;
    int state = 0;
    quint32 missionId = 0;
    quint32 reports = 0;
    int reachedSeq = -1;
    bool navValid = false;
    float waypointDistance = NAN;
    float crossTrackError = NAN;
};

/** Copy of the autopilot telemetry published with every change, so readers in other threads never touch DeviceManager's own fields. */
struct AutopilotState
{
    float voltage = NAN;
    float current = NAN;
    float velocityH = NAN;
    int batteryPercent = -1;
    int armState = -1;
    int flightMode = -1;
    bool online = false;
    bool proxyTraffic = false;
    double homeLat = NAN;
    double homeLon = NAN;
    int linkQuality = -1;
    bool radioRssiValid = false;
    int radioRssi = 0;
    double latitude = NAN;
    double longitude = NAN;
    /** GPS_RAW_INT of the latched vehicle: MAVLink GPS_FIX_TYPE (-1 = not received), satellites (-1 = unknown), HDOP (NaN = unknown). */
    int gpsFixType = -1;
    int gpsSatellites = -1;
    double gpsHdop = NAN;
    MissionTelemetry mission;
};

Q_DECLARE_METATYPE(AutopilotState)

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    /*methods*/
    DeviceManager();
    ~DeviceManager() override;

    Q_INVOKABLE float vruVoltage();
    Q_INVOKABLE float vruCurrent();
    Q_INVOKABLE float vruVelocityH();
    Q_INVOKABLE int pilotArmState();
    Q_INVOKABLE int pilotModeState();
    Q_INVOKABLE int vruBatteryPercent();
    Q_INVOKABLE bool autopilotOnline();
    Q_INVOKABLE int autopilotSystemId();
    double vehicleHomeLat();
    double vehicleHomeLon();
    bool proxyLinkActive();
    QList<DevQProperty*> getDevList();
    // True while any connected device has answered the stand probe. The stand panel kind is
    // hidden everywhere this is false, so it has to follow the devices rather than a snapshot.
    bool standAvailable();
    QList<DevQProperty*> getDevList(BoardVersion ver);
    int calcAverageChartLosses();

public slots:
    Q_INVOKABLE bool isCreatedId(int id);
    Q_INVOKABLE StreamListModel* streamsList();
    Q_INVOKABLE void startStreamDownload(int id);
    Q_INVOKABLE void cancelStreamDownload(int id);
    Q_INVOKABLE void refreshStreamList();

    void initStreamList();
    void frameInput(QUuid uuid, Link* link, Parsers::FrameParser frame);
    void openFile(QString filePath);
#ifdef SEPARATE_READING
    void closeFile(bool onOpen = false);
#else
    void closeFile();
#endif
    void onLinkOpened(QUuid uuid, Link *link);
    void onLinkClosed(QUuid uuid, Link* link);
    void onLinkDeleted(QUuid uuid, Link* link);
    void binFrameOut(Parsers::ProtoBinOut protoOut);
    void setProtoBinConsoled(bool isConsoled);
    void setNmeaConsoled(bool isConsoled);
    void upgradeLastDev(QByteArray data);

    void beaconActivationReceive(uint8_t id);
    void beaconDirectQueueAsk();
    bool isbeaconDirectQueueAsk() { return isUSBLBeaconDirectAsk; }
    void setUSBLBeaconDirectAsk(bool is_ask);

    void onLoggingKlfStarted(bool started);
    void onSendRequestAll(QUuid uuid);

    void onStartUpgradingFirmware(QUuid linkUuid, uint8_t address, const QByteArray& firmware);
    void onUpgradingFirmwareDone();

    void createLocationReader();
    void destroyLocationReader();
    void shutdown();

    void onPositionUpdated(const QGeoPositionInfo& info);

    void setUseGPS(bool state);

    void autopilotArm(bool arm, bool force);
    void autopilotSetMode(int customMode);
    void autopilotStartMission();
    /**
     * Makes mission item @p seq (vehicle list index, 0 = home slot) the current one: the vehicle
     * goes to it on the shortest path, skipping the items in between. Sends
     * MAV_CMD_DO_SET_MISSION_CURRENT and falls back to MISSION_SET_CURRENT once the vehicle
     * answers UNSUPPORTED, remembered until the next vehicle binds (QGroundControl behaviour).
     */
    void autopilotSetMissionCurrent(int seq);
    void startMissionUpload(const autopilot::MissionBatches& batches);
    void startMissionDownload();
    void sendAutopilotMessage(quint32 msgId, const QByteArray& payload, int v1Length);

signals:
    void sendFrameInputToLogger(QUuid uuid, Link* link, Parsers::FrameParser frame);
    void writeMavlinkBytes(QByteArray data);
    void autopilotCommandAcked(int command, int result);
    void autopilotStatusText(int severity, const QString& text);
    void autopilotMissionItemReached(int seq);
    void missionFrameReceived(quint32 msgId, const QByteArray& payload);
    void missionUploadStart(const autopilot::MissionBatches& batches, int targetSystem, int targetComponent);
    void missionUploadRefused(int result, int missionType);
    void missionDownloadStart(const QVector<int>& missionTypes, int targetSystem, int targetComponent);
    void missionDownloadRefused(int result);
    void missionTransferAbort(int result);

    //
    void sendChartSetup (const ChannelId& channelId, uint16_t resol, uint16_t count, uint16_t offset);
    void sendTranscSetup(const ChannelId& channelId, uint16_t freq, uint8_t pulse, uint8_t boost);
    void sendSoundSpeeed(const ChannelId& channelId, uint32_t soundSpeed);

    void dataSend(QByteArray data);
    void chartComplete(const ChannelId& channelId, const ChartParameters& chartParams, const QVector<QVector<uint8_t>>& data, float resolution, float offset);
    void rawDataRecieved(const ChannelId& channelId, RawData rawData);
    void distComplete(const ChannelId& channelId, int dist);
    void usblSolutionComplete(IDBinUsblSolution::UsblSolution data);
    void dopplerBeamComlete(IDBinDVL::BeamSolution* beams, uint16_t cnt);
    void dvlSolutionComplete(IDBinDVL::DVLSolution dvlSolution);
    void chartSetupChanged();
    void distSetupChanged();
    void datasetChanged();
    void transChanged();
    void soundChanged();
    void UARTChanged();
    void upgradeProgressChanged(int progressStatus);
    void deviceVersionChanged();
    void devChanged();
    void standAvailableChanged();
    void streamChanged();
    void vruChanged(const AutopilotState& state);
    void writeProxyFrame(Parsers::FrameParser frame);
    void writeMavlinkFrame(Parsers::FrameParser frame);
    void eventComplete(int timestamp, int id, int unixt);
    void rangefinderComplete(const ChannelId& channelId, float distance);
    void positionComplete(double lat, double lon, uint32_t date, uint32_t time);
    void positionCompleteRTK(Position position);
    void depthComplete(float depth);
    void gnssVelocityComplete(double hSpeed, double course);
    void simpleNavV2Complete(uint8_t gnssFixType,
                             uint8_t numSats,
                             uint32_t unixTime,
                             int16_t unixOffsetMs,
                             double latitude,
                             double longitude,
                             double groundCourseDeg,
                             double groundVelocityMps,
                             float yawDeg,
                             float pitchDeg,
                             float rollDeg);
    void boatStatusComplete(uint8_t batteryBoatPercent, uint8_t batteryBridgePercent, uint8_t signalQualityBoatPercent, uint8_t signalQualityBridgePercent);
    void attitudeComplete(float yaw, float pitch, float roll);
    void tempComplete(float val);
    void encoderComplete(float e1, float e2, float e3);
    void fileStopsOpening();
    void chartLossesChanged();

    // logger
    void sendProtoFrame(Parsers::ProtoBinOut protoOut);

#ifdef SEPARATE_READING
    void fileStartOpening();
    void fileBreaked(bool);
    void onFileReadEnough();
#endif
    void fileOpened();

private:
    /*methods*/
    DevQProperty* getDevice(QUuid uuid, Link* link, uint8_t addr);
    void delAllDev();
    void deleteDevicesByLink(QUuid uuid);
    DevQProperty* createDev(QUuid uuid, Link* link, uint8_t addr);
    void sendCommandLong(uint16_t command, float p1 = 0.0f, float p2 = 0.0f, float p3 = 0.0f, float p4 = 0.0f, float p5 = 0.0f, float p6 = 0.0f, float p7 = 0.0f);
    void checkAutopilotOnline();
    void publishVru();
    void resetLinkQuality();
    void countAutopilotFrame(int seq);
    void updateLinkQuality();
    void requestVehicleHome();
    void resetMissionTelemetry();
    void requestAutopilotStreams();
    void sendDataStreamRequest();
    void sendMissionSetCurrent(int seq);
    void handleStatusText(const MAVLink_MSG_STATUSTEXT& message);
    void emitStatusText(int severity, const QString& text);
    void flushStatusText();
    void bindAutopilotLink(QUuid uuid, Link* link);
    void unbindAutopilotLink();
    void resetAutopilot();

    /*data*/
    struct VruData {
        VruData() :
            voltage(NAN),
            current(NAN),
            velocityH(NAN),
            batteryPercent(-1),
            homeLat(NAN),
            homeLon(NAN),
            latitude(NAN),
            longitude(NAN),
            armState(-1),
            flightMode(-1),
            systemId(-1),
            componentId(-1),
            mavlinkVersion(2),
            online(false),
            lastHeartbeatMs(0)
        {};

        void cleanVru()
        {
            voltage = NAN;
            current = NAN;
            velocityH = NAN;
            batteryPercent = -1;
            homeLat = NAN;
            homeLon = NAN;
            latitude = NAN;
            longitude = NAN;
            armState = -1;
            flightMode = -1;
            systemId = -1;
            componentId = -1;
            mavlinkVersion = 2;
            online = false;
            lastHeartbeatMs = 0;
        };

        float voltage;
        float current;
        float velocityH;
        int batteryPercent;
        double homeLat;
        double homeLon;
        double latitude;
        double longitude;
        int armState;
        int flightMode;
        int systemId;
        int componentId;
        int mavlinkVersion;
        bool online;
        qint64 lastHeartbeatMs;
    };

    VruData vru_;
    DevQProperty* lastDevs_;
    DevQProperty* lastDevice_;
    Link* mavlinkLink_;
    Link* autopilotLink_ = nullptr;
    QUuid autopilotLinkUuid_;
    QList<DevQProperty*> devList_;
    QHash<QUuid, QHash<int, DevQProperty*>> devTree_;
    QHash<QUuid, int> otherProtocolStat_;
    StreamList streamList_;
    QUuid lastUuid_;
    QUuid proxyLinkUuid_;
    QUuid mavlinUuid_;
    int lastAddress_;
    int progress_;
    bool isConsoled_;
    bool nmeaConsoled_;
    volatile bool break_;
#ifdef SEPARATE_READING
    bool onOpen_{ false };
#endif

    bool isUSBLBeaconDirectAsk = false;
    bool proxyTraffic_ = false;
    qint64 proxyTrafficMs_ = -1;
    int linkLastSeq_ = -1;
    int linkReceived_ = 0;
    int linkLost_ = 0;
    QVector<QPair<int, int>> linkWindow_;
    int linkQuality_ = -1;
    int radioRssi_ = 0;
    qint64 radioRssiMs_ = -1;
    MissionTelemetry missionTelemetry_;
    qint64 navOutputMs_ = -1;
    int pendingMissionCurrentSeq_ = -1;
    bool missionSetCurrentUnsupported_ = false;
    qint64 lastMissionCurrentMs_ = -1;
    int gpsFixType_ = -1;
    int gpsSatellites_ = -1;
    double gpsHdop_ = NAN;
    qint64 gpsMs_ = -1;
    qint64 streamRequestMs_ = -1;
    int streamRequests_ = 0;
    bool streamIntervalUnsupported_ = false;
    int statusTextId_ = 0;
    int statusTextSeverity_ = 0;
    QString statusText_;
    QTimer beacon_timer;
    QTimer autopilotTimer_{ this };
    QElapsedTimer heartbeatClock_;
    uint8_t mavlinkSeq_ = 0;
    QUuid upgradeUuid_;
    uint8_t upgradeAddr_;
    QByteArray upgradeData_;
    bool loggingStarted_ = false;
    bool autoDownloadStarted_ = false;
    LocationReader* locReader_{ nullptr };
    bool useGPS_{ false };

private slots:
    void readyReadProxy(Link* link);
    void readyReadProxyNav(Link* link);
};
