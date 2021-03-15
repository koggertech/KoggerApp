#include "SonarDriver.h"
#include <time.h>
#include <core.h>
extern Core core;

SonarDriver::SonarDriver(QObject *parent) :
    QObject(parent),
    m_proto(new ProtoBinIn()),
    idTimestamp(new IDBinTimestamp((ProtoBinIn*)m_proto)),
    idDist(new IDBinDist((ProtoBinIn*)m_proto)),
    idChart(new IDBinChart((ProtoBinIn*)m_proto)),
    idAtt(new IDBinAttitude((ProtoBinIn*)m_proto)),
    idTemp(new IDBinTemp((ProtoBinIn*)m_proto)),

    idDataset(new IDBinDataset((ProtoBinIn*)m_proto)),
    idDistSetup(new IDBinDistSetup((ProtoBinIn*)m_proto)),
    idChartSetup(new IDBinChartSetup((ProtoBinIn*)m_proto)),
    idTransc(new IDBinTransc((ProtoBinIn*)m_proto)),
    idSoundSpeed(new IDBinSoundSpeed((ProtoBinIn*)m_proto)),
    idUART(new IDBinUART((ProtoBinIn*)m_proto)),

    idVersion(new IDBinVersion((ProtoBinIn*)m_proto)),
    idMark(new IDBinMark((ProtoBinIn*)m_proto)),
    idFlash(new IDBinFlash((ProtoBinIn*)m_proto)),
    idBoot(new IDBinBoot((ProtoBinIn*)m_proto)),
    idUpdate(new IDBinUpdate((ProtoBinIn*)m_proto)),

    idNav(new IDBinNav((ProtoBinIn*)m_proto))
{
    regID(idTimestamp, &SonarDriver::receivedTimestamp);
    regID(idDist, &SonarDriver::receivedDist);
    regID(idChart, &SonarDriver::receivedChart);
    regID(idAtt, &SonarDriver::receivedAtt);
    regID(idTemp, &SonarDriver::receivedTemp);

    regID(idDataset, &SonarDriver::receivedDataset, true);
    regID(idDistSetup, &SonarDriver::receivedDistSetup, true);
    regID(idChartSetup, &SonarDriver::receivedChartSetup, true);
    regID(idTransc, &SonarDriver::receivedTransc, true);
    regID(idSoundSpeed, &SonarDriver::receivedSoundSpeed, true);
    regID(idUART, &SonarDriver::receivedUART, true);
    regID(idVersion, &SonarDriver::receivedVersion, true);

    regID(idMark, &SonarDriver::receivedMark);
    regID(idFlash, &SonarDriver::receivedFlash);
    regID(idBoot, &SonarDriver::receivedBoot);
    regID(idUpdate, &SonarDriver::receivedUpdate);

    regID(idNav, &SonarDriver::receivedNav);

    connect(&m_processTimer, &QTimer::timeout, this, &SonarDriver::process);
}

void SonarDriver::regID(IDBin* id_bin, void (SonarDriver::* method)(Type type, Version ver, Resp resp), bool is_setup) {
    hashIDParsing[id_bin->id()] = id_bin;

    if(is_setup) {
        hashIDSetup[id_bin->id()] = id_bin;
    }

    connect(id_bin, &IDBin::updateContent, this, method);
    connect(id_bin, &IDBin::dataSend, this, &SonarDriver::dataSend);
}


void SonarDriver::requestSetup() {
    QHashIterator<ID, IDBin*> i(hashIDSetup);
    while (i.hasNext()) {
        i.next();
        i.value()->requestAll();
    }

    m_state.conf = ConfRequest;
}

void SonarDriver::setBusAddress(int addr) {
    m_busAddress = addr;
    QHashIterator<ID, IDBin*> i(hashIDParsing);
    while (i.hasNext()) {
        i.next();
        i.value()->setAddress(m_busAddress);
    }
}

int SonarDriver::getBusAddress() {
    return m_busAddress;
}

void SonarDriver::setDevAddress(int addr) {
    idUART->setDevAddress(addr);
}

int SonarDriver::getDevAddress() {
    return idUART->devAddress();
}

void SonarDriver::setDevDefAddress(int addr) {
     idUART->setDevDefAddress(addr);
}

int SonarDriver::getDevDefAddress() {
    return idUART->devDefAddress();
}

uint32_t SonarDriver::devSerialNumber() {
    return idVersion->serialNumber();
}

QString SonarDriver::devPN() {

}

void SonarDriver::putData(const QByteArray &data) {
    if(m_state.connect) {
        uint8_t* ptr_data = (uint8_t*)(data.data());
        m_proto->setContext(ptr_data, data.size());

        while (m_proto->availContext()) {
            m_proto->process();
            switch (m_proto->protoFlag()) {
            case FrameParser::ProtoNone:
                //            qInfo("bin err %u, frame err %u, data size %u, context %u", m_proto->binError(), m_proto->frameError(), data.size(), m_proto->availContext());
                break;
            case FrameParser::ProtoBin:
                protoComplete(*(ProtoBinIn*)m_proto);
                break;
            case FrameParser::ProtoNMEA:
                nmeaComplete(*(ProtoNMEA*)m_proto);
                break;
            }
        }
    }
}

void SonarDriver::nmeaComplete(ProtoNMEA &proto) {
    if(proto.isEqualId("RMC")) {
        uint32_t time_ms = proto.readTimems();
        proto.skip();
        double lat = proto.readLatitude();
        double lon = proto.readLongitude();
        emit positionComplete(0xFFFFFFFF, time_ms, lat, lon);
    }


    QString str_data = QByteArray((char*)proto.frame(),proto.frameLen() - 2);
    core.consoleInfo(QString(">> NMEA: %5").arg(str_data));
}

void SonarDriver::protoComplete(ProtoBinIn &proto) {
    m_state.mark = proto.mark();

    if(hashIDParsing.contains(proto.id())) {
        hashIDParsing[proto.id()]->parse();
    }

    core.consoleProto(proto);

//    else {
//        qInfo("ID is not find: %u", proto.id());
//    }
}

void SonarDriver::startConnection(bool duplex) {
    m_devName = "...";
    m_state.connect = true;
    m_state.uptime = UptimeNone;
    m_state.conf = ConfNone;
    m_state.duplex = duplex;

    idVersion->reset();

    if(m_state.duplex) {
        m_processTimer.start(100);
    }

    m_bootloader = false;
    m_upgrade_status = 0;
}

void SonarDriver::requestDist() {
    idDist->simpleRequest(v0);
}

void SonarDriver::requestChart() {
    idChart->simpleRequest(v0);
}

void SonarDriver::sendUpdateFW(QByteArray update_data) {
    m_bootloader = true;
    idUpdate->setUpdate(update_data);
//    idDataset->resetAll();
//    idDataset->commit();
    reboot();
    QTimer::singleShot(250, idUpdate, SLOT(putUpdate()));
    QTimer::singleShot(400, idUpdate, SLOT(putUpdate()));
}

void SonarDriver::sendFactoryFW(QByteArray update_data) {

}

int SonarDriver::transFreq() {
    return idTransc->freq();
}

void SonarDriver::setTransFreq(int freq) {
    bool is_changed = transFreq() != freq;
    idTransc->setFreq((U2)freq);
    if(is_changed) { emit transChanged(); }
}

int SonarDriver::transPulse() {
    return idTransc->pulse();
}

void SonarDriver::setTransPulse(int pulse) {
    bool is_changed = transPulse() != pulse;
    idTransc->setPulse((U1)pulse);
    if(is_changed) { emit transChanged(); }
}

int SonarDriver::transBoost() {
    return idTransc->boost();
}

void SonarDriver::setTransBoost(int boost) {
    bool is_changed = transBoost() != boost;
    idTransc->setBoost((U1)boost);
    if(is_changed) { emit transChanged(); }
}

int SonarDriver::soundSpeed() {
    return idSoundSpeed->getSoundSpeed();
}

void SonarDriver::setSoundSpeed(int speed) {
    bool is_changed = transBoost() != speed;
    idSoundSpeed->setSoundSpeed(speed);
    if(is_changed) { emit soundChanged(); }
}

void SonarDriver::flashSettings() {
    idFlash->flashing();
}

void SonarDriver::resetSettings() {
    idFlash->erase();
}

void SonarDriver::reboot() {
    idBoot->reboot();
}

int SonarDriver::distMax() {
    return idDistSetup->max();
}
void SonarDriver::setDistMax(int dist) {
    bool is_changed = dist != distMax();
    idDistSetup->setMax(dist);
    if(is_changed) { emit distSetupChanged(); }
}

int SonarDriver::distDeadZone() {
    return idDistSetup->deadZone();
}

void SonarDriver::setDistDeadZone(int dead_zone) {
    bool is_changed = dead_zone != distDeadZone();
    idDistSetup->setDeadZone(dead_zone);
    if(is_changed) { emit distSetupChanged(); }
}

int SonarDriver::distConfidence() {
    return idDistSetup->confidence();
}

void SonarDriver::setConfidence(int confidence) {
    bool is_changed = confidence != distConfidence();
    idDistSetup->setConfidence(confidence);
    if(is_changed) { emit distSetupChanged(); }
}

int SonarDriver::chartSamples() {
    return idChartSetup->count();
}

void SonarDriver::setChartSamples(int smpls) {
    bool is_changed = smpls != chartSamples();
    idChartSetup->setCount((U2)smpls);
    if(is_changed) { emit chartSetupChanged(); }
}

int SonarDriver::chartResolution() {
    return idChartSetup->resolution();
}

void SonarDriver::setChartResolution(int resol) {
    bool is_changed = resol != chartResolution();
    idChartSetup->setResolution((U2)resol);
    if(is_changed) { emit chartSetupChanged(); }
}

int SonarDriver::chartOffset() {
    return idChartSetup->offset();
}

void SonarDriver::setChartOffset(int offset) {
    bool is_changed = offset != chartOffset();
    idChartSetup->setOffset((U2)offset);
    if(is_changed) { emit chartSetupChanged(); }
}

int SonarDriver::datasetDist() {
    int ch_param = 0;
    if(idDataset->getDist_v0(1)) {
        ch_param |= 1;
    }
    if(idDataset->getDist_v0(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void SonarDriver::setDatasetDist(int ch_param) {
    bool is_changed = (ch_param != datasetDist());
    idDataset->setDist_v0(ch_param);
    idDataset->commit();
    if(is_changed) {
        emit datasetChanged();
    }
}

int SonarDriver::datasetChart() {
    int ch_param = 0;
    if(idDataset->getChart_v0(1)) {
        ch_param |= 1;
    }
    if(idDataset->getChart_v0(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void SonarDriver::setDatasetChart(int ch_param) {
    bool is_changed = (ch_param != datasetChart());
    idDataset->setChart_v0(ch_param);
    idDataset->commit();
    if(is_changed) {
        emit datasetChanged();
    }
}

int SonarDriver::datasetTemp() {
    int ch_param = 0;
    if(idDataset->getTemp_v0(1)) {
        ch_param |= 1;
    }
    if(idDataset->getTemp_v0(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void SonarDriver::setDatasetTemp(int ch_param) {
    bool is_changed = (ch_param != datasetTemp());
    idDataset->setTemp_v0(ch_param);
    idDataset->commit();
    if(is_changed) {
        emit datasetChanged();
    }
}

int SonarDriver::datasetSDDBT() {
    int ch_param = 0;
    if(idDataset->getSDDBT(1)) {
        ch_param |= 1;
    }
    if(idDataset->getSDDBT(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void SonarDriver::setDatasetSDDBT(int ch_param) {
    bool is_changed = (ch_param != datasetSDDBT());
    idDataset->setSDDBT(ch_param);
    idDataset->commit();
    if(is_changed) {
        emit datasetChanged();
    }
}

int SonarDriver::datasetSDDBT_P2() {
    int ch_param = 0;
    if(idDataset->getSDDBT_P2(1)) {
        ch_param |= 1;
    }
    if(idDataset->getSDDBT_P2(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void SonarDriver::setDatasetSDDBT_P2(int ch_param) {
    bool is_changed = (ch_param != datasetSDDBT_P2());
    idDataset->setSDDBT_P2(ch_param);
    idDataset->commit();
    if(is_changed) {
        emit datasetChanged();
    }
}

int SonarDriver::ch1Period() {
    return (int)idDataset->period(1);
}

void SonarDriver::setCh1Period(int period) {
    bool is_changed = (period != ch1Period());
    idDataset->setPeriod(1, (uint)period);
    if(is_changed) {
        emit datasetChanged();
    }
}

int SonarDriver::ch2Period() {
    return (int)idDataset->period(2);
}

void SonarDriver::setCh2Period(int period) {
    bool is_changed = (period != ch2Period());
    idDataset->setPeriod(2, (uint)period);
    if(is_changed) {
        emit datasetChanged();
    }
}

void SonarDriver::receivedTimestamp(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
//    qInfo("Timestamp: %u", idTimestamp->timestamp());
}

void SonarDriver::receivedDist(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)

//    qInfo("Dist %u", idDist->dist_mm());
    emit distComplete(idDist->dist_mm());
}

void SonarDriver::receivedChart(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
    if(idChart->isCompleteChart()) {
        QVector<int16_t> data(idChart->chartSize());
        uint8_t* raw_data = idChart->rawData();
        for(int i = 0; i < data.length(); i++) {
            data[i] = raw_data[i];
        }
        emit chartComplete(data, idChart->resolution(), idChart->offsetRange());
    }
}

void SonarDriver::receivedAtt(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}

void SonarDriver::receivedTemp(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}

void SonarDriver::receivedDataset(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)

    if(resp == respNone) {
        qInfo("Data set update");
        emit datasetChanged();
    } else {
//        qInfo("Data set resp %u", resp);
        qInfo("Data set update %u", resp);
    }
}

void SonarDriver::receivedDistSetup(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
    qInfo("Dist resp %u", resp);
    if(resp == respNone) {
        emit distSetupChanged();
    }
}

void SonarDriver::receivedChartSetup(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)

    if(resp == respNone) {
        emit chartSetupChanged();
    }
}

void SonarDriver::receivedTransc(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
    if(resp == respNone) {
        emit transChanged();
    }
}

void SonarDriver::receivedSoundSpeed(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
    if(resp == respNone) {
        emit soundChanged();
    }
}

void SonarDriver::receivedUART(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
    qInfo("UART ver %u, resp %u", ver, resp);
    if(resp == respNone) {
        emit UARTChanged();
    }
}

void SonarDriver::receivedVersion(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
    if(resp == respNone) {
        switch (idVersion->boardVersion()) {
        case IDBinVersion::BoardEnhanced:
            m_devName = "2D-ENHANCED";
            break;
        case IDBinVersion::BoardChirp:
            m_devName = "2D-CHIRP";
            break;
        case IDBinVersion::BoardBase:
            m_devName = "2D-BASE";
            break;
        case IDBinVersion::BoardNBase:
            m_devName = "2D-NBASE";
            break;
        default:
            m_devName = "None";
        }
        emit deviceVersionChanged();
    }
//

}

void SonarDriver::receivedMark(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}
void SonarDriver::receivedFlash(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}
void SonarDriver::receivedBoot(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}
void SonarDriver::receivedUpdate(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)

//    qInfo("Upgrade resp %u", resp);
    if(resp == respOk) {
        bool is_avail_data = idUpdate->putUpdate();
        m_upgrade_status = idUpdate->progress();
        if(!is_avail_data) {
            idBoot->runFW();
            m_bootloader = false;
            m_upgrade_status = 0;
        }

    } else {
        m_upgrade_status = -1;
    }

    emit upgradeProgressChanged();
}

void SonarDriver::receivedNav(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}

void SonarDriver::process() {
    if(m_state.duplex && isUpdatingFw() == false) {
        if(!m_state.mark) {
            m_state.uptime = UptimeNone;
        }
//        qInfo("mark %u, state %u", m_state.mark, m_state.uptime);

        if(m_state.uptime == UptimeNone) {
            idMark->setMark();
            idVersion->requestAll();
            m_state.uptime = UptimeRequest;
        } else if(m_state.uptime == UptimeRequest) {
            requestSetup();
            m_state.uptime = UptimeFix;
        } else if(m_state.uptime == UptimeFix) {

        }
    }
}
