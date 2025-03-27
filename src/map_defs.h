#pragma once

#include <cstdint>
#include <QDateTime>
#include <QVector>
#include <QVector3D>
#include <QImage>
#include <QPair>
#include <QHash>
#include <QMetaType>
#include <QOpenGLFunctions>
#include <QtMath>
#include <functional>

#include "plotcash.h"


namespace map {


/*data*/
inline const QVector<QVector2D> kTextureCoords = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f}
};

inline const QVector<int> kIndices = {
    0, 1, 2,
    0, 2, 3
};

/*structures*/
enum class CameraTilt {
    Up = 0,
    Down,
    Left,
    Right
};

enum class ZoomState {
    kUndefined = 0,
    kOut,
    kUnchanged,
    kIn
};

enum class TilePosition {
    kFits = 0,
    kOnLeft,
    kOnRight
};

struct GeoBounds {
    double north;
    double south;
    double east;
    double west;
};

struct TileInfo {
    GeoBounds bounds;
    double tileSizeMeters;

    friend QDebug operator<<(QDebug dbg, const TileInfo& info) {
        dbg.nospace() << "TileInfo(tileSizeMeters=" << info.tileSizeMeters
                      << ", bounds.west=" << info.bounds.west
                      << ", bounds.east=" << info.bounds.east
                      << ", bounds.bounds.south=" << info.bounds.south
                      << ", bounds.bounds.north=" << info.bounds.north
                      << ")";
        return dbg.space();
    }
};

struct TileIndex {
    TileIndex();
    TileIndex(int32_t x, int32_t y, int32_t z, int32_t providerId);

    bool isValid() const;

    int32_t x_; // indx x
    int32_t y_; // indx y
    int32_t z_; // zoom value

    int32_t providerId_;

    std::pair<TileIndex, bool> getParent(int depth = 1) const;
    std::pair<std::vector<TileIndex>, bool> getChilds(int depth = 1) const;

    bool operator==(const TileIndex& other) const;
    bool operator!=(const TileIndex &other) const;
    bool operator<(const TileIndex& other) const;

    friend QDebug operator<<(QDebug dbg, const TileIndex& index) {
        dbg.nospace() << "TileIndex(x=" << index.x_
                      << ", y=" << index.y_
                      << ", z=" << index.z_
                      << ", providerId=" << index.providerId_
                      << ")";
        return dbg.space();
    }
};

class Tile {
public:
    enum class State {
        kNone = 0, kReady, kWaitDB, kWaitServer, kErrorServer
    };

    Tile() = default;
    explicit Tile(TileIndex index);

    void updateVertices(const LLARef& llaRef, bool isPerspective);
    bool isValid() const;

    void      setOriginTileInfo(const TileInfo& info);
    void      setModifiedTileInfo(const TileInfo& info);
    void      setState(State state);
    void      setInUse(bool val); // for tileSet
    void      setIsCopied(bool val);
    void      setImage(const QImage& image);
    void      setTextureId(GLuint textureId);
    void      setIndex(const TileIndex &index);
    void      setCreationTime(const QDateTime& val);
    void      setRequestLastTime(const QDateTime& val);
    void      setVertices(const QVector<QVector3D>& vertices);

    TileInfo  getOriginTileInfo() const;
    TileInfo  getModifiedTileInfo() const;
    State     getState() const;
    bool      getInUse() const; // for tileSet
    bool      getIsCopied() const;
    QImage    getImage() const;
    QImage&   getImageRef();
    bool      getImageIsNull() const;
    GLuint    getTextureId() const;
    TileIndex getIndex() const;
    QDateTime getCreationTime() const;
    QDateTime getRequestLastTime() const;
    LLARef    getUsedLlaRef() const;
    bool      getHasValidImage() const;

    const QVector<QVector3D>& getVerticesRef() const;

    bool operator==(const Tile& other) const;
    bool operator!=(const Tile &other) const;
    bool operator<(const Tile &other) const;

private:
    /*data*/
    TileInfo originInfo_;
    TileInfo modifiedInfo_;
    State state_;
    bool  inUse_;
    bool  copied_;
    QImage image_;
    GLuint textureId_;
    QVector<QVector3D> vertices_;
    TileIndex index_;
    QDateTime requestLastTime_;
    QDateTime creationTime_;
    LLARef usedLlaRef_;
};

inline float calculateDistance(const LLARef &llaRef1, const LLARef &llaRef2)
{
    constexpr double R = 6371000.0;

    double lat1 = qDegreesToRadians(llaRef1.refLla.latitude);
    double lon1 = qDegreesToRadians(llaRef1.refLla.longitude);
    double lat2 = qDegreesToRadians(llaRef2.refLla.latitude);
    double lon2 = qDegreesToRadians(llaRef2.refLla.longitude);

    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;

    double a = qPow(qSin(dLat / 2), 2) + qCos(lat1) * qCos(lat2) * qPow(qSin(dLon / 2), 2);
    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    double distance2D = R * c;

    double dAlt = 0;
    double distance = qSqrt(qPow(distance2D, 2) + qPow(dAlt, 2));

    return distance;
}

inline LLARef findCentralLLA(const QVector<QVector3D> &llaVertices)
{
    if (llaVertices.isEmpty()) {
        return LLARef();
    }

    QVector3D sumXYZ(0.0, 0.0, 0.0);

    for (const auto& lla : llaVertices) {
        double latRad = qDegreesToRadians(lla.x());
        double lonRad = qDegreesToRadians(lla.y());

        double x = cos(latRad) * cos(lonRad);
        double y = cos(latRad) * sin(lonRad);
        double z = sin(latRad);

        sumXYZ += QVector3D(x, y, z);
    }

    sumXYZ /= static_cast<double>(llaVertices.size());

    double length = sumXYZ.length();
    if (length == 0.0) {
        return LLARef();
    }

    QVector3D normalized = sumXYZ / length;

    double centralLat = qRadiansToDegrees(asin(normalized.z()));
    double centralLon = qRadiansToDegrees(atan2(normalized.y(), normalized.x()));

    LLA resLla(centralLat, centralLon, 0.0f);
    return LLARef(resLla);
}

inline double clampLatitude(double lat)
{
    double maxLat = 85.05112878;
    return std::max(-maxLat, std::min(maxLat, lat));
}

inline double normalizeLongitude(double lon)
{
    lon = std::fmod(lon + 180.0, 360.0);
    if (lon < 0) {
        lon += 360.0;
    }
    return lon - 180.0;
}

inline TilePosition getTilePosition(double minLon, double maxLon, const TileInfo &info)
{
    if (info.bounds.east < minLon) {
        return TilePosition::kOnLeft;
    }

    if (info.bounds.west > maxLon) {
        return TilePosition::kOnRight;
    }

    return TilePosition::kFits;
}


} // namespace map


Q_DECLARE_METATYPE(map::TileIndex)
Q_DECLARE_METATYPE(map::Tile)


namespace std {
template <>
struct hash<::map::TileIndex> {
    std::size_t operator()(const ::map::TileIndex& index) const noexcept {
        return (std::hash<int32_t>()(index.x_)              ) ^
               (std::hash<int32_t>()(index.y_)          << 1) ^
               (std::hash<int32_t>()(index.z_)          << 2) ^
               (std::hash<int32_t>()(index.providerId_) << 3);
    }
};
} // namespace std


namespace map {
inline uint qHash(const ::map::TileIndex& key, uint seed = 0) {
    std::size_t stlHash = std::hash<::map::TileIndex>()(key);
    return static_cast<uint>(stlHash ^ (seed * 0x9e3779b9));
}
} // namespace map
