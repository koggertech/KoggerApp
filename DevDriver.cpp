#include "DevDriver.h"
#include <time.h>
#include <core.h>
#include <QXmlStreamWriter>

extern Core core;

DevDriver::DevDriver(QObject *parent) :
    QObject(parent),
    datasetState_(false),
    distSetupState_(false),
    chartSetupState_(false),
    dspSetupState_(false),
    transcState_(false),
    soundSpeedState_(false),
    uartState_(false)
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
    regID(idDVL = new IDBinDVL(), &DevDriver::receivedDVL);
    regID(idDVLMode = new IDBinDVLMode(), &DevDriver::receivedDVLMode);

    connect(&m_processTimer, &QTimer::timeout, this, &DevDriver::process);

    QObject::connect(idDataset, &IDBin::notifyDevDriver, this, &DevDriver::setDatasetState);
    QObject::connect(idDistSetup, &IDBin::notifyDevDriver, this, &DevDriver::setDistSetupState);
    QObject::connect(idChartSetup, &IDBin::notifyDevDriver, this, &DevDriver::setChartSetupState);
    QObject::connect(idDSPSetup, &IDBin::notifyDevDriver, this, &DevDriver::setDspSetupState);
    QObject::connect(idTransc, &IDBin::notifyDevDriver, this, &DevDriver::setTranscState);
    QObject::connect(idSoundSpeed, &IDBin::notifyDevDriver, this, &DevDriver::setSoundSpeedState);
    QObject::connect(idUART, &IDBin::notifyDevDriver, this, &DevDriver::setUartState);
}

void DevDriver::regID(IDBin* id_bin, ParseCallback method, bool is_setup) {
//    hashIDParsing[id_bin->id()] = id_bin;
//    hashIDCallback[id_bin->id()] = method;

    _hashID[id_bin->id()] = ID_Instance(id_bin, method, is_setup);


//    if(is_setup) {
//        hashIDSetup[id_bin->id()] = id_bin;
//    }

    connect(id_bin, &IDBin::binFrameOut, this, &DevDriver::binFrameOut);
}


void DevDriver::requestSetup() {
    QHashIterator<ID, ID_Instance> i(_hashID);
    while (i.hasNext()) {
        i.next();
        if(i.value().isSetup) {
            i.value().instance->startColdStartTimer();
            i.value().instance->requestAll();
        }
    }

    m_state.conf = ConfRequest;
}



void DevDriver::setConsoleOut(bool is_console) {
    QHashIterator<ID, ID_Instance> i(_hashID);
    while (i.hasNext()) {
        i.next();
        i.value().instance->setConsoleOut(is_console);
    }
    m_isConsole = is_console;
}

void DevDriver::setBusAddress(int addr) {
    m_busAddress = addr;
    QHashIterator<ID, ID_Instance> i(_hashID);
    while (i.hasNext()) {
        i.next();
        i.value().instance->setAddress(m_busAddress);
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
    if(is_changed) { emit UARTChanged(); }
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

int DevDriver::dopplerVeloX() {
    return idDVL->velX();
}

int DevDriver::dopplerVeloY() {
    return idDVL->velY();
}

int DevDriver::dopplerVeloZ() {
    return idDVL->velZ();
}

int DevDriver::dopplerDist() {
    return idDVL->dist();
}

void DevDriver::dvlChangeMode(bool ismode1, bool ismode2, bool ismode3, bool ismode4, float range_mode4) {
    idDVLMode->setModes(ismode1, ismode2, ismode3, ismode4, range_mode4);
}

void DevDriver::importSettingsFromXML(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QXmlStreamReader xmlReader(&file);
    while (!xmlReader.atEnd() && !xmlReader.hasError()) {
        const QXmlStreamReader::TokenType token = xmlReader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            const QString elementName = xmlReader.name().toString();
            if (elementName == "Settings")
                continue;

            while (!(xmlReader.tokenType() == QXmlStreamReader::EndElement && xmlReader.name() == elementName)) {
                xmlReader.readNext();
                if (xmlReader.isStartElement()) {
                    if (elementName == "Echogram") {
                        if (xmlReader.name().toString() == "resolution_mm")
                            setChartResolution(xmlReader.readElementText().toInt());
                        else if (xmlReader.name().toString() == "number_of_samples")
                            setChartSamples(xmlReader.readElementText().toInt());
                        else if (xmlReader.name().toString() == "offset_of_samples")
                            setChartOffset(xmlReader.readElementText().toInt());
                    }
                    else if (elementName == "Rangefinder") {
                        if (xmlReader.name().toString() == "max_distance_mm")
                            setDistMax(xmlReader.readElementText().toInt());
                        else if (xmlReader.name().toString() == "dead_zone_mm")
                            setDistDeadZone(xmlReader.readElementText().toInt());
                        else if (xmlReader.name().toString() == "confidence_threshold_perc")
                            setConfidence(xmlReader.readElementText().toInt());
                    }
                    else if (elementName == "Transducer") {
                        if (xmlReader.name().toString() == "pulse_count")
                            setTransPulse(xmlReader.readElementText().toInt());
                        else if (xmlReader.name().toString() == "frequency_khz")
                            setTransFreq(xmlReader.readElementText().toInt());
                        else if (xmlReader.name().toString() == "booster") {
                            const QString state = xmlReader.readElementText().trimmed();
                            setTransBoost(state.toUpper() == "TRUE" ? 1 : 0);
                        }
                    }
                    else if (elementName == "DSP") {
                        if (xmlReader.name().toString() == "horizontal_smoothing_factor")
                            setDspSmoothFactor(xmlReader.readElementText().toInt());
                        else if (xmlReader.name().toString() == "speed_of_sound_m_s")
                            setSoundSpeed(xmlReader.readElementText().toInt());
                    }
                    else if (elementName == "Dataset") {
                        auto idString = xmlReader.name().toString();
                        const auto id = static_cast<U1>(idString.remove(0, 6).toInt());
                        while (xmlReader.readNext() != QXmlStreamReader::EndElement) {
                            if (xmlReader.tokenType() == QXmlStreamReader::StartElement) {
                                const QString nameStr = xmlReader.name().toString();
                                const QString valStr = xmlReader.readElementText().trimmed();
                                if (nameStr == "period_ms")
                                    id == 1 ? setCh1Period(valStr.toUInt()) : setCh2Period(valStr.toUInt());
                                else if (nameStr == "echogram")
                                    setDatasetChart(id, valStr.toUpper() == "TRUE" ? 1 : 0);
                                else if (nameStr == "rangefinder")
                                    setDatasetDist(id, valStr.toUpper() == "TRUE" ? 1 : 0);
                                else if (nameStr == "AHRS")
                                    setDatasetEuler(id, valStr.toUpper() == "TRUE" ? 1 : 0);
                                else if (nameStr == "temperature")
                                    setDatasetTemp(id, valStr.toUpper() == "TRUE" ? 1 : 0);
                                else if (nameStr == "timestamp")
                                    setDatasetTimestamp(id, valStr.toUpper() == "TRUE" ? 1 : 0);
                            }
                        }
                    }
                    else if (elementName == "UART") {
                        if (xmlReader.name().toString() == "dev_def_address")
                            setDevDefAddress(xmlReader.readElementText().toInt());
                        else if (xmlReader.name().toString() == "baudrate")
                            setBaudrate(xmlReader.readElementText().toInt());
                        else if (xmlReader.name().toString() == "dev_address")
                            setDevAddress(xmlReader.readElementText().toInt());
                    }
                }
            }
        }
    }

    if (xmlReader.hasError())
        qDebug() << "XML error:" << xmlReader.errorString().toUtf8();

    file.close();
}

void DevDriver::exportSettingsToXML(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("Settings");

    xmlWriter.writeStartElement("Echogram");
    xmlWriter.writeTextElement("resolution_mm", QString::number(chartResolution()));
    xmlWriter.writeTextElement("number_of_samples", QString::number(chartSamples()));
    xmlWriter.writeTextElement("offset_of_samples", QString::number(chartOffset()));
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("Rangefinder");
    xmlWriter.writeTextElement("max_distance_mm", QString::number(distMax()));
    xmlWriter.writeTextElement("dead_zone_mm", QString::number(distDeadZone()));
    xmlWriter.writeTextElement("confidence_threshold_perc", QString::number(distConfidence()));
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("Transducer");
    xmlWriter.writeTextElement("pulse_count", QString::number(transPulse()));
    xmlWriter.writeTextElement("frequency_khz", QString::number(transFreq()));
    xmlWriter.writeTextElement("booster", QVariant(static_cast<bool>(transBoost())).toString());
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("DSP");
    xmlWriter.writeTextElement("horizontal_smoothing_factor", QString::number(dspSmoothFactor()));
    xmlWriter.writeTextElement("speed_of_sound_m_s", QString::number(soundSpeed()));
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("Dataset");
    const auto channels = idDataset->getChannels();
    for (uint8_t i = 1; i < channels.count(); ++i) {
        xmlWriter.writeStartElement("group_" + QString::number(channels.at(i).id));
        xmlWriter.writeTextElement("period_ms", QString::number(channels.at(i).period));
        xmlWriter.writeTextElement("echogram", QVariant(idDataset->getChart_v0(i)).toString());
        xmlWriter.writeTextElement("rangefinder", QVariant(idDataset->getDist_v0(i)).toString());
        xmlWriter.writeTextElement("AHRS", QVariant(static_cast<bool>(idDataset->getEuler(i))).toString());
        xmlWriter.writeTextElement("temperature", QVariant(idDataset->getTemp_v0(i)).toString());
        xmlWriter.writeTextElement("timestamp", QVariant(idDataset->getTimestamp_v0(i)).toString());
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("UART");
    xmlWriter.writeTextElement("dev_def_address", QString::number(getDevDefAddress()));
    xmlWriter.writeTextElement("baudrate", QString::number(getBaudrate()));
    xmlWriter.writeTextElement("dev_address", QString::number(getDevAddress()));
    xmlWriter.writeEndElement();

    xmlWriter.writeEndElement(); // Settings
    xmlWriter.writeEndDocument();
    file.close();

    return;
}

void DevDriver::setDatasetState(bool state) {
    if (state != datasetState_) {
        datasetState_ = state;
        emit datasetChanged();
    }
}

void DevDriver::setDistSetupState(bool state) {
    if (state != distSetupState_) {
        distSetupState_ = state;
        emit distSetupChanged();
    }
}

void DevDriver::setChartSetupState(bool state) {
    if (state != chartSetupState_) {
        chartSetupState_ = state;
        emit chartSetupChanged();
    }
}

void DevDriver::setDspSetupState(bool state) {
    if (state != dspSetupState_) {
        dspSetupState_ = state;
        emit dspSetupChanged();
    }
}

void DevDriver::setTranscState(bool state) {
    if (state != transcState_) {
        transcState_ = state;
        emit transChanged();
    }
}

void DevDriver::setSoundSpeedState(bool state) {
    if (state != soundSpeedState_) {
        soundSpeedState_ = state;
        emit soundChanged();
    }
}

void DevDriver::setUartState(bool state) {
    if (state != uartState_) {
        uartState_ = state;
        emit UARTChanged();
    }
}

uint32_t DevDriver::devSerialNumber() {
    return idVersion->serialNumber();
}

QString DevDriver::devPN() {
    return QString();
}

void DevDriver::protoComplete(FrameParser &proto) {
    if(!proto.isComplete()) return;

    m_state.mark = proto.mark();

    if(_hashID.contains(proto.id())) {
        if(_hashID[proto.id()].instance != NULL) {
            IDBin* parse_instance = _hashID[proto.id()].instance;
            parse_instance->parse(proto);
            _lastAddres = proto.route();

            if(_hashID[proto.id()].callback != NULL) {
                ParseCallback callback = _hashID[proto.id()].callback;
                (this->*callback)(parse_instance->lastType(), parse_instance->lastVersion(), parse_instance->lastResp());
            }
        }
    }
}

void DevDriver::startConnection(bool duplex) {
    m_devName = "...";
    m_state.duplex = duplex;
    idVersion->reset();

    m_bootloaderLagacyMode = true;
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

void DevDriver::requestStreamList() {
    ProtoBinOut id_out;
    id_out.create(GETTING, v0, ID_STREAM, getDevAddress());
    id_out.write<U2>(0);
    id_out.write<U2>(0xFFFF);
    id_out.write<U4>(0x0);
    id_out.end();
    emit binFrameOut(id_out);
}

void DevDriver::requestStream(int stream_id) {
    ProtoBinOut id_out;
    id_out.create(SETTING, v1, ID_STREAM, getDevAddress());
    id_out.write<U2>(stream_id);
    id_out.write<U2>(0); // FLAGS
    id_out.write<U4>(0x0);
    id_out.write<U4>(0xFFFFFFF);
    id_out.end();
    emit binFrameOut(id_out);
}

void DevDriver::sendUpdateFW(QByteArray update_data) {
//    if(!m_state.connect) return;
    m_bootloaderLagacyMode = true;
    m_bootloader = true;
    _timeoutUpgradeAnswerTime = 5000;
    idUpdate->setUpdate(update_data);
    reboot();
    restartState();
    QTimer::singleShot(500, idUpdate, SLOT(putUpdate()));
//    QTimer::singleShot(400, idUpdate, SLOT(putUpdate()));
}

void DevDriver::sendFactoryFW(QByteArray update_data) {
    Q_UNUSED(update_data);
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

float DevDriver::yaw() {
    return idAtt->yaw();
}

float DevDriver::pitch() {
    return idAtt->pitch();
}

float DevDriver::roll() {
    return idAtt->roll();
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

void DevDriver::setDatasetTimestamp(U1 channel_id, bool state) {
    bool is_on = idDataset->getTimestamp_v0(channel_id);
    if (is_on != state) {
        state ? idDataset->setTimestamp_v0(channel_id) : idDataset->resetTimestamp_v0(channel_id);
        emit datasetChanged();
    }
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

void DevDriver::setDatasetDist(U1 channel_id, bool state) {
    bool is_on = idDataset->getDist_v0(channel_id);
    if (is_on != state) {
        state ? idDataset->setDist_v0(channel_id) : idDataset->resetDist_v0(channel_id);
        emit datasetChanged();
    }
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

void DevDriver::setDatasetChart(U1 channel_id, bool state) {
    bool is_on = idDataset->getChart_v0(channel_id);
    if (is_on != state) {
        state ? idDataset->setChart_v0(channel_id) : idDataset->resetChart_v0(channel_id);
        emit datasetChanged();
    }
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

void DevDriver::setDatasetTemp(U1 channel_id, bool state) {
    bool is_on = idDataset->getTemp_v0(channel_id);
    if (is_on != state) {
        state ? idDataset->setTemp_v0(channel_id) : idDataset->resetTemp_v0(channel_id);
        emit datasetChanged();
    }
}

int DevDriver::datasetEuler() {
    int ch_param = 0;
    if(idDataset->getEuler(1)) {
        ch_param |= 1;
    }
    if(idDataset->getEuler(2)) {
        ch_param |= 2;
    }
    return ch_param;
}

void DevDriver::setDatasetEuler(int ch_param) {
    if(!m_state.connect) return;
    bool is_changed = (ch_param != datasetEuler());
    idDataset->setEuler(ch_param);
    idDataset->commit();
    if(is_changed) { emit datasetChanged();  }
}

void DevDriver::setDatasetEuler(U1 channel_id, bool state) {
    bool is_on = idDataset->getEuler(channel_id);
    if (is_on != state) {
        state ? idDataset->setEuler(channel_id) : idDataset->resetEuler(channel_id);
        emit datasetChanged();
    }
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

void DevDriver::receivedTimestamp(Type type, Version ver, Resp resp)
{
    Q_UNUSED(type);
    Q_UNUSED(ver);
    Q_UNUSED(resp);
}

void DevDriver::receivedDist(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);
    Q_UNUSED(resp);

    emit distComplete(idDist->dist_mm());
}

void DevDriver::receivedChart(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(resp);

    if(idChart->isCompleteChart()) {
        if(ver == v0) {
            QVector<uint8_t> data(idChart->chartSize());
            memcpy(data.data(), idChart->logData8(), idChart->chartSize());

            emit chartComplete(_lastAddres, data, 0.001*idChart->resolution(), 0.001*idChart->offsetRange());

        } else if(ver == v7) {
            QByteArray data((const char*)idChart->rawData(), idChart->rawDataSize());
            emit iqComplete(data, idChart->rawType());
        }
    }
}

void DevDriver::receivedAtt(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);
    Q_UNUSED(resp);
//    qInfo("Euler: yaw %f, pitch %f, roll %f", idAtt->yaw(), idAtt->pitch(), idAtt->roll());
    emit attitudeComplete(idAtt->yaw(), idAtt->pitch(), idAtt->roll());
}

void DevDriver::receivedTemp(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);
    Q_UNUSED(resp);
    core.dataset()->addTemp(idTemp->temp());
}

void DevDriver::receivedDataset(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);

    if(resp == respNone) { emit datasetChanged(); } else {  }
}

void DevDriver::receivedDistSetup(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);

    if(resp == respNone) {   emit distSetupChanged();  }
}

void DevDriver::receivedChartSetup(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);

    if(resp == respNone) {  emit chartSetupChanged();  }
}

void DevDriver::receivedDSPSetup(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);

    if(resp == respNone) {  emit dspSetupChanged();  }
}

void DevDriver::receivedTransc(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);

    if(resp == respNone) {  emit transChanged();  }
}

void DevDriver::receivedSoundSpeed(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);

    if(resp == respNone) {  emit soundChanged();  }
}

void DevDriver::receivedUART(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);

    if(resp == respNone) { emit UARTChanged(); }
}

void DevDriver::receivedVersion(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);

    if(resp == respNone) {

        if(ver == v0) {
            switch (idVersion->boardVersion()) {
            case BoardNone:
                if(idVersion->boardVersionMinor() == BoardAssist) {
                    m_devName = "Assist";
                } else {
                    m_devName = QString("Device ID: %1.%2").arg(idVersion->boardVersion()).arg(idVersion->boardVersionMinor());
                }
                break;
            case BoardEnhanced:
                m_devName = "2D-Enhanced";
                break;
            case BoardChirp:
                m_devName = "2D-Chirp";
                break;
            case BoardBase:
                m_devName = "2D-Base";
                break;
            case BoardNBase:
                m_devName = "2D-Base";
                break;

            case BoardAssist:
            case BoardRecorderMini:
                m_devName = "Recorder";
                break;

            case BoardNEnhanced:
                m_devName = "2D-Enhanced";
                break;
            case BoardSideEnhanced:
                m_devName = "Side-Enhanced";
                break;
            case BoardDVL:
                m_devName = "DVL";
                break;
            case BoardEcho20:
                m_devName = "ECHO20";
                break;
            default:
                m_devName = QString("Device ID: %1.%2").arg(idVersion->boardVersion()).arg(idVersion->boardVersionMinor());
            }

//            qInfo("board info %u", idVersion->boardVersion());
            emit deviceVersionChanged();
        } else if(ver == v1) {
            emit deviceIDChanged(idVersion->uid());
        } else if(ver == v2) {
            if(idVersion->fwVersion() != 0) {
                m_fwVer = QString("%1.%2").arg(idVersion->fwVersion()).arg(idVersion->fwVersionMinor());
            }

            emit deviceVersionChanged();
        }



    }
}

void DevDriver::receivedMark(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);
    Q_UNUSED(resp);
}

void DevDriver::receivedFlash(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);
    Q_UNUSED(resp);
}

void DevDriver::receivedBoot(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);
    Q_UNUSED(resp);
}

void DevDriver::fwUpgradeProcess() {
    bool is_avail_data = idUpdate->putUpdate();
    if(is_avail_data && idUpdate->currentNumPacket() == 3) {
        is_avail_data = idUpdate->putUpdate();
    }
    m_upgrade_status = idUpdate->progress();
    if(!is_avail_data) {
        idBoot->runFW();
        m_bootloader = false;
        m_upgrade_status = successUpgrade;
        core.consoleInfo("Upgrade: done");
        restartState();
    }
}

void DevDriver::receivedUpdate(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);

    if(resp == respNone) {
        if(ver == v0) {
            m_bootloaderLagacyMode = false;
            _timeoutUpgradeAnswerTime = 2000;
            IDBinUpdate::ID_UPGRADE_V0 prog = idUpdate->getDeviceProgress();

            if(prog.type <= 2) {
                _lastUpgradeAnswerTime = QDateTime::currentMSecsSinceEpoch();

                if(prog.type == 1) {
                    core.consoleInfo(QString("Upgrade: back offset condition error with device msg/offset %1 %2, host msg/offset %3 %4").arg(prog.lastNumMsg).arg(prog.lastOffset).arg(idUpdate->currentNumPacket()).arg(idUpdate->currentFwOffset()));
                } else if(prog.type == 2) {
                    core.consoleInfo(QString("Upgrade: forward offset condition error with device msg/offset %1 %2, host msg/offset %3 %4").arg(prog.lastNumMsg).arg(prog.lastOffset).arg(idUpdate->currentNumPacket()).arg(idUpdate->currentFwOffset()));
                    idUpdate->setUpgradeNewPoint(prog.lastNumMsg, prog.lastOffset);
                }

                fwUpgradeProcess();
            } else {
                if(m_bootloader) {
                    core.consoleInfo("Upgrade: critical error!");
                    m_upgrade_status = failUpgrade;
                    m_bootloader = false;
                    m_bootloaderLagacyMode = true;
                    restartState();
                }
            }
        }
    } else {
        if(resp == respOk) {
            if(m_bootloader && m_bootloaderLagacyMode) {
                fwUpgradeProcess();
            }
        } else {
            if(m_bootloader && m_bootloaderLagacyMode) {
                core.consoleInfo("Upgrade: lagacy mode error");
                m_upgrade_status = failUpgrade;
                m_bootloader = false;
                m_bootloaderLagacyMode = true;
                restartState();
            }
        }
    }


    emit upgradeProgressChanged(m_upgrade_status);
    emit upgradeChanged();
}

void DevDriver::receivedNav(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);
    Q_UNUSED(resp);
}

void DevDriver::receivedDVL(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);

    if(resp == respNone) {
        if(ver == v0) {
            emit dopplerVeloComplete();
        } else  if(ver == v1) {
             emit dopplerBeamComplete(idDVL->beams(), idDVL->beamsCount());
        } else  if(ver == v2) {
            emit dvlSolutionComplete(idDVL->dvlSolution());
       }
    }
}

void DevDriver::receivedDVLMode(Type type, Version ver, Resp resp) {
    Q_UNUSED(type);
    Q_UNUSED(ver);
    Q_UNUSED(resp);
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

            if(m_bootloader && !m_bootloaderLagacyMode) {
                int64_t curr_time = QDateTime::currentMSecsSinceEpoch();
                if(curr_time - _lastUpgradeAnswerTime > _timeoutUpgradeAnswerTime && _timeoutUpgradeAnswerTime > 0) {
                    core.consoleInfo("Upgrade: timeout error!");
                    idUpdate->putUpdate(false);
//                    idUpdate->putUpdate();
                }
            }
        }
    }
}
