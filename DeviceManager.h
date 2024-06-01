#pragma once

#include <QObject>
#include <QList>
#include <QVariant>
#include <QStringListModel>
#include <QUdpSocket>
#include <QThread>

#include "Link.h"
//#include "FileReader.h"
#include "streamlist.h"
#include "DevQProperty.h"
//#include "ProtoBinnary.h"
//#include "core.h"


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
    DevQProperty* getLastDev();

    /*QML*/
    Q_PROPERTY(QList<DevQProperty*> devs READ getDevList NOTIFY devChanged)
    Q_PROPERTY(bool protoBinConsoled WRITE setProtoBinConsoled)
    Q_PROPERTY(StreamListModel* streamsList READ streamsList NOTIFY streamChanged)
    Q_PROPERTY(float vruVoltage READ vruVoltage NOTIFY vruChanged)
    Q_PROPERTY(float vruCurrent READ vruCurrent NOTIFY vruChanged)
    Q_PROPERTY(float vruVelocityH READ vruVelocityH NOTIFY vruChanged)
    Q_PROPERTY(int pilotArmState READ pilotArmState NOTIFY vruChanged)
    Q_PROPERTY(int pilotModeState READ pilotModeState NOTIFY vruChanged)
    //Q_PROPERTY(int fileReaderProgress READ getFileReaderProgress NOTIFY fileReaderProgressChanged) //

public slots:
    Q_INVOKABLE bool isCreatedId(int id);
    Q_INVOKABLE StreamListModel* streamsList();

    void frameInput(QUuid uuid, Link* link, FrameParser frame);
    void openFile(const QString& filePath);
    void onLinkOpened(QUuid uuid, Link *link);
    void onLinkClosed(QUuid uuid, Link* link);
    void onLinkDeleted(QUuid uuid, Link* link);
    void binFrameOut(ProtoBinOut proto_out);
    void stopConnection();
    void setProtoBinConsoled(bool is_consoled);
    void upgradeLastDev(QByteArray data);

private slots:
    void readyReadProxy(Link* link);
    void readyReadProxyNav(Link* link);

signals:
    void dataSend(QByteArray data);
    void chartComplete(int16_t channel, QVector<uint8_t> data, float resolution, float offset);
    void iqComplete(QByteArray data, uint8_t type);
    void attitudeComplete(float yaw, float pitch, float roll);
    void distComplete(int dist);
    void rangefinderComplete(float distance);
    void usblSolutionComplete(IDBinUsblSolution::UsblSolution data);
    void dopplerBeamComlete(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void dvlSolutionComplete(IDBinDVL::DVLSolution dvlSolution);
    void positionComplete(uint32_t date, uint32_t time, double lat, double lon);
    void gnssVelocityComplete(double h_speed, double course);
    void eventComplete(int timestamp, int id, int unixt);
    void chartSetupChanged();
    void distSetupChanged();
    void datasetChanged();
    void transChanged();
    void soundChanged();
    void UARTChanged();
    void upgradeProgressChanged(int progress_status);
    void deviceVersionChanged();
    void devChanged();
    void streamChanged();
    void vruChanged();
    void writeProxyFrame(FrameParser *frame);
    void writeProxy(QByteArray data);
    void writeProxyNavFrame(FrameParser *frame);
    void writeProxyNav(QByteArray data);

private:
    /*methods*/
    DevQProperty* getDevice(QUuid uuid, Link* link, uint8_t addr);
    void delAllDev();
    void deleteDevicesByLink(QUuid uuid);
    DevQProperty* createDev(QUuid uuid, Link* link, uint8_t addr);

    /*data*/
    struct {
        float voltage = NAN;
        float current = NAN;
        float velocityH = NAN;
        int armState = -1;
        int flight_mode = -1;
    } vru_;
    QHash<QUuid, QHash<int, DevQProperty*>> devTree_;
    QUuid lastUuid_;
    DevQProperty* lastDevs_;
    DevQProperty* lastDevice_;
    QList<DevQProperty*> devList_;
    StreamList streamList_;
    int lastAddress_;
    int progress_;
    bool isConsoled_;
    volatile bool break_;
}; // class DeviceWrapper
