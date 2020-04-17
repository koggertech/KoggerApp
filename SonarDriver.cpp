#include "SonarDriver.h"
#include "QTimer"
SonarDriver::SonarDriver(QObject *parent) :
    QObject(parent),
    m_proto(new ProtIn()),
    idTimestamp(new IDBinTimestamp(m_proto)),
    idDist(new IDBinDist(m_proto)),
    idChart(new IDBinChart(m_proto)),
    idAtt(new IDBinAttitude(m_proto)),
    idTemp(new IDBinTemp(m_proto)),

    idDataset(new IDBinDataset(m_proto)),
    idDistSetup(new IDBinDistSetup(m_proto)),
    idChartSetup(new IDBinChartSetup(m_proto)),
    idTransc(new IDBinTransc(m_proto)),
    idSoundSpeed(new IDBinSoundSpeed(m_proto)),
    idUART(new IDBinUART(m_proto)),

    idMark(new IDBinMark(m_proto)),
    idFlash(new IDBinFlash(m_proto)),
    idBoot(new IDBinBoot(m_proto)),
    idUpdate(new IDBinUpdate(m_proto)),

    idNav(new IDBinNav(m_proto))
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

    regID(idMark, &SonarDriver::receivedMark);
    regID(idFlash, &SonarDriver::receivedFlash);
    regID(idBoot, &SonarDriver::receivedBoot);
    regID(idUpdate, &SonarDriver::receivedUpdate);

    regID(idNav, &SonarDriver::receivedNav);
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
}

void SonarDriver::putData(const QByteArray &data) {
    for(int i = 0; i < data.size(); i++) {
        Resp resp = m_proto->putByte(static_cast<uint8_t>(data.at(i)));
        if(resp == respOk) {
            protoComplete(*m_proto);
        }
    }
}

void SonarDriver::protoComplete(ProtIn &proto) {
    if(proto.mark() == false) {
//        idMark->setMark();
//        requestSetup();
    }

    if(hashIDParsing.contains(proto.id())) {
        hashIDParsing[proto.id()]->parse();
    } else {
        qInfo("ID is not find: %u", proto.id());
    }
}

void SonarDriver::sendUpdateFW(QByteArray update_data) {
    idUpdate->setUpdate(update_data);
    idBoot->reboot();
    QTimer::singleShot(250, idUpdate, SLOT(putUpdate()));
}

void SonarDriver::receivedTimestamp(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
    qInfo("Timestamp: %u", idTimestamp->timestamp());
}

void SonarDriver::receivedDist(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}

void SonarDriver::receivedChart(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
    if(idChart->isCompleteChart()) {
        QVector<uint8_t> data(idChart->chartSize());
        uint8_t* raw_data = idChart->rawData();
        for(int i = 0; i < data.length(); i++) {
            data[i] = raw_data[i];
        }
        emit chartComplete(data, 10, 0);
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
}

void SonarDriver::receivedDistSetup(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}

void SonarDriver::receivedChartSetup(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}

void SonarDriver::receivedTransc(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}

void SonarDriver::receivedSoundSpeed(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}

void SonarDriver::receivedUART(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
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

    if(resp == respOk) {
        bool is_avail_data = idUpdate->putUpdate();
        if(!is_avail_data) {
            idBoot->runFW();
        }
    }
}

void SonarDriver::receivedNav(Type type, Version ver, Resp resp) {
    Q_UNUSED(type)
    Q_UNUSED(ver)
}
