#include "DeviceManager.h"

#include <QDateTime>
#include "core.h"
extern Core core;


DeviceManager::DeviceManager() :
    lastDevs_(nullptr),
    lastDevice_(nullptr),
    mavlinkLink_(nullptr),
    lastAddress_(-1),
    progress_(0),
    isConsoled_(false),
    break_(false)
{
    qRegisterMetaType<ProtoBinOut>("ProtoBinOut");
    qRegisterMetaType<int16_t>("int16_t");
    qRegisterMetaType<QVector<uint8_t>>("QVector<uint8_t>");
    qRegisterMetaType<QByteArray>("QByteArray");
    qRegisterMetaType<IDBinUsblSolution::UsblSolution>("IDBinUsblSolution::UsblSolution");
    qRegisterMetaType<IDBinDVL::BeamSolution>("IDBinDVL::BeamSolution");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<IDBinDVL::DVLSolution>("IDBinDVL::DVLSolution");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<FrameParser>("FrameParser");
}

DeviceManager::~DeviceManager()
{

}

float DeviceManager::vruVoltage()
{
    return vru_.voltage;
}

float DeviceManager::vruCurrent()
{
    return vru_.current;
}

float DeviceManager::vruVelocityH()
{
    return vru_.velocityH;
}

int DeviceManager::pilotArmState()
{
    return vru_.armState;
}

int DeviceManager::pilotModeState()
{
    return vru_.flightMode;
}

QList<DevQProperty *> DeviceManager::getDevList()
{
    devList_.clear();

    for (auto i = devTree_.cbegin(), end = devTree_.cend(); i != end; ++i) {
        QHash<int, DevQProperty*> devs = i.value();

        for (auto k = devs.cbegin(), end = devs.cend(); k != end; ++k) {
            devList_.append(k.value());
        }
    }

    return devList_;
}

DevQProperty *DeviceManager::getLastDev()
{
    return lastDevs_;
}

void DeviceManager::frameInput(QUuid uuid, Link* link, FrameParser frame)
{
    if (frame.isComplete()) {

#if !defined(Q_OS_ANDROID)
        if (frame.isStream())
            streamList_.append(&frame);
        if (frame.id() == ID_STREAM)
            streamList_.parse(&frame);
        if (streamList_.isListChenged())
            emit streamChanged();
#endif

        if (link != NULL) {
            if (frame.isProxy() || frame.completeAsKBP()) {
                otherProtocolStat_.remove(uuid);
            }
        }

        if (frame.isProxy()) {
            return; //continue;
        }

        if (frame.completeAsKBP() || frame.completeAsKBP2()) {
            DevQProperty* dev = getDevice(uuid, link, frame.route());

            if (isConsoled_ && (link != NULL) && !(frame.id() == 32 || frame.id() == 33))
                core.consoleProto(frame);

#if !defined(Q_OS_ANDROID)
            if (frame.id() == ID_TIMESTAMP && frame.ver() == v1) {
                int t = static_cast<int>(frame.read<U4>());
                int u = static_cast<int>(frame.read<U4>());
                emit eventComplete(t, 0, u);
            }

            if (frame.id() == ID_EVENT) {
                int timestamp = frame.read<U4>();
                int id = frame.read<U4>();
                if (id < 100) {
                    emit eventComplete(timestamp, id, 0);
                }

            }

            if (frame.id() == ID_VOLTAGE) {
                int v_id = frame.read<U1>();
                int32_t v_uv = frame.read<S4>();
                Q_UNUSED(v_uv);
                if (v_id == 1) {
                    // core.dataset()->addEncoder(float(v_uv));
                    // qInfo("Voltage %f", float(v_uv));
                }
            }
#endif
            dev->protoComplete(frame);
        }

        if (frame.isCompleteAsNMEA()) {
            ProtoNMEA& prot_nmea = (ProtoNMEA&)frame;
            QString str_data = QByteArray((char*)prot_nmea.frame(), prot_nmea.frameLen() - 2);
            core.consoleInfo(QString(">> NMEA: %5").arg(str_data));

            if (prot_nmea.isEqualId("DBT")) {
                prot_nmea.skip();
                prot_nmea.skip();
                double depth_m = prot_nmea.readDouble();
                if (isfinite(depth_m)) {
                    emit rangefinderComplete(depth_m);
                }

            }

            if (prot_nmea.isEqualId("RMC")) {
                uint8_t h = 0, m = 0, s = 0;
                uint16_t ms = 0;

                bool isCorrect =  prot_nmea.readTime(&h, &m, &s, &ms);
                Q_UNUSED(isCorrect);

                char c = prot_nmea.readChar();
                if (c == 'A') {
                    double lat = prot_nmea.readLatitude();
                    double lon = prot_nmea.readLongitude();

                    prot_nmea.skip();
                    prot_nmea.skip();

                    uint16_t year = 0;
                    uint8_t mounth = 0, day = 0;
                    prot_nmea.readDate(&year, &mounth, & day);

                    uint32_t unix_time = QDateTime(QDate(year, mounth, day), QTime(h, m, s), Qt::TimeSpec::UTC).toSecsSinceEpoch();
                    emit positionComplete(lat, lon, unix_time, (uint32_t)ms*1000*1000);
                }
            }
        }

        if (frame.isCompleteAsUBX()) {
            ProtoUBX& ubx_frame = (ProtoUBX&)frame;

            if (ubx_frame.msgClass() == 1 && ubx_frame.msgId() == 7) {

                uint8_t h = 0, m = 0, s = 0;
                uint16_t year = 0;
                uint8_t month = 0, day = 0;
                int32_t nanosec = 0;

                ubx_frame.readSkip(4);
                year = ubx_frame.read<U2>();
                month = ubx_frame.read<U1>();
                day = ubx_frame.read<U1>();
                h = ubx_frame.read<U1>();
                m = ubx_frame.read<U1>();
                s = ubx_frame.read<U1>();
                ubx_frame.read<U1>(); // Validity flags
                ubx_frame.readSkip(4); // Time accuracy estimate (UTC)
                nanosec = ubx_frame.read<S4>();

                uint8_t fix_type = ubx_frame.read<U1>();
                uint8_t fix_flags = ubx_frame.read<U1>();
                Q_UNUSED(fix_flags);

                ubx_frame.read<U1>();
                uint8_t satellites_in_used = ubx_frame.read<U1>();

                int32_t lon_int = ubx_frame.read<S4>();
                int32_t lat_int = ubx_frame.read<S4>();

                uint32_t unix_time = QDateTime(QDate(year, month, day), QTime(h, m, s), Qt::TimeSpec::UTC).toSecsSinceEpoch();

                if (fix_type > 1 && fix_type < 5) {
                    emit positionComplete(double(lat_int)*0.0000001, double(lon_int)*0.0000001, unix_time, nanosec);
                }

                if (isConsoled_) {
                    core.consoleInfo(QString(">> UBX: NAV_PVT, fix %1, sats %2, lat %3, lon %4, time %5:%6:%7.%8")
                                         .arg(fix_type).arg(satellites_in_used).arg(double(lat_int)*0.0000001).arg(double(lon_int)*0.0000001).arg(h).arg(m).arg(s).arg(nanosec/1000));
                }
            }
            else {
                if (isConsoled_)
                    core.consoleInfo(QString(">> UBX: class/id 0x%1 0x%2, len %3").arg(ubx_frame.msgClass(), 2, 16, QLatin1Char('0')).arg(ubx_frame.msgId(), 2, 16, QLatin1Char('0')).arg(ubx_frame.frameLen()));
            }
        }

        if (frame.isCompleteAsMAVLink()) {
            if (link == nullptr || proxyLinkUuid_ != uuid) {
                emit writeProxyFrame(frame);

                if (link != nullptr && mavlinUuid_ != uuid) {
                    mavlinUuid_ = uuid;
                    if(mavlinkLink_ != nullptr) {
                        disconnect(this, &DeviceManager::writeMavlinkFrame,  mavlinkLink_, &Link::writeFrame);
                    }
                    mavlinkLink_ = link;
                    connect(this, &DeviceManager::writeMavlinkFrame, mavlinkLink_, &Link::writeFrame, Qt::UniqueConnection);
                }

                ProtoMAVLink& mavlink_frame = (ProtoMAVLink&)frame;

                if (mavlink_frame.msgId() == 24) { // GLOBAL_POSITION_INT
                    MAVLink_MSG_GPS_RAW_INT pos = mavlink_frame.read<MAVLink_MSG_GPS_RAW_INT>();
                    if (pos.isValid()) {
                        emit positionComplete(pos.latitude(), pos.longitude(), pos.time_boot_msec()/1000, (pos.time_boot_msec()%1000)*1e6);
                        emit gnssVelocityComplete(pos.velocityH(), 0);
                        vru_.velocityH = pos.velocityH();
                        emit vruChanged();
                    }
                }

                if (mavlink_frame.msgId() == 0) { // SYS_STATUS
                    MAVLink_MSG_HEARTBEAT heartbeat = mavlink_frame.read<MAVLink_MSG_HEARTBEAT>();
                    vru_.armState = (int)heartbeat.isArmed();
                    int flight_mode = (int)heartbeat.customMode();
                    if (flight_mode != vru_.flightMode) {
                        core.consoleInfo(QString(">> FC: Flight mode %1").arg(flight_mode));
                    }
                    vru_.flightMode = flight_mode;
                    emit vruChanged();
                }

                if (mavlink_frame.msgId() == 147) { // BATTERY_STATUS
                    MAVLink_MSG_BATTERY_STATUS battery_status = mavlink_frame.read<MAVLink_MSG_BATTERY_STATUS>();
                    vru_.voltage = battery_status.voltage();
                    vru_.current = battery_status.current();
                    emit vruChanged();
                }

                if (mavlink_frame.msgId() == 30) {
                    MAVLink_MSG_ATTITUDE attitude = mavlink_frame.read<MAVLink_MSG_ATTITUDE>();
                    emit attitudeComplete(attitude.yawDeg(),attitude.pitchDeg(), attitude.rollDeg());
                }

                core.consoleInfo(QString(">> MAVLink v%1: ID %2, comp. id %3, seq numb %4, len %5").arg(mavlink_frame.MAVLinkVersion()).arg(mavlink_frame.msgId()).arg(mavlink_frame.componentID()).arg(mavlink_frame.sequenceNumber()).arg(mavlink_frame.frameLen()));
            }
            else {
                if (link != nullptr) {
                    emit writeMavlinkFrame(frame);
                }
            }
        }

        if (link != NULL) {
            if ((frame.isCompleteAsNMEA() && !((ProtoNMEA*)&frame)->isEqualId("DBT")) ||
                frame.isCompleteAsUBX() ||
                frame.isCompleteAsMAVLink()) {
                if (!frame.isNested()) {
                    otherProtocolStat_[uuid]++;
                    if (otherProtocolStat_[uuid] > 30) {
                        deleteDevicesByLink(uuid);
                    }
                }
            }
        }
    }
}

void DeviceManager::openFile(const QString &filePath)
{
    QFile file;
    const QUrl url(filePath);
    url.isLocalFile() ? file.setFileName(url.toLocalFile()) : file.setFileName(url.toString());

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const qint64 totalSize = file.size();
    qint64 bytesRead = 0;
    Parsers::FrameParser frameParser;
    const QUuid someUuid;

    while (true) {
        if (break_) {
            file.close();
            return;
        }

        QByteArray chunk = file.read(1024 * 1024);
        const qint64 chunkSize = chunk.size();

        if (chunkSize == 0)
            break;

        bytesRead += chunkSize;

        auto currProgress = static_cast<int>((static_cast<float>(bytesRead) / static_cast<float>(totalSize)) * 100.0f);
        currProgress = std::max(0, currProgress);
        currProgress = std::min(100, currProgress);

        if (progress_ != currProgress) {
            progress_ = currProgress;
        }

        frameParser.setContext((uint8_t*)chunk.data(), chunk.size());

        while (frameParser.availContext() > 0) {
            frameParser.process();
            if (frameParser.isComplete()) {
                frameInput(someUuid, NULL, frameParser);
            }
        }

        chunk.clear();
    }
    file.close();

    vru_.cleanVru();
    delAllDev();
    emit vruChanged();

    file.close();
}

void DeviceManager::closeFile()
{
    delAllDev();
    vru_.cleanVru();
    emit vruChanged();
}

void DeviceManager::onLinkOpened(QUuid uuid, Link *link)
{
    Q_UNUSED(uuid);

    if (link) {
        if (link->getIsProxy()) {
            proxyLinkUuid_ = uuid;
            connect(this, &DeviceManager::writeProxyFrame, link, &Link::writeFrame);
        }
        else {
            getDevice(uuid, link, 0);
        }
    }
}

void DeviceManager::onLinkClosed(QUuid uuid, Link *link)
{
    Q_UNUSED(uuid);

    if (link) {
        deleteDevicesByLink(uuid);
        this->disconnect(link);
        otherProtocolStat_.remove(uuid);
        if(uuid == mavlinUuid_) {
            mavlinUuid_ = QUuid();
        }
    }
}

void DeviceManager::onLinkDeleted(QUuid uuid, Link *link)
{
    Q_UNUSED(uuid);

    if (link) {
        deleteDevicesByLink(uuid);
        this->disconnect(link);
        otherProtocolStat_.remove(uuid);
        if(uuid == mavlinUuid_) {
            mavlinUuid_ = QUuid();
        }
    }
}

void DeviceManager::binFrameOut(ProtoBinOut protoOut)
{
    if (isConsoled_ && !(protoOut.id() == 33 || protoOut.id() == 33)) {
        core.consoleProto(protoOut, false);
    }
}

bool DeviceManager::isCreatedId(int id)
{
    return getDevList().size() > id;
}

void DeviceManager::setProtoBinConsoled(bool isConsoled)
{
    isConsoled_ = isConsoled;
}

void DeviceManager::upgradeLastDev(QByteArray data)
{
    if (lastDevs_ != NULL) {
        lastDevs_->sendUpdateFW(data);
    }
}

StreamListModel* DeviceManager::streamsList()
{
    return streamList_.streamsList();
}

void DeviceManager::readyReadProxy(Link* link)
{
    while (link->parse()) {
        FrameParser* frame = link->frameParser();

        if (frame->isComplete()) {
            QByteArray data((char*)frame->frame(), frame->frameLen());
            emit dataSend(data);
        }
    }
}

void DeviceManager::readyReadProxyNav(Link* link)
{
    while (link->parse()) {
        FrameParser* frame = link->frameParser();

        if (frame->isComplete()) {
            QByteArray data((char*)frame->frame(), frame->frameLen());
            emit dataSend(data);
        }
    }
}

DevQProperty* DeviceManager::getDevice(QUuid uuid, Link *link, uint8_t addr)
{
    if ((link == NULL || lastUuid_ == uuid) && lastAddress_ == addr && lastDevice_ != NULL) {
        return lastDevice_;
    }
    else {
        lastDevice_ = devTree_[uuid][addr];
        if (lastDevice_ == NULL) {
            lastDevice_ = createDev(uuid, link, addr);
        }
        lastUuid_ = uuid;
        lastAddress_ = addr;
    }

    return lastDevice_;
}

void DeviceManager::delAllDev()
{
    QList<QUuid> keysToDelete;
    for (auto i = devTree_.cbegin(), end = devTree_.cend(); i != end; ++i) {
        keysToDelete.append(i.key());
    }

    for (const auto& key : keysToDelete) {
        deleteDevicesByLink(key);
    }
}

void DeviceManager::deleteDevicesByLink(QUuid uuid)
{
    if (devTree_.contains(uuid)) {
        QHash<int, DevQProperty*> devs = devTree_[uuid];
        for (auto i = devs.cbegin(), end = devs.cend(); i != end; ++i) {
            if (lastDevice_ == i.value()) {
                lastDevice_ = NULL;
            }
            disconnect(i.value());
            delete i.value();
        }
        devTree_[uuid].clear();
        devTree_.remove(uuid);
        emit devChanged();
    }
}

DevQProperty* DeviceManager::createDev(QUuid uuid, Link* link, uint8_t addr)
{
    DevQProperty* dev = new DevQProperty();
    devTree_[uuid][addr] = dev;
    dev->setBusAddress(addr);

    if(link != NULL) {
        connect(dev, &DevQProperty::binFrameOut, this, &DeviceManager::binFrameOut);
        connect(dev, &DevQProperty::binFrameOut, link, &Link::writeFrame);
    }

    connect(dev, &DevQProperty::chartComplete, this, &DeviceManager::chartComplete);
    connect(dev, &DevQProperty::rawDataRecieved, this, &DeviceManager::rawDataRecieved);
    connect(dev, &DevQProperty::attitudeComplete, this, &DeviceManager::attitudeComplete);
    connect(dev, &DevQProperty::distComplete, this, &DeviceManager::distComplete);
    connect(dev, &DevQProperty::usblSolutionComplete, this, &DeviceManager::usblSolutionComplete);
    connect(dev, &DevQProperty::dopplerBeamComplete, this, &DeviceManager::dopplerBeamComlete);
    connect(dev, &DevQProperty::dvlSolutionComplete, this, &DeviceManager::dvlSolutionComplete);
    connect(dev, &DevQProperty::upgradeProgressChanged, this, &DeviceManager::upgradeProgressChanged);

    dev->startConnection(link != NULL);

    emit devChanged();

    return dev;
}
