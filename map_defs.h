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

    Tile(TileIndex index);

    void updateVertices(const LLARef& llaRef);
    bool isValid() const;

    void      setTileInfo(const TileInfo& info);
    void      setState(State state);
    void      setInUse(bool val); // for tileSet
    void      setInterpolated(bool val);
    void      setImage(const QImage& image);
    void      setTextureId(GLuint textureId);
    void      setVertexNed(const QVector3D& vertexNed);
    void      setIndex(const TileIndex &index);
    void      setNeedToInit(bool state);
    void      setNeedToDeinit(bool state);
    void      setUseLastTime(const QDateTime& val);
    void      setRequestLastTime(const QDateTime& val);

    TileInfo  getTileInfo() const;
    State     getState() const;
    bool      getInUse() const; // for tileSet
    bool      getInterpolated() const;
    QImage    getImage() const;
    bool      getImageIsNull() const;
    GLuint    getTextureId() const;
    QVector3D getVertexNed() const;
    TileIndex getIndex() const;
    bool      getNeedToInit() const;
    bool      getNeedToDeinit() const;
    QDateTime getUseLastTime() const;
    QDateTime getRequestLastTime() const;

    const QVector<QVector3D>& getVerticesRef() const;
    const QVector<QVector2D>& getTexCoordsRef() const;
    const QVector<int>& getIndicesRef() const;

    bool operator==(const Tile& other) const;
    bool operator!=(const Tile &other) const;
    bool operator<(const Tile &other) const;

private:
    /*data*/
    bool needToInitT_ = false;
    bool needToDeinitT_ = false;

    TileInfo info_;

    State state_;
    bool  inUse_;
    bool  interpolated_;

    QImage image_;
    GLuint textureId_;

    QVector<QVector3D> vertices_;
    QVector<QVector2D> texCoords_;
    QVector<int> indices_;

    QVector3D vertexNed_;
    TileIndex index_;
    QDateTime useLastTime_;
    QDateTime requestLastTime_;
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
