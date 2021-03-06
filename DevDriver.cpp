#include "DevDriver.h"
#include <time.h>
#include <core.h>
extern Core core;

DevDriver::DevDriver(QObject *parent) :
    QObject(parent)
{
    regID(idTimestamp = new IDBinTimestamp(), &DevDriver::receivedTimestamp);

    regID(idDist = new IDBinDist(), &DevDriver::receivedDist);
    regID(idChart = new IDBinChart(), &DevDriver::receivedChart);
    regID(idAtt = new IDBinAttitude(), &DevDriver::receivedAtt);
    regID(idTemp = new IDBinTemp(), &DevDriver::receivedTemp);

    regID(idDataset = new IDBinDataset(), &DevDriver::receivedDataset, true);
    regID(idDistSetup = new IDBinDistSetup(), &DevDriver::receivedDistSetup, true);
    regID(idChartSetup = new IDBinChartSetup(), &DevDriver::receivedChartSetup, true);
    regID(idDSPSetup = new IDBinDSPSetup(), &DevDriver::receivedDSPSetup, true);
    regID(idTransc = new IDBinTransc(), &DevDriver::receivedTransc, true);
    regID(idSoundSpeed = new IDBinSoundSpeed(), &DevDriver::receivedSoundSpeed, true);
    regID(idUART = new IDBinUART(), &DevDriver::receivedUART, true);
    regID(idVersion = new IDBinVersion(), &DevDriver::receivedVersion, true);

    regID(idMark = new IDBinMark(), &DevDriver::receivedMark);
    regID(idFlash = new IDBinFlash(), &DevDriver::receivedFlash);
    regID(idBoot = new IDBinBoot(), &DevDriver::receivedBoot);
    regID(idUpdate = new IDBinUpdate(), &DevDriver::receivedUpdate);

    regID(idNav = new IDBinNav(), &DevDriver::receivedNav);

    connect(&m_processTimer, &QTimer::timeout, this, &DevDriver::process);
}

void DevDriver::regID(IDBin* id_bin, ParseCallback method, bool is_setup) {
    hashIDParsing[id_bin->id()] = id_bin;
    hashIDCallback[id_bin->id()] = method;

    if(is_setup) {
        hashIDSetup[id_bin->id()] = id_bin;
    }

    connect(id_bin, &IDBin::binFrameOut, this, &DevDriver::binFrameOut);
}


void DevDriver::requestSetup() {
    QHashIterator<ID, IDBin*> i(hashIDSetup);
    while (i.hasNext()) {
        i.next();
        i.value()->requestAll();
    }

    m_state.conf = ConfRequest;
}

void DevDriver::setConsoleOut(bool is_console) {
    QHashIterator<ID, IDBin*> i(hashIDParsing);
    while (i.hasNext()) {
        i.next();
        i.value()->setConsoleOut(is_console);
    }
    m_isConsole = is_console;
}

void DevDriver::setBusAddress(int addr) {
    m_busAddress = addr;
    QHashIterator<ID, IDBin*> i(hashIDParsing);
    while (i.hasNext()) {
        i.next();
        i.value()->setAddress(m_busAddress);
    }
}

int DevDriver::getBusAddress() {
    return m_busAddress;
}

void DevDriver::setDevAddress(int addr) {
    idUART->setDevAddress(addr);
}

int DevDriver::getDevAddress() {
    return idUART->devAddress();
}

void DevDriver::setBaudrate(int baudrate) {
    if(!m_state.connect) return;
    bool is_changed = getBaudrate() != baudrate;
    idUART->setBaudrate(baudrate);
    if(is_changed) { emit transChanged(); }
}

int DevDriver::getBaudrate() {
    return idUART->getBaudrate();
}

void DevDriver::setDevDefAddress(int addr) {
     idUART->setDevDefAddress(addr);
}

int DevDriver::getDevDefAddress() {
    return idUART->devDefAddress();
}

uint32_t DevDriver::devSerialNumber() {
    return idVersion->serialNumber();
}

QString DevDriver::devPN() {

}

void DevDriver::nmeaComplete(ProtoNMEA &proto) {
    if(proto.isEqualId("RMC")) {
        uint32_t time_ms = proto.readTimems();
        proto.skip();
        double lat = proto.readLatitude();
        double lon = proto.readLongitude();
        emit positionComplete(0xFFFFFFFF, time_ms, lat, lon);
    }
}

void DevDriver::protoComplete(ProtoBinIn &proto) {
    m_state.mark = proto.mark();

    if(hashIDParsing.contains(proto.id())) {
        IDBin* parse_instance = hashIDParsing[proto.id()];
        ParseCallback callback = hashIDCallback[proto.id()];
        parse_instance->parse(proto);
        (this->*callback)(parse_instance->lastType(), parse_instance->lastVersion(), parse_instance->lastResp());
    }
}

void DevDriver::startConnection(bool duplex) {
    m_devName = "...";
    m_state.duplex = duplex;
    idVersion->reset();

    m_bootloader = false;
    m_upgrade_status = 0;

    restartState();

    emit deviceVersionChanged();
}

void DevDriver::stopConnection() {
    m_state.connect = false;
    m_processTimer.stop();
    m_devName = "...";
    emit deviceVersionChanged();
}

void DevDriver::restartState() {
    m_processTimer.stop();
    m_state.connect = false;
    m_state.uptime = UptimeNone;
    m_state.conf = ConfNone;
    if(m_state.duplex) {
        m_processTimer.start(200);
    }
}

void DevDriver::requestDist() {
    if(!m_state.connect) return;
    idDist->simpleRequest(v0);
}

void DevDriver::requestChart() {
    if(!m_state.connect) return;
    idChart->simpleRequest(v0);
}

void DevDriver::sendUpdateFW(QByteArray update_data) {
    if(!m_state.connect) return;
    m_bootloader = true;
    idUpdate->setUpdate(update_data);
    reboot();
    restartState();
    QTimer::singleShot(300, idUpdate, SLOT(putUpdate()));
    QTimer::singleShot(400, idUpdate, SLOT(putUpdate()));
}

void DevDriver::sendFactoryFW(QByteArray update_data) {

}

int DevDriver::transFreq() {
    return idTransc->freq();
}

void DevDriver::setTransFreq(int freq) {
    if(!m_state.connect) return;
    bool is_changed = transFreq() != freq;
    idTransc->setFreq((U2)freq);
    if(is_changed) { emit transChanged(); }
}

int DevDriver::transPulse() {
    return idTransc->pulse();
}

void DevDriver::setTransPulse(int pulse) {
    if(!m_state.connect) return;
    bool is_changed = transPulse() != pulse;
    idTransc->setPulse((U1)pulse);
    if(is_changed) { emit transChanged(); }
}

int DevDriver::transBoost() {
    return idTransc->boost();
}

void DevDriver::setTransBoost(int boost) {
    if(!m_state.connect) return;
    bool is_changed = transBoost() != boost;
    idTransc->setBoost((U1)boost);
    if(is_changed) { emit transChanged(); }
}

int DevDriver::soundSpeed() {
    return idSoundSpeed->getSoundSpeed();
}

void DevDriver::setSoundSpeed(int speed) {
    if(!m_state.connect) return;
    bool is_changed = transBoost() != speed;
    idSoundSpeed->setSoundSpeed(speed);
    if(is_changed) { emit soundChanged(); }
}

void DevDriver::flashSettings() {
    if(!m_state.connect) return;
    idFlash->flashing();
}

void DevDriver::resetSettings() {
    if(!m_state.connect) return;
    idFlash->erase();
}

void DevDriver::reboot() {
    if(!m_state.connect) return;
    idBoot->reboot();
    emit onReboot();
}

int DevDriver::distMax() {
    return idDistSetup->max();
}
void DevDriver::setDistMax(int dist) {
    if(!m_state.connect) return;
    bool is_changed = dist != distMax();
    idDistSetup->setMax(dist);
    if(is_changed) { emit distSetupChanged(); }
}

int DevDriver::distDeadZone() {
    return idDistSetup->deadZone();
}

void DevDriver::setDistDeadZone(int dead_zone) {
    if(!m_state.connect) return;
    bool is_changed = dead_zone != distDeadZone();
    idDistSetup->setDeadZone(dead_zone);
    if(is_changed) { emit distSetupChanged(); }
}

int DevDriver::distConfidence() {
    return idDistSetup->confidence();
}

void DevDriver::setConfidence(int confidence) {
    if(!m_state.connect) return;
    bool is_changed = confidence != distConfidence();
    idDistSetup->setConfidence(confidence);
    if(is_changed) { emit distSetupChanged(); }
}

int DevDriver::chartSamples() {
    return idChartSetup->count();
}

void DevDriver::setChartSamples(int smpls) {
    if(!m_state.connect) return;
    bool is_changed = smpls != chartSamples();
    idChartSetup->setCount((U2)smpls);
    if(is_changed) { emit chartSetupChanged(); }
}

int DevDriver::chartResolution() {
    return idChartSetup->resolution();
}

void DevDriver::setChartResolution(int resol) {
    if(!m_state.connect) return;
    bool is_changed = resol != chartResolution();
    idChartSetup->setResolution((U2)resol);
    if(is_changed) { emit chartSetupChanged(); }
}

int DevDriver::chartOffset() {
    return idChartSetup->offset();
}

void DevDriver::setChartOffset(int offset) {
    if(!m_state.connect) return;
    bool is_changed = offset != chartOffset();
    idChartSetup->setOffset((U2)offset);
    if(is_changed) { emit chartSetupChanged(); }
}

int DevDriver::dspSmoothFactor() {
    return idDSPSetup->horSmoothFactor();
}

void DevDriver::setDspSmoothFactor(int dsp_smooth) {
    if(!m_state.connect) return;
    bool is_changed = dsp_smooth != dspSmoothFactor();
    idDSPSetup->setHorSmoothFactor((U1)dsp_smooth);
    if(is_changed) { emit dspSetupChanged(); }
}

int DevDriver::datasetTimestamp() {
    int ch_param = 0;
    if(idDataset->getTimestamp_v0(1)) {
        ch_param |= 1;
    }
    if(idDataset->getTimestamp_v0(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void DevDriver::setDatasetTimestamp(int ch_param) {
    if(!m_state.connect) return;
    bool is_changed = (ch_param != datasetTimestamp());
    idDataset->setTimestamp_v0(ch_param);
    idDataset->commit();
    if(is_changed) { emit datasetChanged(); }
}

int DevDriver::datasetDist() {
    int ch_param = 0;
    if(idDataset->getDist_v0(1)) {
        ch_param |= 1;
    }
    if(idDataset->getDist_v0(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void DevDriver::setDatasetDist(int ch_param) {
    if(!m_state.connect) return;
    bool is_changed = (ch_param != datasetDist());
    idDataset->setDist_v0(ch_param);
    idDataset->commit();
    if(is_changed) { emit datasetChanged(); }
}

int DevDriver::datasetChart() {
    int ch_param = 0;
    if(idDataset->getChart_v0(1)) {
        ch_param |= 1;
    }
    if(idDataset->getChart_v0(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void DevDriver::setDatasetChart(int ch_param) {
    if(!m_state.connect) return;
    bool is_changed = (ch_param != datasetChart());
    idDataset->setChart_v0(ch_param);
    idDataset->commit();
    if(is_changed) { emit datasetChanged(); }
}

int DevDriver::datasetTemp() {
    int ch_param = 0;
    if(idDataset->getTemp_v0(1)) {
        ch_param |= 1;
    }
    if(idDataset->getTemp_v0(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void DevDriver::setDatasetTemp(int ch_param) {
    if(!m_state.connect) return;
    bool is_changed = (ch_param != datasetTemp());
    idDataset->setTemp_v0(ch_param);
    idDataset->commit();
    if(is_changed) { emit datasetChanged();  }
}

int DevDriver::datasetSDDBT() {
    int ch_param = 0;
    if(idDataset->getSDDBT(1)) {
        ch_param |= 1;
    }
    if(idDataset->getSDDBT(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void DevDriver::setDatasetSDDBT(int ch_param) {
    if(!m_state.connect) return;
    bool is_changed = (ch_param != datasetSDDBT());
    idDataset->setSDDBT(ch_param);
    idDataset->commit();
    if(is_changed) {  emit datasetChanged(); }
}

int DevDriver::datasetSDDBT_P2() {
    int ch_param = 0;
    if(idDataset->getSDDBT_P2(1)) {
        ch_param |= 1;
    }
    if(idDataset->getSDDBT_P2(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void DevDriver::setDatasetSDDBT_P2(int ch_param) {
    if(!m_state.connect) return;
    bool is_changed = (ch_param != datasetSDDBT_P2());
    idDataset->setSDDBT_P2(ch_param);
    idDataset->commit();
    if(is_changed) {  emit datasetChanged(); }
}

int DevDriver::ch1Period() {
    return (int)idDataset->period(1);
}

void DevDriver::setCh1Period(int period) {
    if(!m_state.connect) return;
    bool is_changed = (period != ch1Period());
    idDataset->setPeriod(1, (uint)period);
    if(is_changed) {  emit datasetChanged();  }
}

int DevDriver::ch2Period() {
    return (int)idDataset->period(2);
}

void DevDriver::setCh2Period(int period) {
    if(!m_state.connect) return;
    bool is_changed = (period != ch2Period());
    idDataset->setPeriod(2, (uint)period);
    if(is_changed) {  emit datasetChanged();  }
}

void DevDriver::receivedTimestamp(Type type, Version ver, Resp resp) {
}

void DevDriver::receivedDist(Type type, Version ver, Resp resp) {
    emit distComplete(idDist->dist_mm());
}

void DevDriver::receivedChart(Type type, Version ver, Resp resp) {
    if(idChart->isCompleteChart()) {
        QVector<int16_t> data(idChart->chartSize());
        uint8_t* raw_data = idChart->rawData();
        for(int i = 0; i < data.length(); i++) {
            data[i] = raw_data[i];
        }
        emit chartComplete(data, idChart->resolution(), idChart->offsetRange());
    }
}

void DevDriver::receivedAtt(Type type, Version ver, Resp resp) {
}

void DevDriver::receivedTemp(Type type, Version ver, Resp resp) {
}

void DevDriver::receivedDataset(Type type, Version ver, Resp resp) {
    if(resp == respNone) { emit datasetChanged(); } else {  }
}

void DevDriver::receivedDistSetup(Type type, Version ver, Resp resp) {
    if(resp == respNone) {   emit distSetupChanged();  }
}

void DevDriver::receivedChartSetup(Type type, Version ver, Resp resp) {
    if(resp == respNone) {  emit chartSetupChanged();  }
}

void DevDriver::receivedDSPSetup(Type type, Version ver, Resp resp) {
    if(resp == respNone) {  emit dspSetupChanged();  }
}

void DevDriver::receivedTransc(Type type, Version ver, Resp resp) {
    if(resp == respNone) {  emit transChanged();  }
}

void DevDriver::receivedSoundSpeed(Type type, Version ver, Resp resp) {
    if(resp == respNone) {  emit soundChanged();  }
}

void DevDriver::receivedUART(Type type, Version ver, Resp resp) {
    if(resp == respNone) { emit UARTChanged(); }
}

void DevDriver::receivedVersion(Type type, Version ver, Resp resp) {
    if(resp == respNone) {

        switch (idVersion->boardVersion()) {
        case BoardNone:
            if(idVersion->boardVersionMinor() == BoardAssist) {
                m_devName = "Assist";
            } else {
                m_devName = QString("Device ID: %1.%2").arg(idVersion->boardVersion()).arg(idVersion->boardVersionMinor());
            }
            break;
        case BoardEnhanced:
            m_devName = "Sonar Enhanced";
            break;
        case BoardChirp:
            m_devName = "Sonar Chirp";
            break;
        case BoardBase:
            m_devName = "Sonar Base";
            break;
        case BoardNBase:
            m_devName = "Sonar Base2";
            break;
        case BoardAssist:
            m_devName = "Assist";
        case BoardNEnhanced:
            m_devName = "Sonar Enhanced2";
            break;
        default:
            m_devName = QString("Device ID: %1.%2").arg(idVersion->boardVersion()).arg(idVersion->boardVersionMinor());
        }
        emit deviceVersionChanged();
    }
}

void DevDriver::receivedMark(Type type, Version ver, Resp resp) {
}

void DevDriver::receivedFlash(Type type, Version ver, Resp resp) {
}

void DevDriver::receivedBoot(Type type, Version ver, Resp resp) {
}

void DevDriver::receivedUpdate(Type type, Version ver, Resp resp) {
    if(resp == respOk) {
        if(m_bootloader) {
            bool is_avail_data = idUpdate->putUpdate();
            m_upgrade_status = idUpdate->progress();
            if(!is_avail_data) {
                idBoot->runFW();
                m_bootloader = false;
                m_upgrade_status = successUpgrade;
                restartState();
            }
        }
    } else {
        if(m_bootloader) {
            m_upgrade_status = failUpgrade;
            m_bootloader = false;
            restartState();
        }
    }


    emit upgradeProgressChanged(m_upgrade_status);
    emit upgradeChanged();
}

void DevDriver::receivedNav(Type type, Version ver, Resp resp) {
}

void DevDriver::process() {
    if(m_state.duplex) {
        if(!m_state.mark) {
            m_state.uptime = UptimeNone;
        }

        if(m_state.uptime == UptimeNone) {
            idMark->setMark();
            idVersion->requestAll();
            m_state.uptime = UptimeRequest;
        } else if(m_state.uptime == UptimeRequest) {
            if(!m_bootloader) {
                requestSetup();
            }

            m_state.uptime = UptimeFix;
        } else if(m_state.uptime == UptimeFix) {
            if(m_state.mark) {
                m_state.connect = true;
            }
        }
    }
}
