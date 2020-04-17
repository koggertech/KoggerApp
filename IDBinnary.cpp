#include "IDBinnary.h"
#include "math.h"

IDBin::IDBin(ProtIn *proto, QObject *parent) : QObject(parent) {
    setProto(proto);
}

IDBin::~IDBin() {

}

void IDBin::setProto(ProtIn *proto) {
    m_proto = proto;
}

Resp  IDBin::parse() {
    Resp resp_parse = respNone;

    if(m_proto->type() == CONTENT || m_proto->type() == SETTING || m_proto->type() == GETTING) {
        if(m_proto->resp()) {
           m_lastResp = (Resp)m_proto->read<U1>();
           resp_parse = respOk;
        } else {
            resp_parse = respNone;
            resp_parse = parsePayload(*m_proto);
        }

        if(resp_parse == respOk) {
            m_lastType = m_proto->type();
            m_lastVersion = m_proto->ver();
            emit updateContent(m_lastType, m_lastVersion, m_lastResp);
        } else {
            qInfo("Packet error: id %u, type %u, ver %u, len %u, resp %u", m_proto->id(), m_proto->type(), m_proto->ver(), m_proto->len(), resp_parse);
        }
    }

    return resp_parse;
}

void IDBin::request(Version ver) {
    ProtOut req_out;
    req_out.create(GETTING, ver, id(), 0);
    requestSpecific(req_out);
    req_out.end();

    sendDataProcessing(req_out);
}

void IDBin::appendKey(ProtOut &proto_out) {
    proto_out.write<U4>(m_key);
}

void IDBin::sendDataProcessing(ProtOut &proto_out) {
    QByteArray data((char*)proto_out.data(), proto_out.dataSize());
    dataSend(data);
}


Resp IDBinTimestamp::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        m_timestamp = proto.read<U4>();
    } else {
        return respErrorVersion;
    }

    return respOk;
}


Resp IDBinDist::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        m_dist_mm = proto.read<U4>();
    } else {
        return respErrorVersion;
    }

    return respOk;
}


Resp IDBinChart::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        U2 m_seqOffset = proto.read<U2>();
        U2 sampleResol = proto.read<U2>();
        U2 absOffset = proto.read<U2>();

        if(m_seqOffset == 0 && m_chartSizeIncr != 0) {
            for(uint16_t i = 0; i < m_chartSizeIncr; i++) {
                m_completeChart[i] = m_fillChart[i];
            }
            m_chartSize = m_chartSizeIncr;
            m_isCompleteChart = true;
        }

        if(m_seqOffset == 0 || m_sampleResol != sampleResol || m_absOffset != absOffset) {
            m_sampleResol = sampleResol;
            m_absOffset = absOffset;

            m_chartSizeIncr = 0;
        }

        if(m_chartSizeIncr == m_seqOffset) {
            uint16_t part_len = proto.readAvailable();

            if(m_seqOffset + part_len < sizeof (m_fillChart)) {
                proto.read(&m_fillChart[m_chartSizeIncr], part_len);
                m_chartSizeIncr += part_len;
            }
        } else {
            return respErrorPayload;
        }
    } else {
        return respErrorVersion;
    }

    return respOk;
}




Resp IDBinAttitude::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        const float scale_to_deg = 0.01f;
        m_yaw = static_cast<float>(proto.read<S2>())*scale_to_deg;
        m_pitch = static_cast<float>(proto.read<S2>())*scale_to_deg;
        m_roll = static_cast<float>(proto.read<S2>())*scale_to_deg;
    } else if(proto.ver() == v1) {
       m_w0 = proto.read<F4>();
       m_w0 = proto.read<F4>();
       m_w0 = proto.read<F4>();
       m_w0 = proto.read<F4>();
    } else {
        return respErrorVersion;
    }

    return respOk;
}

float IDBinAttitude::yaw(Version src_ver) {
    return m_yaw;
}

float IDBinAttitude::pitch(Version src_ver) {
    return m_pitch;
}

float IDBinAttitude::roll(Version src_ver) {
    return m_roll;
}

float IDBinAttitude::w0(Version src_ver) {
    return m_w0;
}
float IDBinAttitude::w1(Version src_ver) {
    return m_w1;
}
float IDBinAttitude::w2(Version src_ver) {
    return m_w2;
}
float IDBinAttitude::w3(Version src_ver) {
    return m_w3;
}

Resp IDBinTemp::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        const float scale_to_cels = 0.01f;
        m_temp = static_cast<float>(proto.read<S2>())*scale_to_cels;
    } else {
        return respErrorVersion;
    }

    return respOk;
}


Resp IDBinNav::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {

    } else {
        return respErrorVersion;
    }


    return respOk;
}



Resp IDBinDataset::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        uint8_t ch_id = proto.read<U1>();

        if(ch_id < _countof(m_channel)) {
            m_channel[ch_id].id = ch_id;
            m_channel[ch_id].period = proto.read<U4>();
            m_channel[ch_id].mask = proto.read<U4>();
        }
    } else {
        return respErrorVersion;
    }

    return respOk;
}

Resp IDBinDistSetup::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        m_startOffset = proto.read<U4>();
        m_maxDist = proto.read<U4>();
    } else {
        return respErrorVersion;
    }

    return respOk;
}

Resp IDBinChartSetup::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        m_sanpleCount = proto.read<U2>();
        m_sanpleResolution = proto.read<U2>();
        m_sanpleOffset = proto.read<U2>();
    } else {
        return respErrorVersion;
    }

    return respOk;
}

Resp IDBinTransc::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        m_freq = proto.read<U2>();
        m_pulse = proto.read<U1>();
        m_boost = proto.read<U1>();
    } else {
        return respErrorVersion;
    }

    return respOk;
}

Resp IDBinSoundSpeed::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        m_soundSpeed = proto.read<U4>();
    } else {
        return respErrorVersion;
    }

    return respOk;
}

Resp IDBinUART::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        if(checkKeyConfirm(proto.read<U4>())) {
            uint8_t uart_id = proto.read<U1>();

            if(uart_id < _countof(m_uart)) {
                m_uart[uart_id].id = uart_id;
                m_uart[uart_id].baudrate = proto.read<U4>();
            }
        } else {
            return respErrorKey;
        }
    } else if(proto.ver() == v1) {
        if(checkKeyConfirm(proto.read<U4>())) {
            uint8_t uart_id = proto.read<U1>();

            if(uart_id < _countof(m_uart)) {
                m_uart[uart_id].id = uart_id;
                m_uart[uart_id].dev_address = proto.read<U1>();
            }
        } else {
            return respErrorKey;
        }
    } else {
        return respErrorVersion;
    }

    return respOk;
}

Resp IDBinMark::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        m_mark = proto.read<U1>();
    } else {
        return respErrorVersion;
    }

    return respOk;
}

void IDBinMark::setMark() {
    ProtOut id_out;
    id_out.create(SETTING, v0, id(), 0);
    appendKey(id_out);
    id_out.end();

    sendDataProcessing(id_out);
}

Resp IDBinFlash::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        if(checkKeyConfirm(proto.read<U4>())) {
        } else {
            return respErrorKey;
        }
    } else if(proto.ver() == v1) {
        if(checkKeyConfirm(proto.read<U4>())) {
        } else {
            return respErrorKey;
        }
    } else if(proto.ver() == v2) {
        if(checkKeyConfirm(proto.read<U4>())) {
        } else {
            return respErrorKey;
        }
    } else {
        return respErrorVersion;
    }

    return respOk;
}

void IDBinFlash::flashing() {

}

void IDBinFlash::restore() {

}

void IDBinFlash::erase() {

}

Resp IDBinBoot::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
        if(checkKeyConfirm(proto.read<U4>())) {
        } else {
            return respErrorKey;
        }
    } else if(proto.ver() == v1) {
        if(checkKeyConfirm(proto.read<U4>())) {
        } else {
            return respErrorKey;
        }
    } else {
        return respErrorVersion;
    }

    return respOk;
}

void IDBinBoot::reboot() {
    ProtOut id_out;
    id_out.create(SETTING, v0, id(), 0);
    appendKey(id_out);
    id_out.end();
    sendDataProcessing(id_out);
}

void IDBinBoot::runFW() {
    ProtOut id_out;
    id_out.create(SETTING, v1, id(), 0);
    appendKey(id_out);
    id_out.end();
    sendDataProcessing(id_out);
}

Resp IDBinUpdate::parsePayload(ProtIn &proto) {
    if(proto.ver() == v0) {
    } else {
        return respErrorVersion;
    }

    return respOk;
}

void IDBinUpdate::setUpdate(QByteArray fw) {
    _fw = fw;
    _nbr_packet = 1;
}

bool IDBinUpdate::putUpdate() {
    uint16_t len_part = 64;
    if(len_part > _fw.length()) {
        len_part = (uint16_t)_fw.length();
    }

    if(len_part == 0) {
        return false;
    }

    ProtOut id_out;
    id_out.create(SETTING, v0, id(), 0);
    id_out.write<U2>(_nbr_packet);

    for(uint16_t i = 0; i < len_part; i++) {
        id_out.write<U1>((U1)_fw[i]);
    }
    _fw.remove(0, len_part);

    _nbr_packet++;

    id_out.end();
    sendDataProcessing(id_out);

    return true;
}
