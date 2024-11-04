#pragma once

#include <cstdint>
#include <QDateTime>
#include <QVector>
#include <QVector3D>
#include <QImage>
#include <QPair>
#include <QHash>
#include <functional>

#include "plotcash.h"


namespace map {


struct TileIndex {
    TileIndex() : x_(-1), y_(-1), z_(-1), providerId_(-1) {};
    TileIndex(int32_t x, int32_t y, int32_t z, int32_t providerId) :
        x_(x), y_(y), z_(z), providerId_(providerId)
    { };

    int32_t x_;
    int32_t y_;
    int32_t z_;
    int32_t providerId_;

    bool operator==(const TileIndex& other) const {
        return x_ == other.x_ &&
               y_ == other.y_ &&
               z_ == other.z_ &&
               providerId_ == other.providerId_;
    }

    bool operator!=(const TileIndex& other) const {
        return !(*this == other);
    }

    bool operator<(const TileIndex& other) const {
        if (z_ != other.z_) return z_ < other.z_;
        if (x_ != other.x_) return x_ < other.x_;
        if (y_ != other.y_) return y_ < other.y_;
        return providerId_ < other.providerId_;
    }

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

    Tile() :
        state_(State::kNone),
        inUse_(false),
        interpolated_(false)
    {};

   /* bool operator==(const Tile& other) const {
        return index_ == other.index_;
    }

    bool operator!=(const Tile& other) const {
        return !(*this == other);
    }

    bool operator<(const Tile& other) const {
        if (index_ != other.index_)
            return index_ < other.index_;
        return false;
    }*/


    void setVertexNed(const QVector3D& vertexNed) {
        vertexNed_ = vertexNed;
    }

    QVector3D getVertexNed() const {
        return vertexNed_;
    }



/*
    void setIndex(const TileIndex& index) {
        index_ = index;
    }

    TileIndex getIndex() const {
        return index_;
    }

*/



private:
    /*data*/
    State state_;
    bool  inUse_;
    bool  interpolated_;
    QImage img_;
    QVector3D vertexNed_;
   // TileIndex index_;
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
