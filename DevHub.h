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
    Device();
    ~Device();

    Q_PROPERTY(QList<DevQProperty*> devs READ getDevList NOTIFY devChanged)
    Q_PROPERTY(bool protoBinConsoled WRITE setProtoBinConsoled)
    Q_PROPERTY(StreamListModel*  streamsList READ streamsList NOTIFY streamChanged)

    Q_PROPERTY(float vruVoltage READ vruVoltage NOTIFY vruChanged)
    Q_PROPERTY(float vruCurrent READ vruCurrent NOTIFY vruChanged)
    Q_PROPERTY(float vruVelocityH READ vruVelocityH NOTIFY vruChanged)
    Q_PROPERTY(int pilotArmState READ pilotArmState NOTIFY vruChanged)
    Q_PROPERTY(int pilotModeState READ pilotModeState NOTIFY vruChanged)



    float vruVoltage() { return _vru.voltage; }
    float vruCurrent() { return _vru.current; }
    float vruVelocityH() { return _vru.velocityH; }
    int pilotArmState() { return _vru.armState; }
    int pilotModeState() { return _vru.flight_mode; }

    QList<DevQProperty*> getDevList() {
        _devList.clear();

        for (auto i = _devTree.cbegin(), end = _devTree.cend(); i != end; ++i) {
            QHash<int, DevQProperty*> devs = i.value();

            for (auto k = devs.cbegin(), end = devs.cend(); k != end; ++k) {
                _devList.append(k.value());
            }
        }

        // for(uint16_t i = 1; i < 256; i++) {
        //     if(devSort[i] != NULL) {
        //         _devList.append(devSort[i]);
        //     } else {
        //         break;
        //     }
        // }

        // if(devSort[0] != NULL) {
        //     _devList.append(devSort[0]);
        // }
        return _devList;
    }

    DevQProperty* getLastDev() {
        return lastDevs;
    }

public slots:
    void frameInput(QUuid uuid, Link* link, FrameParser frame);
    void openFile(const QString& filePath);
    void onLinkOpened(QUuid uuid, Link *link);
    void onLinkClosed(QUuid uuid, Link* link);
    void onLinkDeleted(QUuid uuid, Link* link);
    void binFrameOut(ProtoBinOut proto_out);
    // void startConnection(bool duplex);
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

    void chartComplete(int16_t channel, QVector<uint8_t> data, float resolution, float offset);
    void iqComplete(QByteArray data, uint8_t type);
    void attitudeComplete(float yaw, float pitch, float roll);
    void distComplete(int dist);
    void usblSolutionComplete(IDBinUsblSolution::UsblSolution data);
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
    QHash<QUuid, QHash<int, DevQProperty*>> _devTree;

    DevQProperty* lastDevs = NULL;
    int lastRoute = -1;
    QUuid lastUid_;
    int lastAddress_ = -1;
    DevQProperty* lastDevice_ = NULL;

    QList<DevQProperty*> _devList;
    StreamList _streamList;

    Link proxyLink;
    Link proxyNavLink;

    // bool _isDuplex = true;
    bool _isConsoled = false;

    struct {
        float voltage = NAN;
        float current = NAN;
        float velocityH = NAN;
        int armState = -1;
        int flight_mode = -1;
    } _vru;


    volatile bool break_ = false;
    int progress_ = 0;


    DevQProperty* getDevice(QUuid uuid, Link* link, uint8_t addr) {
        if((link == NULL || lastUid_ == uuid) && lastAddress_ == addr && lastDevice_ != NULL) {
            return lastDevice_;
        } else {
            lastDevice_ = _devTree[uuid][addr];
            if(lastDevice_ == NULL) {
                lastDevice_ = createDev(uuid, link, addr);
            }
            lastUid_ = uuid;
            lastAddress_ = addr;
        }

        return lastDevice_;
    }

    void delAllDev() {
        for (auto i = _devTree.cbegin(), end = _devTree.cend(); i != end; ++i) {
            deleteDevicesByLink(i.key());
        }
    }

    void deleteDevicesByLink(QUuid uuid) {
        if(_devTree.contains(uuid)) {
            QHash<int, DevQProperty*> devs = _devTree[uuid];
            for (auto i = devs.cbegin(), end = devs.cend(); i != end; ++i) {
                if(lastDevice_ == i.value()) {
                    lastDevice_ = NULL;
                }
                disconnect(i.value());
                delete i.value();
            }
            _devTree[uuid].clear();
            _devTree.remove(uuid);
            emit devChanged();
        }
    }

    DevQProperty* createDev(QUuid uuid, Link* link, uint8_t addr) {
        DevQProperty* dev = new DevQProperty();
        _devTree[uuid][addr] = dev;
        dev->setBusAddress(addr);

        if(link != NULL) {
            connect(dev, &DevQProperty::binFrameOut, this, &Device::binFrameOut);
            connect(dev, &DevQProperty::binFrameOut, link, &Link::writeFrame);
        }

        connect(dev, &DevQProperty::chartComplete, this, &Device::chartComplete);
        connect(devAddr[addr], &DevQProperty::rawDataRecieved, this, &Device::rawDataRecieved);
        connect(dev, &DevQProperty::attitudeComplete, this, &Device::attitudeComplete);
        connect(dev, &DevQProperty::distComplete, this, &Device::distComplete);
        connect(dev, &DevQProperty::usblSolutionComplete, this, &Device::usblSolutionComplete);
        connect(dev, &DevQProperty::dopplerBeamComplete, this, &Device::dopplerBeamComlete);
        connect(dev, &DevQProperty::dvlSolutionComplete, this, &Device::dvlSolutionComplete);
        connect(dev, &DevQProperty::upgradeProgressChanged, this, &Device::upgradeProgressChanged);

        dev->startConnection(link != NULL);

        emit devChanged();

        return dev;
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
