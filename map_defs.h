#pragma once

#include <cstdint>
#include <QDateTime>
#include <QVector>
#include <QVector3D>
#include <QImage>
#include <QPair>
#include <QHash>
#include <QMetaType>
#include <functional>

#include "plotcash.h"


namespace map {


struct GeoBounds {
    double north;
    double south;
    double east;
    double west;
};

struct TileInfo {
    GeoBounds bounds;
    double tileSizeMeters;
};

struct TileIndex {
    TileIndex();;
    TileIndex(int32_t x, int32_t y, int32_t z, int32_t providerId);;

    int32_t x_; // indx x
    int32_t y_; // indx y
    int32_t z_; // zoom value

    int32_t providerId_;

    bool operator==(const TileIndex& other) const;
    bool operator!=(const TileIndex &other) const;
    bool operator<(const TileIndex& other) const;
};

inline QDebug operator<<(QDebug dbg, const TileIndex& index) {
    dbg.nospace() << "TileIndex(x=" << index.x_
                  << ", y=" << index.y_
                  << ", z=" << index.z_
                  << ", providerId=" << index.providerId_
                  << ")";
    return dbg.space();
}

class Tile {
public:
    enum class State {
        kNone = 0, kReady, kWaitDB, kWaitServer, kErrorServer
    };

    Tile(TileIndex* index = nullptr);

    void      setVertexNed(const QVector3D& vertexNed);
    void      setIndex(const TileIndex &index);
    QVector3D getVertexNed() const;
    TileIndex getIndex() const;

    bool operator==(const Tile& other) const;
    bool operator!=(const Tile &other) const;
    bool operator<(const Tile &other) const;

private:
    /*data*/
    State state_;
    bool  inUse_;
    bool  interpolated_;
    QImage img_;
    QVector3D vertexNed_;
    TileIndex index_;
    QDateTime useLastTime_;
    QDateTime requestLastTime_;
};


class TileCalculator
{
public:
    TileCalculator(float tileSize = 1.0f);
    void setTrapezoid(const QVector<QVector3D>& trapezoidVertices);
    QVector<Tile> calculateTiles() const;

private:
    bool isPointInsideTrapezoid(const QVector3D& point) const;
    QVector<QVector3D> getBoundingBox() const;

    /*data*/
    float tileSize_;
    QVector<QVector3D> trapezoid_;
};


} // namespace map

Q_DECLARE_METATYPE(map::TileIndex)

namespace std {
template <>
struct hash<::map::TileIndex> {
    std::size_t operator()(const ::map::TileIndex& index) const noexcept {
        return (std::hash<int32_t>()(index.x_)        ) ^
               (std::hash<int32_t>()(index.y_) << 1) ^
               (std::hash<int32_t>()(index.z_) << 2) ^
               (std::hash<int32_t>()(index.providerId_) << 3);
    }
};
}
