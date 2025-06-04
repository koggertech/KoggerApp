#include "device_manager.h"
#include "device_defs.h"
#include <QDateTime>
#include "core.h"
extern Core core;


DeviceManager::DeviceManager()
    : lastDevs_(nullptr),
      lastDevice_(nullptr),
      mavlinkLink_(nullptr),
      lastAddress_(-1),
      progress_(0),
      isConsoled_(false),
      break_(false),
      upgradeUuid_(QUuid()),
      upgradeAddr_(0)
{
    qRegisterMetaType<ProtoBinOut>("ProtoBinOut");
#ifdef SEPARATE_READING
    qRegisterMetaType<ProtoBinOut>("ProtoBinOut&");
#endif
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

int DeviceManager::calcAverageChartLosses()
{
    int retVal = 0;
    int averageChartLosses = 0;
    int numOfDevices = 0;

    for (auto i = devTree_.cbegin(), end = devTree_.cend(); i != end; ++i) {
        QHash<int, DevQProperty*> devs = i.value();

        for (auto k = devs.cbegin(), end = devs.cend(); k != end; ++k) {
            ++numOfDevices;
            averageChartLosses += k.value()->getAverageChartLosses();
        }
    }

    if (numOfDevices != 0) {
        retVal = averageChartLosses / numOfDevices;
    }

    return retVal;
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

QList<DevQProperty *> DeviceManager::getDevList(BoardVersion ver) {
    QList<DevQProperty *> list;

    for (auto i = devTree_.cbegin(), end = devTree_.cend(); i != end; ++i) {
        QHash<int, DevQProperty*> devs = i.value();

        for (auto k = devs.cbegin(), end = devs.cend(); k != end; ++k) {
            if(k.value()->boardVersion() == ver) {
                list.append(k.value());
            }
        }
    }

    return list;
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
                    if (auto* dev = getDevice(uuid, link, frame.route()); dev) { // work?
                        emit rangefinderComplete(dev->getChannelId(), depth_m);
                    }
                }
            }

            if (prot_nmea.isEqualId("RMC")) {
                uint8_t h = 0, m = 0, s = 0;
                uint16_t ms = 0;

                bool isCorrect =  prot_nmea.readTime(&h, &m, &s, &ms);
                Q_UNUSED(isCorrect);

                char c = prot_nmea.readChar();
                if (c == 'A' || c == 'D') {
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

            if (prot_nmea.isEqualId("GGA")) {
                uint8_t h = 0, m = 0, s = 0;
                uint16_t ms = 0;

                bool isCorrect =  prot_nmea.readTime(&h, &m, &s, &ms);
                Q_UNUSED(isCorrect);

                double lat = prot_nmea.readLatitude();
                double lon = prot_nmea.readLongitude();

                char q = prot_nmea.readChar();

                prot_nmea.skip(); // sv
                prot_nmea.skip(); // HDOP

                float height_msl = prot_nmea.readDouble(); // Orthometric height (MSL reference)

                if (q == '1' || q == '2' || q == '4' || q == '5') {
                    uint16_t year = 1971;
                    uint8_t mounth = 1, day = 1;

                    Position pos;
                    pos.lla.latitude = lat;
                    pos.lla.longitude = lon;
                    pos.lla.altitude = height_msl;
                    pos.lla.source = PositionSourceRTK;
                    pos.lla.altSource = AltitudeSourceRTK;
                    pos.time = DateTime(year, mounth, day, h, m, s, int64_t(ms)*1000*1000);

                    if(q == '4') {
                        emit positionCompleteRTK(pos);
                    }
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

                // if (isConsoled_) {
                    core.consoleInfo(QString(">> UBX: NAV_PVT, fix %1, sats %2, lat %3, lon %4, time %5:%6:%7.%8")
                                         .arg(fix_type).arg(satellites_in_used).arg(double(lat_int)*0.0000001).arg(double(lon_int)*0.0000001).arg(h).arg(m).arg(s).arg(nanosec/1000));
                // }
            }
            else {
                // if (isConsoled_)
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

                // if (mavlink_frame.msgId() == 24) { // GLOBAL_POSITION_INT
                //     MAVLink_MSG_GPS_RAW_INT pos = mavlink_frame.read<MAVLink_MSG_GPS_RAW_INT>();
                //     if (pos.isValid()) {
                //         emit positionComplete(pos.latitude(), pos.longitude(), pos.time_boot_msec()/1000, (pos.time_boot_msec()%1000)*1e6);
                //         emit gnssVelocityComplete(pos.velocityH(), 0);
                //         vru_.velocityH = pos.velocityH();
                //         emit vruChanged();
                //     }
                // }

                if(mavlink_frame.msgId() == MAVLink_MSG_GLOBAL_POSITION_INT::getID()) {
                    MAVLink_MSG_GLOBAL_POSITION_INT pos = mavlink_frame.read<MAVLink_MSG_GLOBAL_POSITION_INT>();
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

                    const float yaw = attitude.yawDeg();
                    const float pitch = attitude.pitchDeg();
                    const float roll = attitude.rollDeg();

                    if (!qFuzzyIsNull(yaw) || !qFuzzyIsNull(pitch) || !qFuzzyIsNull(roll)) {
                        emit attitudeComplete(yaw, attitude.pitchDeg(), attitude.rollDeg());
                    }
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

void DeviceManager::openFile(QString filePath)
{
#ifdef SEPARATE_READING
    break_ = false;
#endif

    QFile file;
    const QUrl url(filePath);
    url.isLocalFile() ? file.setFileName(url.toLocalFile()) : file.setFileName(url.toString());

    if (!file.open(QIODevice::ReadOnly)) {
        emit fileStopsOpening();
        return;
    }

    const qint64 totalSize = file.size();
    qint64 bytesRead = 0;
    Parsers::FrameParser frameParser;
    const QUuid someUuid(kFileUuidStr);

    delAllDev();

#ifdef SEPARATE_READING
    emit fileStartOpening();
    bool fileReadEnough{false};
#endif

    while (true) {

#ifdef SEPARATE_READING
        QCoreApplication::processEvents();
        if (break_) {
            emit fileBreaked(onOpen_);
            onOpen_ = false;
            file.close();
            emit fileStopsOpening();
            return;
        }
#else
        if (break_) {
            file.close();
            return;
        }
#endif

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

#ifdef SEPARATE_READING
        int sleepCnt = 0;
#endif

        while (frameParser.availContext() > 0) {

#ifdef SEPARATE_READING
            QCoreApplication::processEvents();
            if (break_) {
                emit fileBreaked(onOpen_);
                onOpen_ = false;
                file.close();
                return;
            }
            if (sleepCnt > 500) {
                QThread::msleep(50);
                sleepCnt = 0;
            }
            ++sleepCnt;
#endif
            frameParser.process();
            if (frameParser.isComplete()) {
                frameInput(someUuid, NULL, frameParser);
            }
        }

#ifdef SEPARATE_READING
        if (!fileReadEnough) { // it's really that?
            emit onFileReadEnough();
            fileReadEnough = true;
        }
#endif

        chunk.clear();
    }
    file.close();

    vru_.cleanVru();
    delAllDev();
    emit vruChanged();

    emit fileOpened();
    emit fileStopsOpening();
}

#ifdef SEPARATE_READING
void DeviceManager::closeFile(bool onOpen)
{
    onOpen_ = onOpen;
    break_ = true;

    vru_.cleanVru();
    delAllDev();
    emit vruChanged();
}
#else
void DeviceManager::closeFile()
{
    delAllDev();
    vru_.cleanVru();
    emit vruChanged();
}
#endif

void DeviceManager::onLinkOpened(QUuid uuid, Link *link)
{
    if (link) {
        if (link->getIsProxy()) {
            proxyLinkUuid_ = uuid;
            connect(this, &DeviceManager::writeProxyFrame, link, &Link::writeFrame);
        } else if(link->attribute() == LinkAttribute::kLinkAttributeBoot) {
            core.consoleInfo("Device: Boot opened");
        } else {
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
    if (isConsoled_ && protoOut.id() != 33) {
        core.consoleProto(protoOut, false);
    }
    emit sendProtoFrame(protoOut);
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

void DeviceManager::beaconActivationReceive(uint8_t id) {
    Q_UNUSED(id)

    QList<DevQProperty *> usbl_devs = getDevList(BoardUSBL);
    if(usbl_devs.size() > 0) {
        IDBinUsblSolution::USBLRequestBeacon ask = {};
        usbl_devs[0]->askBeaconPosition(ask);
    }
}

void DeviceManager::beaconDirectQueueAsk() {
    QList<DevQProperty *> usbl_devs = getDevList(BoardUSBLBeacon);
    qDebug("Sent request to the Beacon # %d", -1);
    if(usbl_devs.size() > 0) {
        usbl_devs[0]->enableBeaconOnce(3);
        qDebug("Sent request to the Beacon # %d", 0);
    }
}

void DeviceManager::setUSBLBeaconDirectAsk(bool is_ask) {
    isUSBLBeaconDirectAsk = is_ask;
    qDebug("Beacon auto scan is: %d", is_ask);
    if(is_ask == true) {
        QObject::connect(&beacon_timer, &QTimer::timeout, this, &DeviceManager::beaconDirectQueueAsk);
        beacon_timer.setInterval(3000);
        beacon_timer.start();
    } else {
        beacon_timer.stop();
    }
}

void DeviceManager::onLoggingKlfStarted()
{
    for (auto i = devTree_.cbegin(), end = devTree_.cend(); i != end; ++i) {
        QHash<int, DevQProperty*> devs = i.value();
        for (auto k = devs.cbegin(), end = devs.cend(); k != end; ++k) {
            k.value()->requestSetup();
        }
    }
}

void DeviceManager::onSendRequestAll(QUuid uuid)
{
    if (devTree_.contains(uuid)) {
        QHash<int, DevQProperty*> devs = devTree_[uuid];
        for (auto i = devs.cbegin(), end = devs.cend(); i != end; ++i) {
            if (auto* dev = i.value(); dev) {
                dev->doRequestAll();
            }
        }
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

void DeviceManager::onStartUpgradingFirmware(QUuid linkUuid, uint8_t address, const QByteArray& firmware)
{
    qDebug() << "DeviceManager::onStartUpgradingFirmware";

    upgradeUuid_ = linkUuid;
    upgradeAddr_ = address;
    upgradeData_ = firmware;
}

void DeviceManager::onUpgradingFirmwareDone()
{
    qDebug() << "DeviceManager::onUpgradingFirmwareDone";

    upgradeUuid_ = QUuid();
    upgradeAddr_ = 0;
    upgradeData_.clear();
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

#ifdef SEPARATE_READING
            QMetaObject::invokeMethod(i.value(), "deleteLater", Qt::QueuedConnection);
#else
            i.value()->deleteLater();
#endif
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
    dev->setLinkUuid(uuid);

    if (upgradeUuid_ == uuid && upgradeAddr_ == addr) {
        dev->setFirmware(upgradeData_);
        upgradeUuid_ = QUuid();
    }

#ifdef SEPARATE_READING
    auto connType = Qt::AutoConnection;

    if(link != NULL) {
        connect(dev, &DevQProperty::binFrameOut, this, &DeviceManager::binFrameOut, connType);
        connect(dev, &DevQProperty::binFrameOut, link, &Link::writeFrame, connType);
        connect(dev, &DevQProperty::startUpgradingFirmware, link, &Link::onStartUpgradingFirmware, connType);
        connect(dev, &DevQProperty::upgradingFirmwareDone, link, &Link::onUpgradingFirmwareDone, connType);
    }

    connect(dev, &DevQProperty::startUpgradingFirmwareDM, this, &DeviceManager::onStartUpgradingFirmware, connType);
    connect(dev, &DevQProperty::upgradingFirmwareDoneDM, this, &DeviceManager::onUpgradingFirmwareDone, connType);

    //
    connect(dev, &DevQProperty::sendChartSetup, this, &DeviceManager::sendChartSetup, connType);
    connect(dev, &DevQProperty::sendTranscSetup, this, &DeviceManager::sendTranscSetup, connType);
    connect(dev, &DevQProperty::sendSoundSpeed, this, &DeviceManager::sendSoundSpeeed, connType);
    connect(dev, &DevQProperty::averageChartLossesChanged, this, &DeviceManager::chartLossesChanged, connType);

    connect(dev, &DevQProperty::chartComplete, this, &DeviceManager::chartComplete, connType);
    connect(dev, &DevQProperty::rawDataRecieved, this, &DeviceManager::rawDataRecieved, connType);
    connect(dev, &DevQProperty::attitudeComplete, this, &DeviceManager::attitudeComplete, connType);
    connect(dev, &DevQProperty::distComplete, this, &DeviceManager::distComplete, connType);
    connect(dev, &DevQProperty::usblSolutionComplete, this, &DeviceManager::usblSolutionComplete, connType);
    connect(dev, &DevQProperty::dopplerBeamComplete, this, &DeviceManager::dopplerBeamComlete, connType);
    connect(dev, &DevQProperty::dvlSolutionComplete, this, &DeviceManager::dvlSolutionComplete, connType);
    connect(dev, &DevQProperty::upgradeProgressChanged, this, &DeviceManager::upgradeProgressChanged, connType);

    dev->moveToThread(qApp->thread());
    dev->getProcessTimer()->moveToThread(qApp->thread());
    QList<QTimer*> timers = dev->getChildTimers();
    foreach (QTimer* timer, timers) {
        timer->moveToThread(qApp->thread());
    }

    QMetaObject::invokeMethod(dev, "initProcessTimerConnects", Qt::QueuedConnection);
    QMetaObject::invokeMethod(dev, "initChildsTimersConnects", Qt::QueuedConnection);
    QMetaObject::invokeMethod(dev, "startConnection", Qt::QueuedConnection, Q_ARG(bool, link != NULL));
#else
    if(link != NULL) {
        connect(dev, &DevQProperty::binFrameOut, this, &DeviceManager::binFrameOut);
        connect(dev, &DevQProperty::binFrameOut, link, &Link::writeFrame);
        connect(dev, &DevQProperty::startUpgradingFirmware, link, &Link::onStartUpgradingFirmware);
        connect(dev, &DevQProperty::upgradingFirmwareDone, link, &Link::onUpgradingFirmwareDone);
    }

    connect(dev, &DevQProperty::startUpgradingFirmwareDM, this, &DeviceManager::onStartUpgradingFirmware);
    connect(dev, &DevQProperty::upgradingFirmwareDoneDM, this, &DeviceManager::onUpgradingFirmwareDone);

    //
    connect(dev, &DevQProperty::sendChartSetup,  this, &DeviceManager::sendChartSetup);
    connect(dev, &DevQProperty::sendTranscSetup, this, &DeviceManager::sendTranscSetup);
    connect(dev, &DevQProperty::sendSoundSpeed, this, &DeviceManager::sendSoundSpeeed);
    connect(dev, &DevQProperty::averageChartLossesChanged, this, &DeviceManager::chartLossesChanged);

    connect(dev, &DevQProperty::chartComplete, this, &DeviceManager::chartComplete);
    connect(dev, &DevQProperty::rawDataRecieved, this, &DeviceManager::rawDataRecieved);
    connect(dev, &DevQProperty::attitudeComplete, this, &DeviceManager::attitudeComplete);
    connect(dev, &DevQProperty::distComplete, this, &DeviceManager::distComplete);
    connect(dev, &DevQProperty::usblSolutionComplete, this, &DeviceManager::usblSolutionComplete);
    connect(dev, &DevQProperty::beaconActivationComplete, this, &DeviceManager::beaconActivationReceive);
    connect(dev, &DevQProperty::dopplerBeamComplete, this, &DeviceManager::dopplerBeamComlete);
    connect(dev, &DevQProperty::dvlSolutionComplete, this, &DeviceManager::dvlSolutionComplete);
    connect(dev, &DevQProperty::upgradeProgressChanged, this, &DeviceManager::upgradeProgressChanged);

    dev->startConnection(link != NULL);
#endif

    emit devChanged();

    return dev;
}
