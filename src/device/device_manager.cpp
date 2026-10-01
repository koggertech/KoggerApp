#include "device_manager.h"
#include "device_defs.h"
#include <QDateTime>
#include "location_reader.h"
#include "core.h"
#include "frame_codec.h"
#include "autopilot_messages.h"
extern Core core;

namespace {

constexpr qint64 kAutopilotHeartbeatTimeoutMs = 3000;
constexpr int kLinkWindowSeconds = 5;
constexpr int kLinkMaxGap = 128;
constexpr qint64 kRadioStatusTimeoutMs = 5000;
constexpr qint64 kProxyTrafficTimeoutMs = 3000;
constexpr int kRadioRssiUnknown = 255;
constexpr int kSikSystemId = '3';
constexpr int kSikComponentId = 'D';

int radioDbm(int raw, bool sik)
{
    if (sik) {
        return std::clamp(int(std::lround(double(raw) / 1.9 - 127.0)), -120, 0);
    }
    return int(int8_t(uint8_t(raw)));
}

} // namespace


DeviceManager::DeviceManager()
    : lastDevs_(nullptr),
      lastDevice_(nullptr),
      mavlinkLink_(nullptr),
      streamList_(this),
      lastAddress_(-1),
      progress_(0),
      isConsoled_(false),
      nmeaConsoled_(true),
      break_(false),
      upgradeUuid_(QUuid()),
      upgradeAddr_(0)
{
    qRegisterMetaType<ProtoBinOut>("ProtoBinOut");
    qRegisterMetaType<Parsers::ProtoBinOut>("Parsers::ProtoBinOut");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<int16_t>("int16_t");
    qRegisterMetaType<QVector<uint8_t>>("QVector<uint8_t>");
    qRegisterMetaType<QByteArray>("QByteArray");
    qRegisterMetaType<IDBinUsblSolution::UsblSolution>("IDBinUsblSolution::UsblSolution");
    qRegisterMetaType<IDBinUsblSolution::AcousticNavSolution>("IDBinUsblSolution::AcousticNavSolution");
    qRegisterMetaType<IDBinUsblSolution::BaseToBeacon>("IDBinUsblSolution::BaseToBeacon");
    qRegisterMetaType<IDBinModemSolution::ModemSolutionHeader>("IDBinModemSolution::ModemSolutionHeader");
    qRegisterMetaType<IDBinDVL::BeamSolution>("IDBinDVL::BeamSolution");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<IDBinDVL::DVLSolution>("IDBinDVL::DVLSolution");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<FrameParser>("FrameParser");

    heartbeatClock_.start();
    autopilotTimer_.setInterval(1000);
    QObject::connect(&autopilotTimer_, &QTimer::timeout, this, &DeviceManager::checkAutopilotOnline);
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

int DeviceManager::vruBatteryPercent()
{
    return vru_.batteryPercent;
}

bool DeviceManager::autopilotOnline()
{
    return vru_.online;
}

int DeviceManager::autopilotSystemId()
{
    return vru_.systemId;
}

void DeviceManager::autopilotArm(bool arm, bool force)
{
    sendCommandLong(MavCmdComponentArmDisarm, arm ? 1.0f : 0.0f, force ? kMavArmForceMagic : 0.0f);
}

void DeviceManager::autopilotSetMode(int customMode)
{
    if (customMode < 0) {
        return;
    }
    sendCommandLong(MavCmdDoSetMode, float(kMavModeFlagCustomModeEnabled), float(customMode));
}

void DeviceManager::autopilotStartMission()
{
    sendCommandLong(MavCmdMissionStart);
}

void DeviceManager::sendCommandLong(uint16_t command, float p1, float p2, float p3, float p4, float p5, float p6, float p7)
{
    if (!vru_.online || vru_.systemId < 0 || autopilotLink_ == nullptr) {
        emit autopilotCommandAcked(command, MavResultNotSent);
        return;
    }

    MAVLink_MSG_COMMAND_LONG cmd;
    cmd.param1 = p1;
    cmd.param2 = p2;
    cmd.param3 = p3;
    cmd.param4 = p4;
    cmd.param5 = p5;
    cmd.param6 = p6;
    cmd.param7 = p7;
    cmd.command = command;
    cmd.target_system = uint8_t(vru_.systemId);
    cmd.target_component = uint8_t(vru_.componentId);
    cmd.confirmation = 0;

    emit writeMavlinkBytes(autopilot::encodeFrame(vru_.mavlinkVersion, mavlinkSeq_++, MAVLink_MSG_COMMAND_LONG::getID(),
                                                autopilot::payloadOf(cmd), MAVLink_MSG_COMMAND_LONG::v1Length()));
#ifndef SEPARATE_READING
    core.consoleInfo(QString("<< MAVLink: COMMAND_LONG %1 (%2, %3) to sys %4").arg(command).arg(p1).arg(p2).arg(vru_.systemId));
#endif
}

double DeviceManager::vehicleHomeLat()
{
    return vru_.homeLat;
}

double DeviceManager::vehicleHomeLon()
{
    return vru_.homeLon;
}

void DeviceManager::requestVehicleHome()
{
    sendCommandLong(MavCmdRequestMessage, float(MAVLink_MSG_HOME_POSITION::getID()));
}

void DeviceManager::sendAutopilotMessage(quint32 msgId, const QByteArray& payload, int v1Length)
{
    if (!vru_.online || autopilotLink_ == nullptr) {
        return;
    }
    emit writeMavlinkBytes(autopilot::encodeFrame(vru_.mavlinkVersion, mavlinkSeq_++, msgId, payload, v1Length));
}

void DeviceManager::startMissionUpload(const autopilot::MissionBatches& batches)
{
    if (!vru_.online || vru_.systemId < 0 || autopilotLink_ == nullptr) {
        emit missionUploadRefused(autopilot::MissionTransfer::ResultNoLink, MavMissionTypeMission);
        return;
    }
#ifndef SEPARATE_READING
    QStringList sizes;
    for (const auto& b : batches) {
        sizes.append(QString::number(b.items.size()));
    }
    core.consoleInfo(QString("<< MAVLink: mission upload (route/fence/rally items %1) to sys %2").arg(sizes.join(QLatin1Char('/'))).arg(vru_.systemId));
#endif
    if (vru_.mavlinkVersion == 1) {
        autopilot::MissionBatches routeOnly;
        for (const auto& b : batches) {
            if (b.missionType == MavMissionTypeMission) {
                routeOnly.append(b);
            } else if (!b.items.isEmpty()) {
                emit missionUploadRefused(autopilot::MissionTransfer::ResultMavlink1, MavMissionTypeMission);
                return;
            }
        }
        emit missionUploadStart(routeOnly, vru_.systemId, vru_.componentId);
        return;
    }
    emit missionUploadStart(batches, vru_.systemId, vru_.componentId);
}

void DeviceManager::startMissionDownload()
{
    if (!vru_.online || vru_.systemId < 0 || autopilotLink_ == nullptr) {
        emit missionDownloadRefused(autopilot::MissionTransfer::ResultNoLink);
        return;
    }
#ifndef SEPARATE_READING
    core.consoleInfo(QString("<< MAVLink: mission download (route/fence/rally) from sys %1").arg(vru_.systemId));
#endif
    if (vru_.mavlinkVersion == 1) {
        emit missionDownloadStart({ MavMissionTypeMission }, vru_.systemId, vru_.componentId);
        return;
    }
    emit missionDownloadStart({ MavMissionTypeMission, MavMissionTypeFence, MavMissionTypeRally }, vru_.systemId, vru_.componentId);
}

void DeviceManager::publishVru()
{
    AutopilotState state;
    state.voltage = vru_.voltage;
    state.current = vru_.current;
    state.velocityH = vru_.velocityH;
    state.batteryPercent = vru_.batteryPercent;
    state.armState = vru_.armState;
    state.flightMode = vru_.flightMode;
    state.online = vru_.online;
    state.proxyTraffic = proxyTraffic_;
    state.homeLat = vru_.homeLat;
    state.homeLon = vru_.homeLon;
    state.linkQuality = vru_.online ? linkQuality_ : -1;
    state.radioRssiValid = vru_.online && radioRssiMs_ >= 0;
    state.radioRssi = radioRssi_;
    emit vruChanged(state);
}

bool DeviceManager::proxyLinkActive()
{
    return proxyTraffic_;
}

void DeviceManager::checkAutopilotOnline()
{
    if (!vru_.online) {
        autopilotTimer_.stop();
        return;
    }
    if (heartbeatClock_.elapsed() - vru_.lastHeartbeatMs > kAutopilotHeartbeatTimeoutMs) {
        vru_.online = false;
        vru_.homeLat = NAN;
        vru_.homeLon = NAN;
        autopilotTimer_.stop();
        unbindAutopilotLink();
        resetLinkQuality();
        publishVru();
        return;
    }
    if (proxyTraffic_ && heartbeatClock_.elapsed() - proxyTrafficMs_ > kProxyTrafficTimeoutMs) {
        proxyTraffic_ = false;
        publishVru();
    }
    updateLinkQuality();
}

void DeviceManager::resetLinkQuality()
{
    linkLastSeq_ = -1;
    linkReceived_ = 0;
    linkLost_ = 0;
    linkWindow_.clear();
    linkQuality_ = -1;
    radioRssiMs_ = -1;
}

void DeviceManager::countAutopilotFrame(int seq)
{
    if (linkLastSeq_ >= 0) {
        const int gap = (seq - linkLastSeq_ - 1) & 0xFF;
        if (gap < kLinkMaxGap) {
            linkLost_ += gap;
        }
    }
    linkLastSeq_ = seq;
    ++linkReceived_;
}

void DeviceManager::updateLinkQuality()
{
    linkWindow_.append(qMakePair(linkReceived_, linkLost_));
    while (linkWindow_.size() > kLinkWindowSeconds) {
        linkWindow_.removeFirst();
    }
    linkReceived_ = 0;
    linkLost_ = 0;
    int received = 0;
    int lost = 0;
    for (const auto& bucket : std::as_const(linkWindow_)) {
        received += bucket.first;
        lost += bucket.second;
    }
    const int quality = received + lost > 0 ? int(std::lround(100.0 * received / (received + lost))) : linkQuality_;
    const bool radioStale = radioRssiMs_ >= 0 && heartbeatClock_.elapsed() - radioRssiMs_ > kRadioStatusTimeoutMs;
    if (radioStale) {
        radioRssiMs_ = -1;
    }
    if (quality != linkQuality_ || radioStale) {
        linkQuality_ = quality;
        publishVru();
    }
}

void DeviceManager::bindAutopilotLink(QUuid uuid, Link* link)
{
    if (autopilotLink_ == link && autopilotLinkUuid_ == uuid) {
        return;
    }
    unbindAutopilotLink();
    autopilotLink_ = link;
    autopilotLinkUuid_ = uuid;
    connect(this, &DeviceManager::writeMavlinkBytes, autopilotLink_, &Link::write, Qt::UniqueConnection);
}

void DeviceManager::resetAutopilot()
{
    autopilotTimer_.stop();
    unbindAutopilotLink();
    resetLinkQuality();
    vru_.cleanVru();
}

void DeviceManager::unbindAutopilotLink()
{
    if (autopilotLink_ != nullptr) {
        disconnect(this, &DeviceManager::writeMavlinkBytes, autopilotLink_, &Link::write);
        emit missionTransferAbort(autopilot::MissionTransfer::ResultLinkLost);
    }
    autopilotLink_ = nullptr;
    autopilotLinkUuid_ = QUuid();
}

int DeviceManager::calcAverageChartLosses()
{
    int averageChartLosses = 0;
    int numOfDevices = 0;

    for (auto i = devTree_.cbegin(), end = devTree_.cend(); i != end; ++i) {
        const auto& devs = i.value();

        for (auto k = devs.cbegin(), end = devs.cend(); k != end; ++k) {
            if (!k.value()->isBoardInited()) {
                continue;
            }
            ++numOfDevices;
            averageChartLosses += k.value()->getAverageChartLosses();
        }
    }

    return numOfDevices != 0 ? averageChartLosses / numOfDevices : -1;
}

void DeviceManager::initStreamList()
{
    streamList_.initTimer();
}

QList<DevQProperty *> DeviceManager::getDevList()
{
    devList_.clear();

    for (auto i = devTree_.cbegin(), end = devTree_.cend(); i != end; ++i) {
        const auto& devs = i.value();

        for (auto k = devs.cbegin(), end = devs.cend(); k != end; ++k) {
            devList_.append(k.value());
        }
    }

    return devList_;
}

QList<DevQProperty *> DeviceManager::getDevList(BoardVersion ver) {
    QList<DevQProperty *> list;

    for (auto i = devTree_.cbegin(), end = devTree_.cend(); i != end; ++i) {
        const auto& devs = i.value();

        for (auto k = devs.cbegin(), end = devs.cend(); k != end; ++k) {
            if(k.value()->boardVersion() == ver) {
                list.append(k.value());
            }
        }
    }

    return list;
}

void DeviceManager::frameInput(QUuid uuid, Link* link, Parsers::FrameParser frame)
{
    if (loggingStarted_) {
        emit sendFrameInputToLogger(uuid, link, frame);
    }

    if (frame.isComplete()) {

        if (frame.isStream())
            streamList_.append(&frame);
        if (frame.id() == ID_STREAM)
            streamList_.parse(&frame);
        if (streamList_.isListChenged()) {
            emit streamChanged();
            // qInfo("stream-list: %d logs", streamList_.streamsList()->size());
            static const QString kAutoDl = qEnvironmentVariable("KOGGER_AUTODOWNLOAD");
            if (!kAutoDl.isEmpty() && !autoDownloadStarted_) {
                autoDownloadStarted_ = true;
                startStreamDownload(kAutoDl.toInt());
            }
        }

        if (link != nullptr) {
            if (frame.isProxy() || frame.completeAsKBP()) {
                otherProtocolStat_.remove(uuid);
            }
        }

        if (frame.isProxy()) {
            return; //continue;
        }

        if (frame.completeAsKBP() || frame.completeAsKBP2()) {
            DevQProperty* dev = getDevice(uuid, link, frame.route());

            if (isConsoled_ && link && frame.id() != 32 && frame.id() != 33) { // link ptr check added
#ifndef SEPARATE_READING
                core.consoleProto(frame);
#endif
            }

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
#ifndef SEPARATE_READING
            if (nmeaConsoled_) {
                core.consoleProtoText(QString(">> NMEA: %5").arg(str_data));
            }
#endif
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
                    uint8_t month = 0, day = 0;
                    prot_nmea.readDate(&year, &month, & day);

                    QDate date(year, month, day);
                    QTime time(h, m, s);

                    QDateTime dt(date, time, QTimeZone::utc());
                    uint32_t unix_time = static_cast<uint32_t>(dt.toSecsSinceEpoch());

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
                    uint8_t month = 1, day = 1;

                    Position pos;
                    pos.lla.latitude = lat;
                    pos.lla.longitude = lon;
                    pos.lla.altitude = height_msl;
                    pos.lla.source = PositionSourceRTK;
                    pos.lla.altSource = AltitudeSourceRTK;
                    pos.time = DateTime(year, month, day, h, m, s, int64_t(ms)*1000*1000);

                    if(q == '4') {
                        emit positionCompleteRTK(pos);
                    }
                }
            }

            if (prot_nmea.isEqualId("HDT")) {
                const double headingDeg = prot_nmea.readDouble();
                //const char reference = prot_nmea.readChar();
                const bool headingValid = isfinite(headingDeg);

                if (/*reference == 'T' && TODO: check this*/ headingValid) {
                    //qDebug().noquote() << QString("NMEA HDT parsed: heading=%1 deg true").arg(heading_deg, 0, 'f', 3);
                    emit attitudeComplete(static_cast<float>(headingDeg), 0.0f, 0.0f);
                }
                else {
                    //qDebug().noquote() << QString("NMEA HDT rejected: heading=%1 ref=%2")
                    //                      .arg(heading_valid ? QString::number(heading_deg, 'f', 3) : QStringLiteral("nan"))
                    //                      .arg(reference);
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
                Q_UNUSED(satellites_in_used)

                int32_t lon_int = ubx_frame.read<S4>();
                int32_t lat_int = ubx_frame.read<S4>();

                QDate date(year, month, day);
                QTime time(h, m, s);

                QDateTime dt(date, time, QTimeZone::utc());
                uint32_t unix_time = static_cast<uint32_t>(dt.toSecsSinceEpoch());

                if (fix_type > 1 && fix_type < 5) {
                    emit positionComplete(double(lat_int)*0.0000001, double(lon_int)*0.0000001, unix_time, nanosec);
                }

                // if (isConsoled_) {
#ifndef SEPARATE_READING
                    core.consoleStreamInfo(QString(">> UBX: NAV_PVT, fix %1, sats %2, lat %3, lon %4, time %5:%6:%7.%8")
                                         .arg(fix_type).arg(satellites_in_used).arg(double(lat_int)*0.0000001).arg(double(lon_int)*0.0000001).arg(h).arg(m).arg(s).arg(nanosec/1000));
#endif
                    // }
            }
            else {
                // if (isConsoled_)
#ifndef SEPARATE_READING
                    core.consoleStreamInfo(QString(">> UBX: class/id 0x%1 0x%2, len %3").arg(ubx_frame.msgClass(), 2, 16, QLatin1Char('0')).arg(ubx_frame.msgId(), 2, 16, QLatin1Char('0')).arg(ubx_frame.frameLen()));
#endif
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
                const bool fromAutopilot = !vru_.online
                                           || (uuid == autopilotLinkUuid_ && int(mavlink_frame.systemID()) == vru_.systemId);
                if (vru_.online && uuid == autopilotLinkUuid_) {
                    if (int(mavlink_frame.systemID()) == vru_.systemId && int(mavlink_frame.componentID()) == vru_.componentId) {
                        countAutopilotFrame(mavlink_frame.sequenceNumber());
                    }
                    const bool radioStatus = mavlink_frame.msgId() == MAVLink_MSG_RADIO_STATUS::getID();
                    const auto radio = radioStatus ? mavlink_frame.read<MAVLink_MSG_RADIO_STATUS>() : MAVLink_MSG_RADIO_STATUS{};
                    if (radioStatus && radio.rssi != kRadioRssiUnknown) {
                        const bool sik = mavlink_frame.systemID() == kSikSystemId && mavlink_frame.componentID() == kSikComponentId;
                        const int dbm = radioDbm(radio.rssi, sik);
                        const bool changed = radioRssiMs_ < 0 || dbm != radioRssi_;
                        radioRssi_ = dbm;
                        radioRssiMs_ = heartbeatClock_.elapsed();
                        if (changed) {
                            publishVru();
                        }
                    }
                }

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
                        publishVru();
                    }
                }

                MAVLink_MSG_HEARTBEAT heartbeat;
                bool isAutopilotHeartbeat = false;
                if (fromAutopilot && mavlink_frame.msgId() == MAVLink_MSG_HEARTBEAT::getID()
                    && mavlink_frame.componentID() == kMavCompIdAutopilot1) {
                    heartbeat = mavlink_frame.read<MAVLink_MSG_HEARTBEAT>();
                    isAutopilotHeartbeat = heartbeat.isVehicle();
                }
                if (isAutopilotHeartbeat) {
                    if (link != nullptr && !vru_.online) {
                        bindAutopilotLink(uuid, link);
                    }
                    const bool wasOnline = vru_.online;
                    const int previousArm = vru_.armState;
                    vru_.armState = (int)heartbeat.isArmed();
                    int flight_mode = (int)heartbeat.customMode();
                    if (flight_mode != vru_.flightMode) {
#ifndef SEPARATE_READING
                        core.consoleStreamInfo(QString(">> FC: Flight mode %1").arg(flight_mode));
#endif
                    }
                    vru_.flightMode = flight_mode;
                    vru_.systemId = mavlink_frame.systemID();
                    vru_.componentId = mavlink_frame.componentID();
                    vru_.mavlinkVersion = mavlink_frame.MAVLinkVersion();
                    vru_.lastHeartbeatMs = heartbeatClock_.elapsed();
                    vru_.online = link != nullptr;
                    if (vru_.online && !autopilotTimer_.isActive()) {
                        autopilotTimer_.start();
                    }
                    if (vru_.online && !wasOnline) {
                        resetLinkQuality();
                    }
                    if (vru_.online && (!wasOnline || (previousArm == 0 && vru_.armState == 1))) {
                        requestVehicleHome();
                    }
                    publishVru();
                }

                if (fromAutopilot && vru_.online && mavlink_frame.msgId() == MAVLink_MSG_COMMAND_ACK::getID()
                    && int(mavlink_frame.componentID()) == vru_.componentId) {
                    MAVLink_MSG_COMMAND_ACK ack = mavlink_frame.read<MAVLink_MSG_COMMAND_ACK>();
                    const bool addressedToUs = (ack.target_system == 0 || ack.target_system == kMavGcsSystemId)
                                               && (ack.target_component == 0 || ack.target_component == kMavGcsComponentId);
                    if (addressedToUs) {
#ifndef SEPARATE_READING
                        core.consoleInfo(QString(">> MAVLink: COMMAND_ACK %1 result %2").arg(ack.command).arg(ack.result));
#endif
                        emit autopilotCommandAcked(ack.command, ack.result);
                    }
                }

                const quint32 msgId = mavlink_frame.msgId();
                if (fromAutopilot && vru_.online
                    && (msgId == MAVLink_MSG_MISSION_REQUEST::getID() || msgId == MAVLink_MSG_MISSION_REQUEST_INT::getID()
                        || msgId == MAVLink_MSG_MISSION_ACK::getID() || msgId == MAVLink_MSG_MISSION_COUNT::getID()
                        || msgId == MAVLink_MSG_MISSION_ITEM_INT::getID())) {
                    const uint16_t length = mavlink_frame.payloadLen();
                    emit missionFrameReceived(msgId, QByteArray(reinterpret_cast<const char*>(mavlink_frame.read(length)), length));
                }

                if (fromAutopilot && vru_.online && msgId == MAVLink_MSG_HOME_POSITION::getID()) {
                    const MAVLink_MSG_HOME_POSITION home = mavlink_frame.read<MAVLink_MSG_HOME_POSITION>();
                    vru_.homeLat = double(home.latitude) / 1.0e7;
                    vru_.homeLon = double(home.longitude) / 1.0e7;
                    publishVru();
                }

                if (fromAutopilot && mavlink_frame.msgId() == 147) { // BATTERY_STATUS
                    MAVLink_MSG_BATTERY_STATUS battery_status = mavlink_frame.read<MAVLink_MSG_BATTERY_STATUS>();
                    vru_.voltage = battery_status.voltage();
                    vru_.current = battery_status.current();
                    vru_.batteryPercent = battery_status.battery_remaining >= 0 ? int(battery_status.battery_remaining) : -1;
                    publishVru();
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
#ifndef SEPARATE_READING
                core.consoleProtoText(QString(">> MAVLink v%1: ID %2, comp. id %3, seq numb %4, len %5").arg(mavlink_frame.MAVLinkVersion()).arg(mavlink_frame.msgId()).arg(mavlink_frame.componentID()).arg(mavlink_frame.sequenceNumber()).arg(mavlink_frame.frameLen()));
#endif
            }
            else {
                if (link != nullptr) {
                    proxyTrafficMs_ = heartbeatClock_.elapsed();
                    if (!proxyTraffic_) {
                        proxyTraffic_ = true;
                        publishVru();
                    }
                    emit writeMavlinkFrame(frame);
                }
            }
        }

        if (link != nullptr) {
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

        QByteArray chunk = file.read(static_cast<qint64>(1024) * 1024);
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
            if (sleepCnt > 10) {
                QThread::msleep(1);
                sleepCnt = 0;
            }
            ++sleepCnt;
#endif
            frameParser.process();
            if (frameParser.isComplete()) {
                frameInput(someUuid, nullptr, frameParser);
#ifdef SEPARATE_READING
                if (!fileReadEnough) { // TODO: check this
                    emit onFileReadEnough();
                    fileReadEnough = true;
                }
#endif
            }
        }

        chunk.clear();
    }
    file.close();

    resetAutopilot();
    delAllDev();
    publishVru();

    emit fileOpened();
    emit fileStopsOpening();
}

#ifdef SEPARATE_READING
void DeviceManager::closeFile(bool onOpen)
{
    onOpen_ = onOpen;
    break_ = true;

    resetAutopilot();
    delAllDev();
    publishVru();
}
#else
void DeviceManager::closeFile()
{
    delAllDev();
    resetAutopilot();
    publishVru();
}
#endif

void DeviceManager::onLinkOpened(QUuid uuid, Link *link)
{
    if (link) {
        if (link->getIsProxy()) {
            proxyLinkUuid_ = uuid;
            connect(this, &DeviceManager::writeProxyFrame, link, &Link::writeFrame);
            publishVru();
        } else if(link->attribute() == LinkAttribute::kLinkAttributeBoot) {
#ifndef SEPARATE_READING
            core.consoleInfo("Device: Boot opened");
#endif
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
            mavlinkLink_ = nullptr;
        }
        if(uuid == proxyLinkUuid_) {
            proxyLinkUuid_ = QUuid();
            proxyTraffic_ = false;
            publishVru();
        }
        if(uuid == autopilotLinkUuid_) {
            resetAutopilot();
            publishVru();
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
            mavlinkLink_ = nullptr;
        }
        if(uuid == proxyLinkUuid_) {
            proxyLinkUuid_ = QUuid();
            proxyTraffic_ = false;
            publishVru();
        }
        if(uuid == autopilotLinkUuid_) {
            resetAutopilot();
            publishVru();
        }
    }
}

void DeviceManager::binFrameOut(Parsers::ProtoBinOut protoOut)
{
    if (isConsoled_ && protoOut.id() != 33) {
#ifndef SEPARATE_READING
        core.consoleProto(protoOut, false);
#endif
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

void DeviceManager::setNmeaConsoled(bool isConsoled)
{
    nmeaConsoled_ = isConsoled;
}

void DeviceManager::upgradeLastDev(QByteArray data)
{
    if (lastDevs_ != nullptr) {
        lastDevs_->sendUpdateFW(data);
    }
}

void DeviceManager::beaconActivationReceive(uint8_t id) {
    Q_UNUSED(id)

    QList<DevQProperty *> usbl_devs = getDevList(BoardUSBL);
    if (!usbl_devs.isEmpty()) {
        IDBinUsblSolution::USBLRequestBeacon ask = {};
        usbl_devs[0]->askBeaconPosition(ask);
    }
}

void DeviceManager::beaconDirectQueueAsk() {
    QList<DevQProperty *> usbl_devs = getDevList(BoardUSBLBeacon);
    qDebug("Sent request to the Beacon # %d", -1);
    if (!usbl_devs.isEmpty()) {
        usbl_devs[0]->enableBeaconOnce(3);
        qDebug("Sent request to the Beacon # %d", 0);
    }
}

void DeviceManager::setUSBLBeaconDirectAsk(bool is_ask) {
    isUSBLBeaconDirectAsk = is_ask;
    qDebug("Beacon auto scan is: %d", is_ask);
    if (is_ask) {
        QObject::connect(&beacon_timer, &QTimer::timeout, this, &DeviceManager::beaconDirectQueueAsk);
        beacon_timer.setInterval(3000);
        beacon_timer.start();
    } else {
        beacon_timer.stop();
    }
}

void DeviceManager::onLoggingKlfStarted(bool started)
{
    loggingStarted_ = started;

    if (loggingStarted_) {
        for (auto i = devTree_.cbegin(), end = devTree_.cend(); i != end; ++i) {
            const auto& devs = i.value();
            for (auto k = devs.cbegin(), end = devs.cend(); k != end; ++k) {
                k.value()->requestSetup();
            }
        }
    }
}

void DeviceManager::onSendRequestAll(QUuid uuid)
{
    if (devTree_.contains(uuid)) {
        const auto& devs = devTree_[uuid];
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

void DeviceManager::startStreamDownload(int id)
{
    QList<DevQProperty*> recs = getDevList(BoardRecorderMini);
    if (recs.isEmpty()) {
        qInfo("startStreamDownload: no recorder device connected");
        return;
    }
    DevQProperty* rec = recs.first();
    qRegisterMetaType<QVector<quint32>>("QVector<quint32>");
    connect(&streamList_, &StreamList::requestRanges, rec, &DevDriver::requestStreamRanges, Qt::UniqueConnection);
    streamList_.startDownload(id);
}

void DeviceManager::cancelStreamDownload(int id)
{
    streamList_.cancelDownload(id);
}

void DeviceManager::refreshStreamList()
{
    QList<DevQProperty*> recs = getDevList(BoardRecorderMini);
    if (!recs.isEmpty()) {
        if (!streamList_.hasActiveDownload()) {
            streamList_.reset();
            emit streamChanged();
        }
        recs.first()->requestStreamList();
    }
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

void DeviceManager::createLocationReader()
{
    //qDebug() << "DeviceManager::createLocationReader";

    if (locReader_) {
        return;
    }

    locReader_ = new LocationReader(this);
    connect(locReader_, &LocationReader::positionUpdated, this, &DeviceManager::onPositionUpdated, Qt::QueuedConnection);
    connect(locReader_, &LocationReader::gpsAlive, &core, &Core::setIsGPSAlive, Qt::QueuedConnection);
}

void DeviceManager::destroyLocationReader()
{
    if (!locReader_) {
        return;
    }

    locReader_->deleteLater();
    locReader_ = nullptr;
}

void DeviceManager::shutdown()
{
    destroyLocationReader();
}

void DeviceManager::onPositionUpdated(const QGeoPositionInfo &info)
{
    if (!useGPS_) {
        return;
    }

    IDBinNav::SimpleNav smplNav;
    smplNav.latitude = info.coordinate().latitude();
    smplNav.longitude = info.coordinate().longitude();
    smplNav.depth = 0;
    smplNav.yaw = info.attribute(QGeoPositionInfo::Attribute::Direction) ;
    smplNav.pitch = 0;
    smplNav.roll = 0;

    emit positionComplete(smplNav.latitude, smplNav.longitude, info.timestamp().toSecsSinceEpoch(), info.timestamp().toMSecsSinceEpoch());
    emit attitudeComplete(smplNav.yaw, 0.0, 0.0);

    // LOGGING
    if (loggingStarted_) {
        ProtoBinOut req_out;
        req_out.create(Parsers::CONTENT, IDBinNav::SimpleNav::getVer(), IDBinNav::SimpleNav::getId(), 0/*m_address*/);
        req_out.write<IDBinNav::SimpleNav>(smplNav);
        req_out.end();

        //QString str1 = "emit coords 1 " + QString::number(smplNav.latitude, 'f', 4) + " " + QString::number(smplNav.longitude, 'f', 4) + " " + QString::number(smplNav.depth, 'f', 4) + " "
        //+ QString::number(smplNav.yaw, 'f', 4) + " " + QString::number(smplNav.pitch, 'f', 4) + " " + QString::number(smplNav.roll, 'f', 4);
        //core.consoleInfo(str1);
        //QString str2 = "emit coords 2 " + QString::number(req_out.binError()) + " " + QString::number(req_out.isComplete()) + " " + QString::number(req_out.payloadLen()) + " " + QString::number(req_out.frameLen()) + " "
        //               + QString::number(req_out.readAvailable()) + " " + QString::number(req_out.availContext());
        //core.consoleInfo(str2);

        emit sendFrameInputToLogger(QUuid(), nullptr, req_out);
    }
}

void DeviceManager::setUseGPS(bool state)
{
    //qDebug() << "DeviceManager::setUseGPS" << state;
    useGPS_ = state;
}

DevQProperty* DeviceManager::getDevice(QUuid uuid, Link *link, uint8_t addr)
{
    if ((link == nullptr || lastUuid_ == uuid) && lastAddress_ == addr && lastDevice_ != nullptr) {
        return lastDevice_;
    }
    else {
        lastDevice_ = devTree_[uuid][addr];
        if (lastDevice_ == nullptr) {
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
        const auto& devs = devTree_[uuid];
        bool hadRecorder = false;
        for (auto i = devs.cbegin(), end = devs.cend(); i != end; ++i) {
            if (i.value() && i.value()->isRecorder()) {
                hadRecorder = true;
            }
            if (lastDevice_ == i.value()) {
                lastDevice_ = nullptr;
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
        if (hadRecorder) {
            streamList_.reset();
            emit streamChanged();
        }
        emit devChanged();
        emit standAvailableChanged();
    }
}

bool DeviceManager::standAvailable()
{
    const QList<DevQProperty*> devs = getDevList();
    for (DevQProperty* dev : devs) {
        if (dev && dev->getStandState())
            return true;
    }
    return false;
}

DevQProperty* DeviceManager::createDev(QUuid uuid, Link* link, uint8_t addr)
{
    DevQProperty* dev = new DevQProperty();
    devTree_[uuid][addr] = dev;
    dev->setBusAddress(addr);
    dev->setLinkUuid(uuid);

#ifdef SEPARATE_READING
    auto connType = Qt::AutoConnection;

    if (link != nullptr) {
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
    connect(dev, &DevQProperty::standChanged, this, &DeviceManager::standAvailableChanged, connType);

    connect(dev, &DevQProperty::chartComplete, this, &DeviceManager::chartComplete, connType);
    connect(dev, &DevQProperty::rawDataRecieved, this, &DeviceManager::rawDataRecieved, connType);
    connect(dev, &DevQProperty::attitudeComplete, this, &DeviceManager::attitudeComplete, connType);
    connect(dev, &DevQProperty::tempComplete, this, &DeviceManager::tempComplete, connType);
    connect(dev, &DevQProperty::distComplete, this, &DeviceManager::distComplete, connType);
    connect(dev, &DevQProperty::usblSolutionComplete, this, &DeviceManager::usblSolutionComplete, connType);
    connect(dev, &DevQProperty::dopplerBeamComplete, this, &DeviceManager::dopplerBeamComlete, connType);
    connect(dev, &DevQProperty::dvlSolutionComplete, this, &DeviceManager::dvlSolutionComplete, connType);
    connect(dev, &DevQProperty::upgradeProgressChanged, this, &DeviceManager::upgradeProgressChanged, connType);

    connect(dev, &DevQProperty::positionComplete, this, &DeviceManager::positionComplete, connType);
    connect(dev, &DevQProperty::gnssVelocityComplete, this, &DeviceManager::gnssVelocityComplete, connType);
    connect(dev, &DevQProperty::simpleNavV2Complete, this, &DeviceManager::simpleNavV2Complete, connType);
    connect(dev, &DevQProperty::boatStatusComplete, this, &DeviceManager::boatStatusComplete, connType);
    connect(dev, &DevQProperty::depthComplete, this, &DeviceManager::depthComplete, connType);

    dev->moveToThread(qApp->thread());
    dev->getProcessTimer()->moveToThread(qApp->thread());
    QList<QTimer*> timers = dev->getChildTimers();
    foreach (QTimer* timer, timers) {
        timer->moveToThread(qApp->thread());
    }

    QMetaObject::invokeMethod(dev, "initProcessTimerConnects", Qt::QueuedConnection);
    QMetaObject::invokeMethod(dev, "initChildsTimersConnects", Qt::QueuedConnection);
    QMetaObject::invokeMethod(dev, "startConnection", Qt::QueuedConnection, Q_ARG(bool, link != nullptr));
#else
    if (link != nullptr) {
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
    connect(dev, &DevQProperty::tempComplete, this, &DeviceManager::tempComplete);
    connect(dev, &DevQProperty::distComplete, this, &DeviceManager::distComplete);
    connect(dev, &DevQProperty::encoderComplete, this, &DeviceManager::encoderComplete);
    connect(dev, &DevQProperty::usblSolutionComplete, this, &DeviceManager::usblSolutionComplete);
    connect(dev, &DevQProperty::beaconActivationComplete, this, &DeviceManager::beaconActivationReceive);
    connect(dev, &DevQProperty::dopplerBeamComplete, this, &DeviceManager::dopplerBeamComlete);
    connect(dev, &DevQProperty::dvlSolutionComplete, this, &DeviceManager::dvlSolutionComplete);
    connect(dev, &DevQProperty::upgradeProgressChanged, this, &DeviceManager::upgradeProgressChanged);

    connect(dev, &DevQProperty::positionComplete, this, &DeviceManager::positionComplete);
    connect(dev, &DevQProperty::gnssVelocityComplete, this, &DeviceManager::gnssVelocityComplete);
    connect(dev, &DevQProperty::simpleNavV2Complete, this, &DeviceManager::simpleNavV2Complete);
    connect(dev, &DevQProperty::boatStatusComplete, this, &DeviceManager::boatStatusComplete);
    connect(dev, &DevQProperty::depthComplete, this, &DeviceManager::depthComplete);

    dev->startConnection(link != nullptr);
#endif

    if (link != nullptr) {
        auto syncStatus = [dev, link](QUuid = {}) {
            dev->setLinkStatus(link->getConnectionStatus(),
                               link->getIsRecievesData(),
                               link->getIsNotAvailable());
        };
        connect(link, &Link::connectionStatusChanged, dev, syncStatus);
        connect(link, &Link::isReceivesDataChanged,   dev, syncStatus);
        connect(link, &Link::isNotAvailableChanged,   dev, syncStatus);
        syncStatus();
    }

    if (upgradeUuid_ == uuid && upgradeAddr_ == addr) {
        upgradeUuid_ = QUuid();
        QMetaObject::invokeMethod(dev, [dev, firmware = upgradeData_]() {
            dev->setFirmware(firmware);
        }, Qt::QueuedConnection);
    }

    emit devChanged();

    return dev;
}
