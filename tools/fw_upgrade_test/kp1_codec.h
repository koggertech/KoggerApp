#pragma once

// Device-side KP1 codec. Deliberately a second, independent implementation: if it were
// built on ProtoBinOut the test would confirm that the host agrees with itself.

#include <QByteArray>
#include <QVector>

#include "proto_binnary.h"

namespace FwTest {

using namespace Parsers;

struct WireFrame {
    Type     type    = CONTENT;
    Version  ver     = v0;
    ID       id      = ID_NONE;
    uint8_t  route   = 0;
    bool     mark    = false;
    bool     resp    = false;
    uint16_t checksum = 0;
    QByteArray payload;
};

inline void put8(QByteArray& b, uint8_t v)   { b.append(static_cast<char>(v)); }
inline void put16(QByteArray& b, uint16_t v) { put8(b, uint8_t(v & 0xFF)); put8(b, uint8_t(v >> 8)); }
inline void put32(QByteArray& b, uint32_t v) { put16(b, uint16_t(v & 0xFFFF)); put16(b, uint16_t(v >> 16)); }

inline uint16_t rd16(const QByteArray& b, int at) {
    return uint16_t(uint8_t(b.at(at))) | uint16_t(uint16_t(uint8_t(b.at(at + 1))) << 8);
}

inline QByteArray encode(Type type, Version ver, ID id, uint8_t route,
                         bool mark, bool resp, const QByteArray& payload)
{
    QByteArray f;
    put8(f, 0xBB);
    put8(f, 0x55);
    put8(f, route);
    put8(f, uint8_t((uint8_t(type) & 0x3)
                    | uint8_t((uint8_t(ver) & 0x7) << 3)
                    | uint8_t(mark ? 0x40 : 0)
                    | uint8_t(resp ? 0x80 : 0)));
    put8(f, uint8_t(id));
    put8(f, uint8_t(payload.size()));
    f.append(payload);

    uint8_t c1 = 0, c2 = 0;
    for (int i = 2; i < f.size(); ++i) { c1 = uint8_t(c1 + uint8_t(f.at(i))); c2 = uint8_t(c2 + c1); }
    put8(f, c1);
    put8(f, c2);
    return f;
}

// Decodes exactly one complete KP1 frame from the head of `bytes`. The host writes one
// frame per binFrameOut, so streaming reassembly is not needed on this side.
inline bool decode(const QByteArray& bytes, WireFrame* out)
{
    if (bytes.size() < 8) return false;
    if (uint8_t(bytes.at(0)) != 0xBB || uint8_t(bytes.at(1)) != 0x55) return false;

    const int payloadLen = uint8_t(bytes.at(5));
    const int total = 6 + payloadLen + 2;
    if (bytes.size() < total) return false;

    uint8_t c1 = 0, c2 = 0;
    for (int i = 2; i < 6 + payloadLen; ++i) { c1 = uint8_t(c1 + uint8_t(bytes.at(i))); c2 = uint8_t(c2 + c1); }
    if (c1 != uint8_t(bytes.at(6 + payloadLen)) || c2 != uint8_t(bytes.at(6 + payloadLen + 1))) return false;

    const uint8_t mode = uint8_t(bytes.at(3));
    out->route    = uint8_t(bytes.at(2));
    out->type     = Type(mode & 0x3);
    out->ver      = Version((mode >> 3) & 0x7);
    out->mark     = ((mode >> 6) & 0x1) == 0x1;
    out->resp     = ((mode >> 7) & 0x1) == 0x1;
    out->id       = ID(uint8_t(bytes.at(4)));
    out->payload  = bytes.mid(6, payloadLen);
    out->checksum = rd16(bytes, 6 + payloadLen);
    return true;
}

} // namespace FwTest
