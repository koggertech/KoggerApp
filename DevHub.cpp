#include "DevHub.h"

#include <core.h>
extern Core core;

void Device::putData(const QByteArray &data) {

    uint8_t* ptr_data = (uint8_t*)(data.data());
    if(ptr_data == NULL || data.size() < 1) { return; }
    m_proto.setContext(ptr_data, data.size());

    while (m_proto.availContext()) {
        m_proto.process();

        if(m_proto.completeAsKBP()) {
            ProtoKP1In& prot_bin = (ProtoKP1In&)m_proto;
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

            if(_isConsoled && _isDuplex && !(prot_bin.id() == 32 || prot_bin.id() == 33)) { core.consoleProto(prot_bin); }

#if !defined(Q_OS_ANDROID)
            if(prot_bin.id() == ID_EVENT) {
                int timestamp = prot_bin.read<U4>();
                int id = prot_bin.read<U4>();
                if(id < 100) {
                    core.plot()->addEvent(timestamp, id);
                }
            }

            if(prot_bin.id() == ID_VOLTAGE) {
                int v_id = prot_bin.read<U1>();
                int32_t v_uv = prot_bin.read<S4>();
                if(v_id == 1) {
                    core.plot()->addEncoder(float(v_uv));
                    qInfo("Voltage %f", float(v_uv));
                }
            }
#endif

        } else if(m_proto.completeAsNMEA()) {
            ProtoNMEA& prot_nmea = (ProtoNMEA&)m_proto;

            if(true) {
                QString str_data = QByteArray((char*)prot_nmea.frame(), prot_nmea.frameLen() - 2);
                core.consoleInfo(QString(">> NMEA: %5").arg(str_data));
            }

            if(prot_nmea.isEqualId("RMC")) {
                uint32_t time_ms = prot_nmea.readTimems();
                prot_nmea.skip();
                double lat = prot_nmea.readLatitude();
                double lon = prot_nmea.readLongitude();
                core.plot()->addPosition(0, time_ms, lat, lon);
            }
        } else if(m_proto.isCompleteAsUBX()) {
            ProtoUBX& ubx_frame = (ProtoUBX&)m_proto;

            if(ubx_frame.msgClass() == 1 && ubx_frame.msgId() == 7) {
                ubx_frame.readSkip(20);

                uint8_t fix_type = ubx_frame.read<U1>();
                uint8_t fix_flags = ubx_frame.read<U1>();
                ubx_frame.read<U1>();
                uint8_t satellites_in_used = ubx_frame.read<U1>();

                int32_t lon_int = ubx_frame.read<S4>();
                int32_t lat_int = ubx_frame.read<S4>();

                if(fix_type > 1 && fix_type < 5) {
                    core.plot()->addPosition(0, 0, double(lat_int)*0.0000001, double(lon_int)*0.0000001);
                }
                core.consoleInfo(QString(">> UBX: NAV_PVT, fix %1, sats %2, lat %3, lon %4").arg(fix_type).arg(satellites_in_used).arg(double(lat_int)*0.0000001).arg(double(lon_int)*0.0000001));
            } else {
                core.consoleInfo(QString(">> UBX: class %1, id %2, len %3").arg(ubx_frame.msgClass()).arg(ubx_frame.msgId()).arg(ubx_frame.frameLen()));
            }


        }
    }
}

void Device::binFrameOut(ProtoBinOut &proto_out) {
    QByteArray data((char*)proto_out.frame(), proto_out.frameLen());
    emit dataSend(data);
    if(_isConsoled && _isDuplex && !(proto_out.id() == 32 || proto_out.id() == 33)) { core.consoleProto(proto_out, false); }
}

void Device::startConnection(bool duplex) {
    _isDuplex = duplex;
    createDev(0, duplex);
}

void Device::stopConnection() {
    delAllDev();
}
