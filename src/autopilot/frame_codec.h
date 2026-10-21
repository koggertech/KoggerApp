#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <QByteArray>

namespace autopilot {

/**
 * Builds one complete MAVLink frame sent by this ground station (sysid 255, compid 190).
 * Version 1 frames carry only the first @p v1Length payload bytes (extensions are not part of
 * MAVLink 1); version 2 frames carry the whole payload, unsigned and not truncated.
 */
QByteArray encodeFrame(int version, uint8_t seq, uint32_t msgId, const QByteArray& payload, int v1Length);

template<typename T>
QByteArray payloadOf(const T& message)
{
    return QByteArray(reinterpret_cast<const char*>(&message), int(sizeof(T)));
}

/**
 * Copies a received payload into a message struct. Bytes missing from a truncated MAVLink 2
 * payload (or from absent extensions) stay zero, as the protocol defines.
 */
template<typename T>
T decodePayload(const QByteArray& payload)
{
    T message{};
    std::memset(static_cast<void*>(&message), 0, sizeof(T));
    std::memcpy(static_cast<void*>(&message), payload.constData(), std::min<size_t>(sizeof(T), size_t(payload.size())));
    return message;
}

} // namespace autopilot
