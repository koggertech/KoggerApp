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

public slots:
    Q_INVOKABLE bool isCreatedId(int id);
    Q_INVOKABLE StreamListModel* streamsList();

    void frameInput(QUuid uuid, Link* link, FrameParser frame);
    void openFile(const QString& filePath, bool isAppend);
    void onLinkOpened(QUuid uuid, Link *link);
    void onLinkClosed(QUuid uuid, Link* link);
    void onLinkDeleted(QUuid uuid, Link* link);
    void binFrameOut(ProtoBinOut proto_out);
    void stopConnection();
    void setProtoBinConsoled(bool is_consoled);
    void upgradeLastDev(QByteArray data);

    // proxy
    void openProxyLink(const QString &address, const int port_in,  const int port_out);
    void openProxyNavLink(const QString &address, const int port_in,  const int port_out);
    bool isProxyOpen() { return proxyLink.isOpen(); }
    bool isProxyNavOpen() { return proxyNavLink.isOpen(); }
    void closeProxyLink();
    void closeProxyNavLink();

private slots:
    void readyReadProxy(Link* link);
    void readyReadProxyNav(Link* link);

signals:
    void dataSend(QByteArray data);
    void chartComplete(int16_t channel, QVector<uint8_t> data, float resolution, float offset);
    void iqComplete(QByteArray data, uint8_t type);
    void distComplete(int dist);
    void usblSolutionComplete(IDBinUsblSolution::UsblSolution data);
    void dopplerBeamComlete(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void dvlSolutionComplete(IDBinDVL::DVLSolution dvlSolution);
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


    void eventComplete(int timestamp, int id, int unixt);
    void rangefinderComplete(float distance);
    void positionComplete(double lat, double lon, uint32_t date, uint32_t time);
    void gnssVelocityComplete(double h_speed, double course);
    void attitudeComplete(float yaw, float pitch, float roll);



    void appendOnFileOpening(bool isAppend);

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


    // proxy
    Link proxyLink;
    Link proxyNavLink;


}; // class DeviceWrapper
