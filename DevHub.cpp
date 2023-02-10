#include "DevHub.h"

#include <core.h>
extern Core core;
#include <QDateTime>

void Device::putData(const QByteArray &data) {

    uint8_t* ptr_data = (uint8_t*)(data.data());
    if(ptr_data == NULL || data.size() < 1) { return; }
    m_proto.setContext(ptr_data, data.size());

    while (m_proto.availContext()) {
        m_proto.process();
        if(!m_proto.isComplete()) { continue; }

//        qInfo("Packets: good %u, frame error %u, check error %u", m_proto.binComplete(),  m_proto.frameError(),  m_proto.binError());

        if(m_proto.completeAsKBP2() && m_proto.isStream()) {
            _streamList.append(&m_proto);
        }

        if(m_proto.id() == ID_STREAM) {
            _streamList.parse(&m_proto);
        }

        if(_streamList.isListChenged()) {
            emit streamChanged();
        }

        if(m_proto.completeAsKBP() || m_proto.completeAsKBP2()) {
            uint8_t addr = m_proto.route();

            if(lastRoute != addr) {
                if(devAddr[addr] == NULL) {
                    createDev(addr, _isDuplex);
                } else {
                    lastDevs = devAddr[addr];
                    lastRoute = addr;
                }
            }


            if(_isConsoled && (_isDuplex || 1) && !(m_proto.id() == 33 || m_proto.id() == 33)) { core.consoleProto(m_proto); }

#if !defined(Q_OS_ANDROID)
            if(m_proto.id() == ID_TIMESTAMP && m_proto.ver() == v1) {
                int timestamp = m_proto.read<U4>();
                int unix = m_proto.read<U4>();
                core.plot()->addEvent(timestamp, 0, unix);
            }

            if(m_proto.id() == ID_EVENT) {
                int timestamp = m_proto.read<U4>();
                int id = m_proto.read<U4>();
                if(id < 100) {
                    core.plot()->addEvent(timestamp, id);
                }
            }

            if(m_proto.id() == ID_VOLTAGE) {
                int v_id = m_proto.read<U1>();
                int32_t v_uv = m_proto.read<S4>();
                if(v_id == 1) {
                    core.plot()->addEncoder(float(v_uv));
                    qInfo("Voltage %f", float(v_uv));
                }
            }
#endif

            lastDevs->protoComplete(m_proto);

        } else if(m_proto.completeAsNMEA()) {
            ProtoNMEA& prot_nmea = (ProtoNMEA&)m_proto;

            if(_isConsoled) {
                QString str_data = QByteArray((char*)prot_nmea.frame(), prot_nmea.frameLen() - 2);
                core.consoleInfo(QString(">> NMEA: %5").arg(str_data));
            }

            if(prot_nmea.isEqualId("RMC")) {
                uint8_t h = 0, m = 0, s = 0;
                uint16_t ms = 0;
                bool is_correct =  prot_nmea.readTime(&h, &m, &s, &ms);
                char c = prot_nmea.readChar();
                if(c == 'A') {
                    double lat = prot_nmea.readLatitude();
                    double lon = prot_nmea.readLongitude();

                    prot_nmea.skip();
                    prot_nmea.skip();

                    uint16_t year = 0;
                    uint8_t mounth = 0, day = 0;
                    prot_nmea.readDate(&year, &mounth, & day);

                    uint32_t unix_time = QDateTime(QDate(year, mounth, day), QTime(h, m, s), Qt::TimeSpec::UTC).toTime_t();
                    core.plot()->addPosition(lat, lon, unix_time, (uint32_t)ms*1000*1000);
                }
            }
        } else if(m_proto.isCompleteAsUBX()) {
            ProtoUBX& ubx_frame = (ProtoUBX&)m_proto;

            if(ubx_frame.msgClass() == 1 && ubx_frame.msgId() == 7) {

                uint8_t h = 0, m = 0, s = 0;
                uint16_t year = 0;
                uint8_t mounth = 0, day = 0;
                int32_t nanosec = 0;

                ubx_frame.readSkip(4);
                year = ubx_frame.read<U2>();
                mounth = ubx_frame.read<U1>();
                day = ubx_frame.read<U1>();
                h = ubx_frame.read<U1>();
                m = ubx_frame.read<U1>();
                s = ubx_frame.read<U1>();
                ubx_frame.read<U1>(); // Validity flags
                ubx_frame.readSkip(4); // Time accuracy estimate (UTC)
                nanosec = ubx_frame.read<S4>();

                uint8_t fix_type = ubx_frame.read<U1>();
                uint8_t fix_flags = ubx_frame.read<U1>();
                ubx_frame.read<U1>();
                uint8_t satellites_in_used = ubx_frame.read<U1>();

                int32_t lon_int = ubx_frame.read<S4>();
                int32_t lat_int = ubx_frame.read<S4>();

                uint32_t unix_time = QDateTime(QDate(year, mounth, day), QTime(h, m, s), Qt::TimeSpec::UTC).toTime_t();

                if(fix_type > 1 && fix_type < 5) {
                    core.plot()->addPosition(double(lat_int)*0.0000001, double(lon_int)*0.0000001, unix_time, nanosec);
                }

                if(_isConsoled) {
                    core.consoleInfo(QString(">> UBX: NAV_PVT, fix %1, sats %2, lat %3, lon %4").arg(fix_type).arg(satellites_in_used).arg(double(lat_int)*0.0000001).arg(double(lon_int)*0.0000001));
                }
            } else {
                if(_isConsoled) {
                    core.consoleInfo(QString(">> UBX: class %1, id %2, len %3").arg(ubx_frame.msgClass()).arg(ubx_frame.msgId()).arg(ubx_frame.frameLen()));
                }
            }


        }
    }

    int a = 0;
}

void Device::binFrameOut(ProtoBinOut &proto_out) {
    QByteArray data((char*)proto_out.frame(), proto_out.frameLen());
    emit dataSend(data);
    if(_isConsoled && _isDuplex && !(proto_out.id() == 33 || proto_out.id() == 33)) { core.consoleProto(proto_out, false); }
}

void Device::startConnection(bool duplex) {
    _isDuplex = duplex;
    createDev(0, duplex);
}

void Device::stopConnection() {
    delAllDev();
}

void Device::upgradeLastDev(QByteArray data) {
    if(lastDevs != NULL) {
        lastDevs->sendUpdateFW(data);
    }
}
