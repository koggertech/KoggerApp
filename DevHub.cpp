#include "DevHub.h"

#include <core.h>
extern Core core;
#include <QDateTime>

void Device::putData(const QByteArray &data) {

    uint8_t* ptr_data = (uint8_t*)(data.data());
    if(ptr_data == NULL || data.size() < 1) { return; }
    _parser.setContext(ptr_data, data.size());

    while (_parser.availContext()) {
        _parser.process();
        if(!_parser.isComplete()) { continue; }

        if(isProxyNavOpen() && (_parser.isCompleteAsNMEA() || _parser.isCompleteAsUBX() || _parser.isCompleteAsMAVLink())) {
            emit writeProxyNav(QByteArray((char*)_parser.frame(), _parser.frameLen()));
            continue;
        }

        if(isProxyOpen() && (_parser.isComplete())) {
            emit writeProxy(QByteArray((char*)_parser.frame(), _parser.frameLen()));
            continue;
        }

        //        qInfo("Packets: good %u, frame error %u, check error %u", m_proto.binComplete(),  m_proto.frameError(),  m_proto.binError());

        if(_parser.isStream()) {
            _streamList.append(&_parser);
        }

        if(_parser.id() == ID_STREAM) {
            _streamList.parse(&_parser);
        }

        if(_streamList.isListChenged()) {
            emit streamChanged();
        }

        if(_parser.isProxy()) {
            continue;
        }

        if(_parser.completeAsKBP() || _parser.completeAsKBP2()) {
            uint8_t addr = _parser.route();

            if(lastRoute != addr) {
                if(devAddr[addr] == NULL) {
                    createDev(addr, _isDuplex);
                } else {
                    lastDevs = devAddr[addr];
                    lastRoute = addr;
                }
            }


            if(_isConsoled && (_isDuplex || 1) && !(_parser.id() == 33 || _parser.id() == 33)) { core.consoleProto(_parser); }

#if !defined(Q_OS_ANDROID)
            if(_parser.id() == ID_TIMESTAMP && _parser.ver() == v1) {
                int timestamp = _parser.read<U4>();
                int unix = _parser.read<U4>();
                core.plot()->addEvent(timestamp, 0, unix);
                core.consoleInfo(QString("Event time %1.%2").arg(unix).arg(timestamp));
            }

            if(_parser.id() == ID_EVENT) {
                int timestamp = _parser.read<U4>();
                int id = _parser.read<U4>();
                if(id < 100) {
                    core.plot()->addEvent(timestamp, id);
                }
            }

            if(_parser.id() == ID_VOLTAGE) {
                int v_id = _parser.read<U1>();
                int32_t v_uv = _parser.read<S4>();
                if(v_id == 1) {
                    core.plot()->addEncoder(float(v_uv));
                    qInfo("Voltage %f", float(v_uv));
                }
            }
#endif

            lastDevs->protoComplete(_parser);
        }

        if(_parser.isCompleteAsNMEA()) {
            ProtoNMEA& prot_nmea = (ProtoNMEA&)_parser;

            //            if(_isConsoled) {
            QString str_data = QByteArray((char*)prot_nmea.frame(), prot_nmea.frameLen() - 2);
            core.consoleInfo(QString(">> NMEA: %5").arg(str_data));
            //            }

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
        }

        if(_parser.isCompleteAsUBX()) {
            ProtoUBX& ubx_frame = (ProtoUBX&)_parser;

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
                    core.consoleInfo(QString(">> UBX: NAV_PVT, fix %1, sats %2, lat %3, lon %4, time %5:%6:%7.%8").arg(fix_type).arg(satellites_in_used).arg(double(lat_int)*0.0000001).arg(double(lon_int)*0.0000001).arg(h).arg(m).arg(s).arg(nanosec/1000));
                }
            } else {
                if(_isConsoled) {
                    core.consoleInfo(QString(">> UBX: class/id 0x%1 0x%2, len %3").arg(ubx_frame.msgClass(), 2, 16, QLatin1Char('0')).arg(ubx_frame.msgId(), 2, 16, QLatin1Char('0')).arg(ubx_frame.frameLen()));
                }
            }
        }

        if(_parser.isCompleteAsMAVLink()) {
            ProtoMAVLink& mavlink_frame = (ProtoMAVLink&)_parser;
            if(mavlink_frame.msgId() == 33) { // GLOBAL_POSITION_INT
                MAVLink_MSG_GLOBAL_POSITION_INT pos = mavlink_frame.read<MAVLink_MSG_GLOBAL_POSITION_INT>();
                if(pos.isValid()) {
                    core.plot()->addPosition(pos.latitude(), pos.longitude(), pos.time_boot_msec()/1000, (pos.time_boot_msec()%1000)*1e6);
                    core.consoleInfo(QString(">> FC: fused position lat/lon %1 %2").arg(pos.latitude()).arg(pos.longitude()));
                }
            }

//            if(mavlink_frame.msgId() == 1) { // SYS_STATUS
//                MAVLink_MSG_SYS_STATUS sys_status = mavlink_frame.read<MAVLink_MSG_SYS_STATUS>();

//                core.consoleInfo(QString(">> FC: Battery voltage %1V, current %2A").arg(sys_status.batteryVoltage()).arg(sys_status.batteryCurrent()));
//            }

            if(mavlink_frame.msgId() == 147) { // BATTERY_STATUS
                 MAVLink_MSG_BATTERY_STATUS battery_status = mavlink_frame.read<MAVLink_MSG_BATTERY_STATUS>();
                 core.consoleInfo(QString(">> FC: Battery voltage %1V, current %2A").arg(battery_status.voltage()).arg(battery_status.current()));
            }



            if(_isConsoled) {
                core.consoleInfo(QString(">> MAVLink v%1: ID %2, comp. id %3, seq numb %4, len %5").arg(mavlink_frame.MAVLinkVersion()).arg(mavlink_frame.msgId()).arg(mavlink_frame.componentID()).arg(mavlink_frame.sequenceNumber()).arg(mavlink_frame.frameLen()));
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

void Device::openProxyLink(const QString &address, const int port_in, const int port_out) {
    closeProxyLink();
    connect(&proxyLink, &Link::readyParse, this, &Device::readyReadProxy);
    connect(this, &Device::writeProxy, &proxyLink, &Link::write);
    proxyLink.openAsUDP(address, port_in, port_out);
    if(proxyLink.isOpen()) {
        core.consoleInfo("Proxy port is open");
    } else {
        this->disconnect(&proxyLink);
        proxyLink.disconnect(this);
        core.consoleInfo("Proxy port isn't open");
    }
}

void Device::openProxyNavLink(const QString &address, const int port_in, const int port_out) {
    closeProxyNavLink();
    connect(&proxyNavLink, &Link::readyParse, this, &Device::readyReadProxyNav);
    connect(this, &Device::writeProxyNav, &proxyNavLink, &Link::write);
    proxyNavLink.openAsUDP(address, port_in, port_out);
    if(proxyNavLink.isOpen()) {
        core.consoleInfo("Proxy Nav port is open");
    } else {
        this->disconnect(&proxyNavLink);
        proxyNavLink.disconnect(this);
        core.consoleInfo("Proxy Nav port isn't open");
    }
}

void Device::closeProxyLink() {
    proxyLink.close();
    this->disconnect(&proxyLink);
    proxyLink.disconnect(this);
}

void Device::closeProxyNavLink() {
    proxyNavLink.close();
    this->disconnect(&proxyNavLink);
    proxyNavLink.disconnect(this);
}

void Device::gatewayKP() {

}

void Device::gatewayUBX() {

}

void Device::gatewayNMEA() {

}

void Device::gatewayMAVLink() {

}

void Device::readyReadProxy(Link* link) {
    while(link->parse()) {
        FrameParser* frame = link->frameParser();
        if(frame->isComplete()) {
            QByteArray data((char*)frame->frame(), frame->frameLen());
            emit dataSend(data);
        }
    }
}

void Device::readyReadProxyNav(Link* link) {
    while(link->parse()) {
        FrameParser* frame = link->frameParser();
        if(frame->isComplete()) {
            QByteArray data((char*)frame->frame(), frame->frameLen());
            emit dataSend(data);
        }
    }
}
