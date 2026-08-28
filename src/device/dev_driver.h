#pragma once

#include <QObject>
#include <QHash>
#include <QVector>
#include <QTimer>
#include <QUuid>
#include <QVariantMap>
#include "proto_binnary.h"
#include "id_binnary.h"


using namespace Parsers;

class DevDriver : public QObject
{
    Q_OBJECT
public:
    explicit DevDriver(QObject *parent = nullptr);
    ~DevDriver() override;
    typedef enum : quint8 {
        DatasetOff = 0,
        DatasetCh1 = 1,
        DatasetCh2 = 2,
        DatasetRequest = 255
    } DatasetChannel;

    enum UpgradeStatus : qint8 {
        failUpgrade = -1,
        successUpgrade = 101
    };

#ifdef SEPARATE_READING
    QTimer* getProcessTimer();
    QList<QTimer*> getChildTimers();
#endif

    ChannelId getChannelId() const;

    int distMax();
    void setDistMax(int dist);

    int distDeadZone();
    void setDistDeadZone(int dead_zone);

    int distConfidence();
    void setConfidence(int confidence);

    int chartSamples();
    void setChartSamples(int samples);

    int chartResolution();
    void setChartResolution(int resol);

    int chartOffset();
    void setChartOffset(int offset);

    int dspSmoothFactor();
    void setDspSmoothFactor(int dsp_smooth);

    int datasetTimestamp();
    void setDatasetTimestamp(int ch_param);
    int datasetDist();
    void setDatasetDist(int ch_param);
    int datasetChart();
    void setDatasetChart(int ch_param);
    int datasetTemp();
    void setDatasetTemp(int ch_param);
    int datasetEuler();
    void setDatasetEuler(int ch_param);
    int datasetSDDBT();
    void setDatasetSDDBT(int ch_param);
    int datasetSDDBT_P2();
    void setDatasetSDDBT_P2(int ch_param);

    int ch1Period();
    void setCh1Period(int period);

    int ch2Period();
    void setCh2Period(int period);

    void sendUpdateFW(QByteArray update_data);
    bool isUpdatingFw() { return m_state.in_update; }
    int upgradeFWStatus() {return m_upgrade_status; }

    void sendFactoryFW(QByteArray update_data);

    int transFreq();
    void setTransFreq(int freq);

    int transPulse();
    void setTransPulse(int pulse);

    int transBoost();
    void setTransBoost(int boost);

    int soundSpeed();
    void setSoundSpeed(int speed);

    float yaw();
    float pitch();
    float roll();

    void setBusAddress(int addr);
    int getBusAddress();

    void setDevAddress(int addr);
    int getDevAddress();

    void setBaudrate(int baudrate);
    int getBaudrate();

    void setDevDefAddress(int addr);
    int getDevDefAddress();

    QString devName() { return m_devName; }
    int devType() const { return static_cast<int>(idVersion->boardVersion()); }
    uint32_t devSerialNumber();
    QString devPN();

    QString fwVersion() { return m_fwVer; }

    // Recorder status (ID_RECORDER_STATUS 0x26 / RecorderStatusV0). See docs
    // Recorder-Host-Integration-Guide.md for field meaning.
    bool recorderStatusValid()           { return idRecorderStatus && idRecorderStatus->isValid(); }
    int  recorderDeviceCondition()       { return idRecorderStatus ? idRecorderStatus->deviceCondition() : 0; }
    int  recorderRecordingMode()         { return idRecorderStatus ? idRecorderStatus->recordingMode() : 0; }
    int  recorderRecordingState()        { return idRecorderStatus ? idRecorderStatus->recordingState() : 0; }
    int  recorderStatusFlags()           { return idRecorderStatus ? idRecorderStatus->statusFlags() : 0; }
    int  recorderWarningFlags()          { return idRecorderStatus ? idRecorderStatus->warningFlags() : 0; }
    int  recorderDegradedFlags()         { return idRecorderStatus ? idRecorderStatus->degradedFlags() : 0; }
    int  recorderCriticalFlags()         { return idRecorderStatus ? idRecorderStatus->criticalFlags() : 0; }
    int  recorderCurrentLogId()          { return idRecorderStatus ? idRecorderStatus->currentLogId() : 0; }
    int  recorderRecordedSize64k()       { return idRecorderStatus ? idRecorderStatus->recordedSize64k() : 0; }
    int  recorderFreeSpace1m()           { return idRecorderStatus ? idRecorderStatus->freeSpace1m() : 0; }
    int  recorderDurationSeconds()       { return idRecorderStatus ? idRecorderStatus->recordingDurationSeconds() : 0; }
    int  recorderSecondsSinceLastWrite() { return idRecorderStatus ? idRecorderStatus->secondsSinceLastWrite() : 0; }

    BoardVersion boardVersion() {
        return idVersion->boardVersion();
    }

    bool isBoardInited() { return boardVersion() != BoardNone; }

    bool isSonar() {
        BoardVersion ver = boardVersion();
        return ver == BoardBase || ver == BoardNBase || ver == BoardEnhanced || ver == BoardChirp || ver == BoardNEnhanced || ver == BoardSideEnhanced || ver == BoardBasic2D || ver == BoardNanoSSS || ver == BoardPULSEred_2D || ver == BoardPULSEblue_DSS;
    }

    bool isRecorder() {
        BoardVersion ver = boardVersion();
        return ver == BoardRecorderMini;
    }

    bool isDoppler() {
        BoardVersion ver = boardVersion();
        return ver == BoardDVL;
    }

    bool isUSBLBeacon() {
        BoardVersion ver = boardVersion();
        return ver == BoardUSBLBeacon;
    }

    bool isUSBL() {
        BoardVersion ver = boardVersion();
        return ver == BoardUSBL;
    }

    bool isChartSupport() { return m_state.duplex && isSonar(); }
    bool isDistSupport() { return m_state.duplex && isSonar(); }
    bool isDSPSupport() { return m_state.duplex && isSonar(); }
    bool isTransducerSupport() { return m_state.duplex && isSonar(); }
    bool isDatasetSupport() { return m_state.duplex && isSonar(); }
    bool isSoundSpeedSupport() { return m_state.duplex && isSonar(); }
    bool isAddressSupport() { return m_state.duplex; }
    bool isUpgradeSupport() { return m_state.duplex; }

    bool getDatasetState() { return datasetState_; };
    bool getDistSetupState() { return distSetupState_; };
    bool getChartSetupState() { return chartSetupState_; };
    bool getDspSetupState() { return dspSetupState_; };
    bool getTranscState() { return transcState_; };
    bool getSoundSpeedState() { return soundSpeedState_; };
    bool getUartState() { return uartState_; };
    bool getServoControlState() { return servoControlState_; };
    bool getStandState() { return standSupported_; };
    bool getPwmRouteState() { return pwmRouteState_; };
    bool getDevSyncState() { return devSyncState_; };
    int getAverageChartLosses() const { return averageChartLosses_; };
    QUuid getLinkUuid() const;
    void setFirmware(const QByteArray& data);

    // Link status mirror — pushed by DeviceManager.
    bool linkConnected()    const { return linkConnected_; }
    bool linkReceivesData() const { return linkReceivesData_; }
    bool linkNotAvailable() const { return linkNotAvailable_; }
    void setLinkStatus(bool connected, bool receivesData, bool notAvailable);

signals:
    void averageChartLossesChanged();
    void binFrameOut(Parsers::ProtoBinOut proto_out);

    // link
    void startUpgradingFirmware();
    void upgradingFirmwareDone();
    // deviceManager
    void startUpgradingFirmwareDM(QUuid linkUuid, uint8_t address, QByteArray firmware);
    void upgradingFirmwareDoneDM();

    //
    void sendChartSetup(const ChannelId& channelId, uint16_t resol, uint16_t count, uint16_t offset);
    void sendTranscSetup(const ChannelId& channelId, uint16_t freq, uint8_t pulse, uint8_t boost);
    void sendSoundSpeed(const ChannelId& channelId, uint32_t soundSpeed);

    void chartComplete(const ChannelId& channelId, const ChartParameters& chartsParams, const QVector<QVector<uint8_t>>& data, float resolution, float offset);
    void rawDataRecieved(const ChannelId& channelId, RawData raw_data);

    void iqComplete(QByteArray data, uint8_t type);
    void attitudeComplete(float yaw, float pitch, float roll);
    void tempComplete(float val);
    void distComplete(const ChannelId& channelId, int dist);
    void encoderComplete(float e1, float e2, float e3);

    void usblSolutionComplete(IDBinUsblSolution::UsblSolution data);
    void acousticNavSolutionComplete(IDBinUsblSolution::AcousticNavSolution data);
    void baseToBeaconComplete(IDBinUsblSolution::BaseToBeacon data);
    void beaconActivationComplete(uint8_t id);
    void modemSolutionComplete(IDBinModemSolution::ModemSolutionHeader header, QByteArray payload);
    // Argument-free companion so DevQProperty can use it as a Q_PROPERTY NOTIFY
    // (same split as recorderStatusChanged).
    void modemPayloadChanged();

    void positionComplete(double lat, double lon, uint32_t date, uint32_t time);
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
    void depthComplete(float depth);
    void chartSetupChanged();
    void dspSetupChanged();
    void distSetupChanged();
    void datasetChanged();
    void transChanged();
    void soundChanged();
    void UARTChanged();
    void servoControlChanged();
    void standChanged();
    void pwmRouteChanged();
    void servoCurrentAngleChanged();
    void devSyncChanged();
    void devSyncErrorOccurred(QString reason);
    void linkStatusChanged();
    void upgradeProgressChanged(int progress_status);
    void upgradeChanged();
    void deviceVersionChanged();
    void deviceIDChanged(QByteArray uid);
    void onReboot();
    void recorderStatusChanged();

    void dopplerVeloComplete();
    void dopplerBeamComplete(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void dvlSolutionComplete(IDBinDVL::DVLSolution dvlSolution);

public slots:
    void protoComplete(Parsers::FrameParser& proto);
    void startConnection(bool duplex);
    void stopConnection();
    void restartState();

    void requestDist();
    void requestChart();

    void requestStreamList();
    void requestStream(int stream_id);
    void requestStreamRange(int stream_id, quint32 start, quint32 end);
    void requestStreamRanges(int stream_id, QVector<quint32> ranges);
    void requestRecorderStatus();

    void setConsoleOut(bool is_console);

    void flashSettings();
    void resetSettings();
    void reboot();
    void process();

    void dvlChangeMode(bool ismode1, bool ismode2, bool ismode3, bool ismode4, float range_mode4);

    void importSettingsFromXML(const QString& file_path);
    void exportSettingsToXML(const QString& file_path);

    void setDatasetState(bool state);
    void setDistSetupState(bool state);
    void setChartSetupState(bool state);
    void setDspSetupState(bool state);
    void setTranscState(bool state);
    void setSoundSpeedState(bool state);
    void setUartState(bool state);
    void setServoControlState(bool state);

    // The stand's whole command surface. Start carries the configuration because the device
    // takes it no other way; the rest are control-only and read nothing from the map.
    void standStart(const QVariantMap& config);
    void standStop();
    void standPause();
    void standResume();
    void standHome();

    void setPwmRouteState(bool state);
    void setDevSyncState(bool state);
    void setDevSyncPeriodMs(int ms);
    void setDevSyncPortSource(int idx, int src);
    void setLinkUuid(QUuid linkUuid);
    void askBeaconPosition() {
        IDBinUsblSolution::USBLRequestBeacon ask;
        askBeaconPosition(ask);
    }
    void askBeaconPosition(IDBinUsblSolution::USBLRequestBeacon ask);
    void enableBeaconOnce(float timeout);

    void acousticPingRequest(uint8_t address, uint32_t timeout_us = 0xFFFFFFFF);
    void acousticPingRequestEx(uint8_t address, uint32_t timeout_us, uint8_t cmdId, uint32_t replyDistanceMm, const QByteArray& payload = {});
    void acousticResponceFilter(uint8_t address);
    void acousticResponceFilterSlots(const QVector<int>& addresses);
    void acousticResponceTimeout(uint32_t timeout_us = 0xFFFFFFFF);

    void setUsblTransponderEnable(bool enabled);
    void setUsblMonitorConfig(uint32_t suppressSelfResponseUs, uint32_t suppressSelfRequestUs, bool receiveResponseInIdle);
    // The only per-slot write there is. v6 USBLCmdConfig carries a receiver_function AND a
    // sender_function, so one frame configures both directions of a command slot.
    //
    // There is no "disable" or "stay silent": current firmware dropped those Function
    // values along with USBLCmdSlotConfig. All-default arguments are the closest thing —
    // the slot still answers, it just handles no payload. Per-device silence is
    // setUsblTransponderEnable(false); per-address filtering is acousticResponceFilterSlots().
    void setUsblCmdConfig(int cmdId, int event,
                          int receiverFunction, int receiveBitLength,
                          int senderFunction, const QString& sendHexPayload,
                          int eventAction = 0,
                          int cmdIdAction = 0, int cmdIdReplacement = 0,
                          int addressAction = 0, int addressReplacement = 0);
    QString modemLastPayload() const;

#ifdef SEPARATE_READING
    Q_INVOKABLE void initProcessTimerConnects();
    Q_INVOKABLE void initChildsTimersConnects();
#endif

    void doRequestAll();

protected:
    friend class DeviceManager;

    typedef void (DevDriver::* ParseCallback)(Type type, Version ver, Resp resp);

    //FrameParser* m_proto;

    IDBinTimestamp* idTimestamp = nullptr;
    IDBinDist* idDist = nullptr;
    IDBinChart* idChart = nullptr;
    IDBinAttitude* idAtt = nullptr;
    IDBinTemp* idTemp = nullptr;
    IDBinEncoder* idEncoder = nullptr;

    IDBinDataset* idDataset = nullptr;
    IDBinDistSetup* idDistSetup = nullptr;
    IDBinChartSetup* idChartSetup = nullptr;
    IDBinDSPSetup* idDSPSetup = nullptr;
    IDBinTransc* idTransc = nullptr;
    IDBinSoundSpeed* idSoundSpeed = nullptr;
    IDBinUART* idUART = nullptr;

    IDBinVersion* idVersion = nullptr;
    IDBinMark* idMark = nullptr;
    IDBinFlash* idFlash = nullptr;
    IDBinBoot* idBoot = nullptr;
    IDBinUpdate* idUpdate = nullptr;

    IDBinNav* idNav = nullptr;
    IDBinBoatStatus* idBoatStatus = nullptr;
    IDBinRecorderStatus* idRecorderStatus = nullptr;
    IDBinDVL* idDVL = nullptr;
    IDBinDVLMode* idDVLMode = nullptr;

    IDBinUsblSolution* idUSBL = nullptr;
    IDBinUsblControl* idUSBLControl = nullptr;
    IDBinModemSolution* idModemSolution = nullptr;

    IDBinServoControl* idServoControl = nullptr;
    IDBinStandScan* idStandScan = nullptr;
    IDBinPwmRoute* idPwmRoute = nullptr;
    IDBinDevSync* idDevSync = nullptr;

//    QHash<ID, IDBin*> hashIDParsing;
//    QHash<ID, ParseCallback> hashIDCallback;
//    QHash<ID, IDBin*> hashIDSetup;

    typedef struct ID_Instance {
        ID_Instance() {
            instance = nullptr;
            callback = nullptr;
            isSetup = false;
        }

        ID_Instance(IDBin* inst, ParseCallback call, bool is_setup = false) {
            instance = inst;
            callback = call;
            isSetup = is_setup;
        }
        IDBin* instance = nullptr;
        ParseCallback callback = nullptr;
        bool isSetup = false;
    } ID_Instance;

    QHash<ID, ID_Instance> _hashID;

    typedef enum : quint8 {
        ConfNone = 0,
        ConfRequest,
        ConfRx
    } ConfStatus;

    typedef enum : quint8 {
        UptimeNone,
        UptimeRequest,
        UptimeFix
    } UptimeStatus;

    struct {
        bool duplex = false;

        bool connect = false;
        bool heartbeat = false;
        bool mark = false;
        bool in_boot = false;
        bool reboot = false;
        bool in_update = false;

        ConfStatus conf = ConfNone;
        UptimeStatus uptime = UptimeNone;

        int64_t lastConnectTime = 0;

        void resetState() {
            connect = false;
            heartbeat = false;
            mark = false;
            // in_boot = false;
            reboot = false;
            in_update = false;

            conf = ConfNone;
            uptime = UptimeNone;
        }
    } m_state;

    uint8_t lastAddress_ = 0;

    QTimer m_processTimer;

    bool m_bootloaderLagacyMode = true;
    bool rebootFlag_ = false;
    int m_upgrade_status = 0;
    int64_t _lastUpgradeAnswerTime = 0;
    int64_t _timeoutUpgradeAnswerTime = 0;
    int64_t upgradeStartedTime_ = 0;
    int64_t rebootAtTime_ = 0;
    int upgradeResendCount_ = 0;

    static constexpr int64_t staleVersionGuardMsec = 400;
    static constexpr int64_t bootHandshakeTimeoutMsec = 8000;
    static constexpr int64_t packetAnswerTimeoutMsec = 2000;
    static constexpr int upgradeResendLimit = 5;
    bool m_isConsole = false;

    int m_busAddress = 0;
    int m_devAddress = 0;
    int m_devDefAddress = 0;

    QString m_devName = "...";
    QString m_fwVer = "";

    void regID(IDBin* id_bin, ParseCallback method, bool is_setup = false);
    void requestSetup();

    void fwUpgradeProcess();
    bool checkUpgradeTimeouts(int64_t curr_time);
    void abortUpgrade(const QString& reason);
    bool isStaleVersionAfterReboot() const;

    // Tolerant hex text → bytes: accepts separators and an odd digit count, which is what a
    // hand-typed payload field produces. Deliberately outside the slots section — it is a
    // static helper, not something to invoke from a connection.
    static QByteArray parseHexPayload(const QString& text);

protected slots:
    void receivedTimestamp  (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedDist       (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedChart      (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedRaw        (RawData raw_data);
    void receivedAtt        (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedTemp       (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedEncoder    (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);

    void receivedDataset    (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedDistSetup  (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedChartSetup (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedDSPSetup   (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedTransc     (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedSoundSpeed (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedUART       (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);

    void receivedVersion    (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedMark       (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedFlash      (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedBoot       (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedUpdate     (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);

    void receivedNav        (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedBoatStatus (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedRecorderStatus(Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedDVL        (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedDVLMode    (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);

    void receivedUSBL       (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedUSBLControl(Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedModemSolution(Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);

    void receivedServoControl(Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedStandScan   (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void receivedPwmRoute    (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);

    void receivedDevSync     (Parsers::Type type, Parsers::Version ver, Parsers::Resp resp);
    void onDevSyncDebounceFired();

private:
    bool datasetState_;
    bool distSetupState_;
    bool chartSetupState_;
    bool dspSetupState_;
    bool transcState_;
    bool soundSpeedState_;
    bool uartState_;
    bool servoControlState_ = false;
    // The stand is discovered by probing, not declared: the command family is control-only, so
    // there is no readback to sync and no board version that separates a stand build from a
    // servo one. Both flags reset with the connection — a device may come back reflashed.
    bool standSupported_ = false;
    bool standProbeSent_ = false;
    bool pwmRouteState_ = false;
    bool devSyncState_ = false;
    QTimer m_devSyncDebounceTimer;
    int errorFreezeCnt_;
    int averageChartLosses_;
    QUuid linkUuid_;
    bool linkConnected_    = false;
    bool linkReceivesData_ = false;
    bool linkNotAvailable_ = false;
    int64_t lastRecorderStatusReq_ = 0;
    bool streamListRequested_ = false;
};
