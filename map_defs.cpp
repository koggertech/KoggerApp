#include "map_defs.h"

#include <QPolygonF>
#include <QPointF>
#include <cmath>


namespace map {


TileCalculator::TileCalculator(float tileSize)
    : tileSize_(tileSize)
{
}

void TileCalculator::setTrapezoid(const QVector<QVector3D>& trapezoidVertices)
{
    if (trapezoidVertices.size() != 4) {
        return;
    }
    trapezoid_ = trapezoidVertices;
}

QVector<Tile> TileCalculator::calculateTiles() const
{
    QVector<Tile> tiles;

    if (trapezoid_.size() != 4) {
        return tiles;
    }

    QVector<QVector3D> bBox = getBoundingBox();
    float minX = bBox[0].x();
    float minY = bBox[0].y();
    float maxX = bBox[1].x();
    float maxY = bBox[1].y();

    for (float x = std::floor(minX); x <= (maxX + tileSize_); x += tileSize_) {
        for (float y = std::floor(minY); y <= (maxY + tileSize_); y += tileSize_) {
            QVector3D tileCenter(x + tileSize_ / 2, y + tileSize_ / 2, 0.0f);
            if (isPointInsideTrapezoid(tileCenter)) {
                Tile tile;
                tile.setVertexNed(tileCenter);
                tiles.append(tile);
            }
        }
    }

    return tiles;
}

bool TileCalculator::isPointInsideTrapezoid(const QVector3D& point) const
{
    QPolygonF polygon;
    for (const auto& vertex : trapezoid_) {
        polygon << QPointF(vertex.x(), vertex.y());
    }

    return polygon.containsPoint(QPointF(point.x(), point.y()), Qt::OddEvenFill);
}

QVector<QVector3D> TileCalculator::getBoundingBox() const
{
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& vertex : trapezoid_) {
        if (vertex.x() < minX) minX = vertex.x();
        if (vertex.y() < minY) minY = vertex.y();
        if (vertex.x() > maxX) maxX = vertex.x();
        if (vertex.y() > maxY) maxY = vertex.y();
    }

    QVector<QVector3D> bBox;
    bBox.append(QVector3D(minX, minY, 0.0f));
    bBox.append(QVector3D(maxX, maxY, 0.0f));

    return bBox;
}

void Tile::setVertexNed(const QVector3D &vertexNed)
{
    vertexNed_ = vertexNed;
}

QVector3D Tile::getVertexNed() const
{
    return vertexNed_;
}

void Tile::setIndex(const TileIndex &index)
{
    index_ = index;
}

TileInfo Tile::getTileInfo() const
{
    return info_;
}

Tile::State Tile::getState() const
{
    return state_;
}

bool Tile::getInUse() const
{
    return inUse_;
}

bool Tile::getInterpolated() const
{
    return interpolated_;
}

QImage Tile::getImage() const
{
    return image_;
}

GLuint Tile::getTextureId() const
{
    return textureId_;
}

TileIndex Tile::getIndex() const
{
    return index_;
}

QVector<QVector3D> &Tile::getVerticesRef()
{
    return vertices_;
}

QVector<QVector2D> &Tile::getTexCoordsRef()
{
    return texCoords_;
}

QVector<int> &Tile::getIndicesRef()
{
    return indices_;
}

bool Tile::operator==(const Tile &other) const
{
    return index_ == other.index_;
}

bool Tile::operator!=(const Tile &other) const
{
    return !(*this == other);
}

bool Tile::operator<(const Tile &other) const
{
    if (index_ != other.index_) {
        return index_ < other.index_;
    }
    return false;
}

Tile::Tile(TileIndex* index) :
    state_(State::kNone),
    inUse_(false),
    interpolated_(false),
    textureId_(0)
{
    if (index) {
        index_ = *index;
    }
}

void Tile::updateVertices(const LLARef& llaRef)
{
    auto ref = llaRef;

    if (ref.isInit) {
        vertices_.clear();
        texCoords_.clear();
        indices_.clear();

        // p 1
        LLA lla1(info_.bounds.south, info_.bounds.west, 0.0f);
        NED ned1(&lla1, &ref);
        // p 2
        LLA lla2(info_.bounds.north, info_.bounds.west, 0.0f);
        NED ned2(&lla2, &ref);
        // p 3
        LLA lla3(info_.bounds.north, info_.bounds.east, 0.0f);
        NED ned3(&lla3, &ref);
        // p 4
        LLA lla4(info_.bounds.south, info_.bounds.east, 0.0f);
        NED ned4(&lla4, &ref);

/*
        //it->second.setVertexNed(QVector3D(ned.n,ned.e,0.0f));
        qDebug () << "ned1:" << ned1.n << ned1.e;
        qDebug () << "ned2:" << ned2.n << ned2.e;
        qDebug () << "ned3:" << ned3.n << ned3.e;
        qDebug () << "ned4:" << ned4.n << ned4.e;
        qDebug () << "tileInfo.tileSizeMeters:" << info_.tileSizeMeters;
*/
        // texture vertices
        vertices_ = {
            QVector3D(ned1.n, ned1.e, 0.0f),
            QVector3D(ned2.n, ned2.e, 0.0f),
            QVector3D(ned3.n, ned3.e, 0.0f),
            QVector3D(ned4.n, ned4.e, 0.0f)
        };

        texCoords_ = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        indices_ = {
            0, 1, 2,
            0, 2, 3
        };
    }
}

bool Tile::isValid() const
{
    if (index_ != TileIndex()) {
        return true;
    }
    return false;
}

void Tile::setTileInfo(const TileInfo &info)
{
    info_ = info;
}

void Tile::setState(State state)
{
    state_ = state;
}

void Tile::setInUse(bool val)
{
    inUse_ = val;
}

void Tile::setInterpolated(bool val)
{
    interpolated_ = val;
}

void Tile::setImage(const QImage &image)
{
    image_ = image;
}

void Tile::setTextureId(GLuint textureId)
{
    textureId_ = textureId;
}

TileIndex::TileIndex() :
    x_(-1),
    y_(-1),
    z_(-1),
    providerId_(-1)
{

}

TileIndex::TileIndex(int32_t x, int32_t y, int32_t z, int32_t providerId) :
    x_(x),
    y_(y),
    z_(z),
    providerId_(providerId)
{

}

bool TileIndex::operator==(const TileIndex &other) const
{
    return x_ == other.x_ &&
           y_ == other.y_ &&
           z_ == other.z_ &&
           providerId_ == other.providerId_;
}

bool TileIndex::operator!=(const TileIndex &other) const
{
    return !(*this == other);
}

bool TileIndex::operator<(const TileIndex &other) const
{
    if (z_ != other.z_) return z_ < other.z_;
    if (x_ != other.x_) return x_ < other.x_;
    if (y_ != other.y_) return y_ < other.y_;
    return providerId_ < other.providerId_;
}
} // namespace map
