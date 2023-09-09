#ifndef DEVICE_H
#define DEVICE_H
#include <QObject>
#include <Link.h>
//#include <ProtoBinnary.h>
#include <QList>
#include <DevQProperty.h>
#include <streamlist.h>
#include <QVariant>
#include <QStringListModel>
#include <QUdpSocket>

class Device : public QObject {
    Q_OBJECT
public:
    Device() {
    }

    Q_PROPERTY(QList<DevQProperty*> devs READ getDevList NOTIFY devChanged)
    Q_PROPERTY(bool protoBinConsoled WRITE setProtoBinConsoled)
    Q_PROPERTY(StreamListModel*  streamsList READ streamsList NOTIFY streamChanged)

    Q_PROPERTY(float vruVoltage READ vruVoltage NOTIFY vruChanged)
    Q_PROPERTY(float vruCurrent READ vruCurrent NOTIFY vruChanged)
    Q_PROPERTY(float vruVelocityH READ vruVelocityH NOTIFY vruChanged)




    float vruVoltage() { return _vru.voltage; }
    float vruCurrent() { return _vru.current; }
    float vruVelocityH() { return _vru.velocityH; }

    QList<DevQProperty*> getDevList() {
        _devList.clear();

        for(uint16_t i = 1; i < 256; i++) {
            if(devSort[i] != NULL) {
                _devList.append(devSort[i]);
            } else {
                break;
            }
        }

        if(devSort[0] != NULL) {
            _devList.append(devSort[0]);
        }
        return _devList;
    }

    DevQProperty* getLastDev() {
        return lastDevs;
    }

public slots:
    void putData(const QByteArray &data);
    void binFrameOut(ProtoBinOut &proto_out);
    void startConnection(bool duplex);
    void stopConnection();
    bool isCreatedId(int id) { return getDevList().size() > id; }
    void setProtoBinConsoled(bool is_consoled) { _isConsoled = is_consoled; }
    void upgradeLastDev(QByteArray data);

    void openProxyLink(const QString &address, const int port_in,  const int port_out);
    void openProxyNavLink(const QString &address, const int port_in,  const int port_out);

    bool isProxyOpen() { return proxyLink.isOpen(); }
    bool isProxyNavOpen() { return proxyNavLink.isOpen(); }

    void closeProxyLink();
    void closeProxyNavLink();

    StreamListModel*  streamsList() {
        return _streamList.streamsList();
    }

signals:
    void dataSend(QByteArray data);

    void chartComplete(int16_t channel, QVector<int16_t> data, float resolution, float offset);
    void iqComplete(QByteArray data, uint8_t type);
    void attitudeComplete(float yaw, float pitch, float roll);
    void distComplete(int dist);
    void dopplerBeamComlete(IDBinDVL::BeamSolution *beams, uint16_t cnt);
    void dvlSolutionComplete(IDBinDVL::DVLSolution dvlSolution);
    void positionComplete(uint32_t date, uint32_t time, double lat, double lon);
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

protected:
    FrameParser _parser;

    DevQProperty* devAddr[256] = {};
    DevQProperty* devSort[256] = {};

    DevQProperty* lastDevs = NULL;
    uint8_t lastRoute = 0;

    QList<DevQProperty*> _devList;
    StreamList _streamList;

    Link proxyLink;
    Link proxyNavLink;

    bool _isDuplex = false;
    bool _isConsoled = false;

    struct {
        float voltage = NAN;
        float current = NAN;
        float velocityH = NAN;
    } _vru;

    void delAllDev() {
        lastRoute = 0;
        lastDevs = NULL;

        for(uint16_t i = 0; i < 256; i++) {
            if(devAddr[i] != NULL) {
                devAddr[i]->stopConnection();
                devAddr[i]->disconnect(this);
                delete devAddr[i];
                devAddr[i] = NULL;
            }

            devSort[i] = NULL;
        }

        emit devChanged();
    }

    void createDev(uint8_t addr, bool duplex) {
        devAddr[addr] = new DevQProperty();
        devAddr[addr]->setBusAddress(addr);

        for(uint16_t i = 0; i < 256; i++) {
            if(devSort[i] == NULL) {
                devSort[i] = devAddr[addr];
                break;
            }
        }

        connect(devAddr[addr], &DevQProperty::binFrameOut, this, &Device::binFrameOut);
        connect(devAddr[addr], &DevQProperty::chartComplete, this, &Device::chartComplete);
        connect(devAddr[addr], &DevQProperty::iqComplete, this, &Device::iqComplete);
        connect(devAddr[addr], &DevQProperty::attitudeComplete, this, &Device::attitudeComplete);
        connect(devAddr[addr], &DevQProperty::distComplete, this, &Device::distComplete);
        connect(devAddr[addr], &DevQProperty::dopplerBeamComplete, this, &Device::dopplerBeamComlete);
        connect(devAddr[addr], &DevQProperty::dvlSolutionComplete, this, &Device::dvlSolutionComplete);
        connect(devAddr[addr], &DevQProperty::upgradeProgressChanged, this, &Device::upgradeProgressChanged);

        lastDevs = devAddr[addr];
        lastRoute = addr;

        devAddr[addr]->startConnection(duplex);
        emit devChanged();
    }

    void gatewayKP();
    void gatewayUBX();
    void gatewayNMEA();
    void gatewayMAVLink();

signals:
    void writeProxyFrame(FrameParser *frame);
    void writeProxy(QByteArray data);
    void writeProxyNavFrame(FrameParser *frame);
    void writeProxyNav(QByteArray data);

private slots:
    void readyReadProxy(Link* link);
    void readyReadProxyNav(Link* link);
};

#endif // DEVICE_H
