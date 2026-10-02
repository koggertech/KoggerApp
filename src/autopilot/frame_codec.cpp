#include "frame_codec.h"

#include <algorithm>

#include "autopilot_messages.h"
#include "proto_binnary.h"

namespace autopilot {

QByteArray encodeFrame(int version, uint8_t seq, uint32_t msgId, const QByteArray& payload, int v1Length)
{
    const int length = std::clamp(version == 1 ? std::min(v1Length, int(payload.size())) : int(payload.size()), 0, 255);

    QByteArray out;
    out.reserve(length + 12);
    if (version == 1) {
        out.append(char(0xFE));
        out.append(char(length));
        out.append(char(seq));
        out.append(char(kMavGcsSystemId));
        out.append(char(kMavGcsComponentId));
        out.append(char(msgId & 0xFF));
    } else {
        out.append(char(0xFD));
        out.append(char(length));
        out.append(char(0));
        out.append(char(0));
        out.append(char(seq));
        out.append(char(kMavGcsSystemId));
        out.append(char(kMavGcsComponentId));
        out.append(char(msgId & 0xFF));
        out.append(char((msgId >> 8) & 0xFF));
        out.append(char((msgId >> 16) & 0xFF));
    }
    out.append(payload.constData(), length);

    uint16_t crc = CRC16_MCRF4XX(reinterpret_cast<uint8_t*>(out.data()) + 1, uint16_t(out.size() - 1), 0xFFFF);
    uint8_t extra = getMAVLinkExtra(msgId);
    crc = CRC16_MCRF4XX(&extra, 1, crc);
    out.append(char(crc & 0xFF));
    out.append(char(crc >> 8));
    return out;
}

} // namespace autopilot
