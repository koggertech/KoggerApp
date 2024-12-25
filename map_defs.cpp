#include "map_defs.h"

#include <QPolygonF>
#include <QPointF>
#include <cmath>


namespace map {

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

void Tile::setNeedToInit(bool state)
{
    needToInitT_ = state;
}

void Tile::setNeedToDeinit(bool state)
{
    needToDeinitT_ = state;
}

void Tile::setCreationTime(const QDateTime &val)
{
    creationTime_ = val;
}

void Tile::setRequestLastTime(const QDateTime &val)
{
    requestLastTime_ = val;
}

void Tile::setVertices(const QVector<QVector3D> &vertices)
{
    vertices_ = vertices;
}

TileInfo Tile::getOriginTileInfo() const
{
    return originInfo_;
}

TileInfo Tile::getModifiedTileInfo() const
{
    return modifiedInfo_;
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

QImage& Tile::getImageRef()
{
    return image_;
}

bool Tile::getImageIsNull() const
{
    return image_.isNull();
}

GLuint Tile::getTextureId() const
{
    return textureId_;
}

TileIndex Tile::getIndex() const
{
    return index_;
}

bool Tile::getNeedToInit() const
{
    return needToInitT_;
}

bool Tile::getNeedToDeinit() const
{
    return needToDeinitT_;
}

QDateTime Tile::getCreationTime() const
{
    return creationTime_;
}

QDateTime Tile::getRequestLastTime() const
{
    return requestLastTime_;
}

LLARef Tile::getUsedLlaRef() const
{
    return usedLlaRef_;
}

const QVector<QVector3D> &Tile::getVerticesRef() const
{
    return vertices_;
}

const QVector<QVector2D> &Tile::getTexCoordsRef() const
{
    return texCoords_;
}

const QVector<int> &Tile::getIndicesRef() const
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

Tile::Tile(TileIndex index) :
    state_(State::kNone),
    inUse_(false),
    interpolated_(false),
    textureId_(0),
    index_(index),
    creationTime_(QDateTime::currentDateTimeUtc())
{

}

void Tile::updateVertices(const LLARef& llaRef,  bool isPerspective)
{
    auto ref = llaRef;

    if (ref.isInit) {
        vertices_.clear();
        texCoords_.clear();
        indices_.clear();

        //   4 <--- 3
        //          |
        //          |
        //   1 ---> 2

        LLA lla1(modifiedInfo_.bounds.south, modifiedInfo_.bounds.west, 0.0f);
        NED ned1(&lla1, &ref, isPerspective);
        LLA lla2(modifiedInfo_.bounds.north, modifiedInfo_.bounds.west, 0.0f);
        NED ned2(&lla2, &ref, isPerspective);
        LLA lla3(modifiedInfo_.bounds.north, modifiedInfo_.bounds.east, 0.0f);
        NED ned3(&lla3, &ref, isPerspective);
        LLA lla4(modifiedInfo_.bounds.south, modifiedInfo_.bounds.east, 0.0f);
        NED ned4(&lla4, &ref, isPerspective);

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

        usedLlaRef_ = ref;
    }
}

bool Tile::isValid() const
{
    if (index_ != TileIndex()) {
        return true;
    }
    return false;
}

void Tile::setOriginTileInfo(const TileInfo &info)
{
    originInfo_ = info;
}

void Tile::setModifiedTileInfo(const TileInfo &info)
{
    modifiedInfo_ = info;
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

bool TileIndex::isValid() const
{
    return !(x_ == -1 || y_ == -1 || z_ == -1 || providerId_ == -1);
}

std::pair<TileIndex, bool> TileIndex::getParent(int depth) const
{
    if (depth < 0) {
        depth = 0;
    }

    int targetZ = z_ - depth;

    if (targetZ < 1) {
        return {*this, false};
    }

    int shift = depth;

    TileIndex parent{
        x_ >> shift,
        y_ >> shift,
        targetZ,
        providerId_
    };

    return { parent, true };
}

std::pair<std::vector<TileIndex>, bool> TileIndex::getChilds(int depth) const
{
    if (depth < 1) {
        depth = 1;
    }

    int targetZ = z_ + depth;

    if (targetZ > 21) {
        return {{}, false};
    }

    std::vector<TileIndex> children = {
        TileIndex{x_ * 2,     y_ * 2,     z_ + 1, providerId_},
        TileIndex{x_ * 2 + 1, y_ * 2,     z_ + 1, providerId_},
        TileIndex{x_ * 2,     y_ * 2 + 1, z_ + 1, providerId_},
        TileIndex{x_ * 2 + 1, y_ * 2 + 1, z_ + 1, providerId_}
    };

    for (int currentDepth = 2; currentDepth <= depth; ++currentDepth) {
        std::vector<TileIndex> nextGeneration;
        for (const auto& child : children) {
            nextGeneration.push_back(TileIndex{child.x_ * 2,     child.y_ * 2,     child.z_ + 1, providerId_});
            nextGeneration.push_back(TileIndex{child.x_ * 2 + 1, child.y_ * 2,     child.z_ + 1, providerId_});
            nextGeneration.push_back(TileIndex{child.x_ * 2,     child.y_ * 2 + 1, child.z_ + 1, providerId_});
            nextGeneration.push_back(TileIndex{child.x_ * 2 + 1, child.y_ * 2 + 1, child.z_ + 1, providerId_});
        }
        children = std::move(nextGeneration);
    }

    return { children, true };
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

void Tile::setPendingRemoval(bool value)
{
    pendingRemoval_ = value;
}

bool Tile::getPendingRemoval() const
{
    return pendingRemoval_;
}

bool Tile::getHasValidImage() const
{
    return (!image_.isNull() && !interpolated_);
}


} // namespace map
