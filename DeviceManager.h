#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QHash>
#include <QUuid>
#include "Link.h"
#include "streamlist.h"
#include "DevQProperty.h"
#include "ProtoBinnary.h"
#include "IDBinnary.h"

#ifdef MOTOR
#include "motor_control.h"
#endif

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    /*methods*/
    DeviceManager();
    ~DeviceManager();

    Q_INVOKABLE float vruVoltage();
    Q_INVOKABLE float vruCurrent();
    Q_INVOKABLE float vruVelocityH();
    Q_INVOKABLE int pilotArmState();
    Q_INVOKABLE int pilotModeState();

    QList<DevQProperty*> getDevList();
    QList<DevQProperty*> getDevList(BoardVersion ver);

#ifdef MOTOR
    bool isMotorControlCreated() const;
#endif

public slots:
    Q_INVOKABLE bool isCreatedId(int id);
    Q_INVOKABLE StreamListModel* streamsList();

    void frameInput(QUuid uuid, Link* link, FrameParser frame);
    void openFile(QString filePath);
#ifdef SEPARATE_READING
    void closeFile(bool onOpen = false);
#else
    void closeFile();
#endif
    void onLinkOpened(QUuid uuid, Link *link);
    void onLinkClosed(QUuid uuid, Link* link);
    void onLinkDeleted(QUuid uuid, Link* link);
    void binFrameOut(ProtoBinOut protoOut);
    void setProtoBinConsoled(bool isConsoled);
    void upgradeLastDev(QByteArray data);

    void beaconActivationReceive(uint8_t id);
    void beaconDirectQueueAsk();
    bool isbeaconDirectQueueAsk() { return isUSBLBeaconDirectAsk; }
    void setUSBLBeaconDirectAsk(bool is_ask);

#ifdef MOTOR
    float getFAngle();
    float getSAngle();
    void returnToZero(int id);
    void runSteps(int id, int speed, int angle);
    void openCsvFile(QString path);
    void clearTasks();
    void calibrationStandIn(float currFAngle, float taskFAngle, float currSAngle, float taskSAngle);
#endif

signals:
    //
    void channelChartSetupChanged(int16_t channel, uint16_t resol, int count, uint16_t offset);

    void dataSend(QByteArray data);
    void chartComplete(int16_t channel, QVector<uint8_t> data, float resolution, float offset);
    void rawDataRecieved(RawData rawData);
    void distComplete(int dist);
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
    void streamChanged();
    void vruChanged();
    void writeProxyFrame(FrameParser frame);
    void writeMavlinkFrame(FrameParser frame);
    void eventComplete(int timestamp, int id, int unixt);
    void rangefinderComplete(float distance);
    void positionComplete(double lat, double lon, uint32_t date, uint32_t time);
    void gnssVelocityComplete(double hSpeed, double course);
    void attitudeComplete(float yaw, float pitch, float roll);
    void encoderComplete(float e1, float e2, float e3);
    void fileStopsOpening();

#ifdef SEPARATE_READING
    void fileStartOpening();
    void fileBreaked(bool);
    void onFileReadEnough();
#endif
    void fileOpened();

#ifdef MOTOR
    void motorDeviceChanged();
    void anglesHasChanged();
    void posIsConstant(float currFAngle, float taskFAngle, float currSAngle, float taskSAngle);
#endif

private:
    /*methods*/
    DevQProperty* getDevice(QUuid uuid, Link* link, uint8_t addr);
    void delAllDev();
    void deleteDevicesByLink(QUuid uuid);
    DevQProperty* createDev(QUuid uuid, Link* link, uint8_t addr);

    /*data*/
    struct VruData {
        VruData() :
            voltage(NAN),
            current(NAN),
            velocityH(NAN),
            armState(-1),
            flightMode(-1)
        {};

        void cleanVru()
        {
            voltage = NAN;
            current = NAN;
            velocityH = NAN;
            armState = -1;
            flightMode = -1;
        };

        float voltage;
        float current;
        float velocityH;
        int armState;
        int flightMode;
    };

    VruData vru_;
    DevQProperty* lastDevs_;
    DevQProperty* lastDevice_;
    Link* mavlinkLink_;
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
    volatile bool break_;
#ifdef SEPARATE_READING
    bool onOpen_{ false };
#endif

    bool isUSBLBeaconDirectAsk = false;
    QTimer beacon_timer;

#ifdef MOTOR
    std::unique_ptr<MotorControl> motorControl_;
    float fAngle_ = 0.0f;
    float sAngle_ = 0.0f;
#endif

private slots:
    void readyReadProxy(Link* link);
    void readyReadProxyNav(Link* link);
};
