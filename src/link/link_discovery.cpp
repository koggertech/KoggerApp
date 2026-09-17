#include "link_discovery.h"

#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QVariant>

#include "link_manager_wrapper.h"
#include "proto_binnary.h"
#include "id_binnary.h"

using namespace Parsers;

namespace {

constexpr int kDefaultProbePort = 14444;
constexpr int kDefaultDurationMs = 1500;
constexpr int kSendCount = 3;
constexpr int kSendIntervalMs = 400;
constexpr uint32_t kUnprogrammedSerial = 0xFFFFFFFFu;

}

LinkDiscovery::LinkDiscovery(LinkManagerWrapper* linkManager, QObject* parent)
    : QObject(parent),
      linkManager_(linkManager),
      ports_({kDefaultProbePort}),
      durationMs_(kDefaultDurationMs),
      sendsLeft_(0),
      running_(false)
{
    sendTimer_.setInterval(kSendIntervalMs);
    finishTimer_.setSingleShot(true);
    QObject::connect(&sendTimer_, &QTimer::timeout, this, &LinkDiscovery::sendProbes);
    QObject::connect(&finishTimer_, &QTimer::timeout, this, &LinkDiscovery::finish);
}

LinkDiscovery::~LinkDiscovery()
{
    sendTimer_.stop();
    finishTimer_.stop();
    closeProbes();
}

bool LinkDiscovery::running() const
{
    return running_;
}

QVariantList LinkDiscovery::found() const
{
    QVariantList out;
    for (const Host& host : hosts_) {
        QVariantList devices;
        for (const DeviceInfo& dev : host.devices) {
            QVariantMap d;
            d.insert(QStringLiteral("route"), dev.route);
            d.insert(QStringLiteral("name"), boardVersionName(static_cast<BoardVersion>(dev.board), dev.boardMinor));
            d.insert(QStringLiteral("board"), dev.board);
            d.insert(QStringLiteral("boardMinor"), dev.boardMinor);
            d.insert(QStringLiteral("serialNumber"), dev.serialNumber == kUnprogrammedSerial ? 0u : dev.serialNumber);
            d.insert(QStringLiteral("fwVersion"), dev.hasVersion2 ? QStringLiteral("%1.%2").arg(dev.fwMajor).arg(dev.fwMinor) : QString());
            devices.append(d);
        }
        QVariantMap h;
        h.insert(QStringLiteral("address"), host.address.toString());
        h.insert(QStringLiteral("port"), host.devicePort);
        h.insert(QStringLiteral("sourcePort"), host.probePort);
        h.insert(QStringLiteral("replyPort"), host.replyPort);
        h.insert(QStringLiteral("label"), hostLabel(host));
        h.insert(QStringLiteral("devices"), devices);
        out.append(h);
    }
    return out;
}

QVariantList LinkDiscovery::ports() const
{
    QVariantList out;
    for (int p : ports_)
        out.append(p);
    return out;
}

void LinkDiscovery::setPorts(const QVariantList& ports)
{
    QList<int> next;
    for (const QVariant& v : ports) {
        const int p = v.toInt();
        if (p > 0 && p <= 65535 && !next.contains(p))
            next.append(p);
    }
    if (next.isEmpty() || next == ports_)
        return;
    ports_ = next;
    emit portsChanged();
}

int LinkDiscovery::durationMs() const
{
    return durationMs_;
}

void LinkDiscovery::setDurationMs(int ms)
{
    const int clamped = qBound(300, ms, 15000);
    if (clamped == durationMs_)
        return;
    durationMs_ = clamped;
    emit durationMsChanged();
}

void LinkDiscovery::start()
{
    if (running_)
        return;

    hosts_.clear();
    emit foundChanged();

    for (int port : ports_) {
        auto* probe = new Probe;
        probe->port = static_cast<quint16>(port);
        probe->socket = std::make_unique<QUdpSocket>(this);
        bool bound = probe->socket->bind(QHostAddress::AnyIPv4, probe->port,
                                         QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
        if (!bound)
            bound = probe->socket->bind(QHostAddress::AnyIPv4, 0);
        if (!bound) {
            delete probe;
            continue;
        }
        QObject::connect(probe->socket.get(), &QUdpSocket::readyRead, this, [this, probe]() { readProbe(probe); });
        probes_.append(probe);
    }

    if (probes_.isEmpty()) {
        emit finished(0);
        return;
    }

    running_ = true;
    emit runningChanged();

    sendsLeft_ = kSendCount;
    sendProbes();
    sendTimer_.start();
    finishTimer_.start(durationMs_);
}

void LinkDiscovery::stop()
{
    if (!running_)
        return;
    finish();
}

int LinkDiscovery::adoptAll(bool open)
{
    if (!linkManager_)
        return 0;

    int count = 0;
    for (const Host& host : hosts_) {
        linkManager_->adoptDiscoveredUdp(host.address.toString(), host.probePort, host.devicePort, hostLabel(host), open);
        ++count;
    }
    return count;
}

void LinkDiscovery::sendProbes()
{
    if (sendsLeft_ <= 0) {
        sendTimer_.stop();
        return;
    }
    --sendsLeft_;

    const QByteArray request = buildRequest();
    const QList<QHostAddress> targets = broadcastTargets();
    for (Probe* probe : probes_) {
        for (const QHostAddress& target : targets)
            probe->socket->writeDatagram(request, target, probe->port);
    }
}

void LinkDiscovery::readProbe(Probe* probe)
{
    QUdpSocket* socket = probe->socket.get();
    while (socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = socket->receiveDatagram();
        if (!datagram.isValid() || datagram.data().isEmpty())
            continue;

        QByteArray data = datagram.data();
        FrameParser parser;
        parser.setContext(reinterpret_cast<uint8_t*>(data.data()), static_cast<uint32_t>(data.size()));
        while (parser.availContext() > 0) {
            parser.process();
            if (!parser.isComplete())
                continue;
            if (!(parser.completeAsKBP() || parser.completeAsKBP2()))
                continue;
            if (parser.id() != ID_VERSION || parser.type() != CONTENT || parser.resp())
                continue;
            if (parser.ver() != v0 && parser.ver() != v2)
                continue;

            QHostAddress sender = datagram.senderAddress();
            bool isV4 = false;
            const quint32 v4 = sender.toIPv4Address(&isV4);
            if (isV4)
                sender = QHostAddress(v4);

            const uint8_t route = parser.route();
            const Version ver = parser.ver();

            int hostIdx = -1;
            for (int i = 0; i < hosts_.size(); ++i) {
                if (hosts_[i].address == sender && hosts_[i].devicePort == probe->port) {
                    hostIdx = i;
                    break;
                }
            }
            if (hostIdx < 0) {
                Host host;
                host.address = sender;
                host.devicePort = probe->port;
                host.probePort = probe->port;
                host.replyPort = static_cast<quint16>(datagram.senderPort());
                hosts_.append(host);
                hostIdx = hosts_.size() - 1;
            }
            Host& host = hosts_[hostIdx];

            int devIdx = -1;
            for (int i = 0; i < host.devices.size(); ++i) {
                if (host.devices[i].route == route) {
                    devIdx = i;
                    break;
                }
            }
            if (devIdx < 0) {
                DeviceInfo dev;
                dev.route = route;
                host.devices.append(dev);
                devIdx = host.devices.size() - 1;
            }
            DeviceInfo& dev = host.devices[devIdx];

            if (ver == v0) {
                dev.boardMinor = parser.read<U1>();
                dev.board = parser.read<U1>();
                parser.readSkip(12);
                dev.serialNumber = parser.read<U4>();
                dev.hasVersion0 = true;
            } else {
                parser.read<U1>();
                dev.boardMinor = parser.read<U1>();
                dev.board = parser.read<U1>();
                parser.read<U1>();
                parser.read<U1>();
                parser.read<U2>();
                dev.fwMinor = parser.read<U1>();
                dev.fwMajor = parser.read<U1>();
                dev.hasVersion2 = true;
            }

            emit foundChanged();
        }
    }
}

void LinkDiscovery::closeProbes()
{
    for (Probe* probe : probes_) {
        probe->socket->close();
        delete probe;
    }
    probes_.clear();
}

void LinkDiscovery::finish()
{
    sendTimer_.stop();
    finishTimer_.stop();
    closeProbes();

    int deviceCount = 0;
    for (const Host& host : hosts_)
        deviceCount += host.devices.size();

    running_ = false;
    emit runningChanged();
    emit finished(deviceCount);
}

QByteArray LinkDiscovery::buildRequest()
{
    QByteArray request;
    for (Version ver : {v0, v2}) {
        ProtoBinOut out;
        out.create(GETTING, ver, ID_VERSION, 0);
        out.end();
        request.append(reinterpret_cast<const char*>(out.frame()), out.frameLen());
    }
    return request;
}

QList<QHostAddress> LinkDiscovery::broadcastTargets()
{
    QList<QHostAddress> targets;
    targets.append(QHostAddress::Broadcast);

    const auto required = QNetworkInterface::IsUp | QNetworkInterface::IsRunning | QNetworkInterface::CanBroadcast;
    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if ((flags & required) != required || (flags & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
                continue;
            const QHostAddress bcast = entry.broadcast();
            if (bcast.isNull() || targets.contains(bcast))
                continue;
            targets.append(bcast);
        }
    }
    return targets;
}

QString LinkDiscovery::hostLabel(const Host& host)
{
    if (host.devices.isEmpty())
        return QString();

    const DeviceInfo* primary = &host.devices.first();
    for (const DeviceInfo& dev : host.devices) {
        if (dev.route == 0) {
            primary = &dev;
            break;
        }
    }

    QString label = boardVersionName(static_cast<BoardVersion>(primary->board), primary->boardMinor);
    if (primary->hasVersion0 && primary->serialNumber != 0 && primary->serialNumber != kUnprogrammedSerial)
        label += QStringLiteral(" #%1").arg(primary->serialNumber);
    if (host.devices.size() > 1)
        label += QStringLiteral(" (+%1)").arg(host.devices.size() - 1);
    return label;
}
