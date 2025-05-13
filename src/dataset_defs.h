#pragma once

#include <stdint.h>
#include <functional>
#include <QUuid>
#include <QHash>
#include <QObject>
#include <QString>
#include <QDebug>


struct ChannelId
{
  ChannelId()
        : uuid(), address(0)
    {
    }

    ChannelId(const QUuid& uuid, uint8_t address)
        : uuid(uuid), address(address)
    {
    }

    bool isValid() const
    {
        return !uuid.isNull();
    }

    QString toShortName() const
    {
        QString uuidPart = uuid.toString(QUuid::WithoutBraces).left(4).toUpper();
        QString addrPart = QString("%1").arg(address, 2, 10, QChar('0'));
        return uuidPart + "|" + addrPart;
    }

    bool operator==(const ChannelId& other) const
    {
        return uuid == other.uuid && address == other.address;
    }

    bool operator!=(const ChannelId& other) const
    {
        return !(*this == other);
    }

    bool operator<(const ChannelId& other) const
    {
        if (uuid < other.uuid) return true;
        if (uuid > other.uuid) return false;
        return address < other.address;
    }

    /*data*/
    QUuid uuid; // usually link ID
    uint8_t address;
};

// std::unordered_map, std::unordered_set
namespace std {
template <>
struct hash<ChannelId>
{
    size_t operator()(const ChannelId& key) const noexcept
    {
        const uint32_t* p = reinterpret_cast<const uint32_t*>(&key.uuid);

        return (hash<uint32_t>()(p[0])) ^
               (hash<uint32_t>()(p[1]) << 1) ^
               (hash<uint32_t>()(p[2]) << 2) ^
               (hash<uint32_t>()(p[3]) << 3) ^
               (hash<uint8_t>()(key.address) << 4);
    }
};
} // namespace std

// QHash, QSet
inline uint qHash(const ChannelId& key, uint seed = 0)
{
    std::size_t stlHash = std::hash<ChannelId>()(key);
    return static_cast<uint>(stlHash ^ (seed * 0x9e3779b9));
}


static const ChannelId CHANNEL_NONE  = ChannelId();
