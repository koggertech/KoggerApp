#include "DevHub.h"

#include <core.h>
extern Core core;

void Device::putData(const QByteArray &data) {
    uint8_t* ptr_data = (uint8_t*)(data.data());
    m_proto.setContext(ptr_data, data.size());

    while (m_proto.availContext()) {
        m_proto.process();

        if(m_proto.completeAsKBP()) {
            ProtoBinIn& prot_bin = (ProtoBinIn&)m_proto;
            uint8_t addr = prot_bin.route();

            if(lastRoute != addr) {
                if(devAddr[addr] == NULL) {
                    createDev(addr, _isDuplex);
                } else {
                    lastDevs = devAddr[addr];
                    lastRoute = addr;
                }
            }

            lastDevs->protoComplete(prot_bin);

            if(_isConsoled && _isDuplex) { core.consoleProto(prot_bin); }

            if(prot_bin.id() == 48) {
                int timestamp = prot_bin.read<U4>();
                int id = prot_bin.read<U4>();
                core.plot()->addEvent(timestamp, id);
//                core.consoleProto(prot_bin);
            }

        } else if(m_proto.completeAsNMEA()) {
            ProtoNMEA& prot_nmea = (ProtoNMEA&)m_proto;

            if(true && _isDuplex) {
                QString str_data = QByteArray((char*)prot_nmea.frame(), prot_nmea.frameLen() - 2);
                core.consoleInfo(QString(">> NMEA: %5").arg(str_data));
            }
        }
    }
}

void Device::binFrameOut(ProtoBinOut &proto_out) {
    QByteArray data((char*)proto_out.frame(), proto_out.frameLen());
    emit dataSend(data);
    if(_isConsoled && _isDuplex) { core.consoleProto(proto_out, false); }
}

void Device::startConnection(bool duplex) {
    _isDuplex = duplex;
    createDev(0, duplex);
}

void Device::stopConnection() {
    delAllDev();
}
