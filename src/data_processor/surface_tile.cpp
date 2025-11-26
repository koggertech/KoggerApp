#include "surface_tile.h"

#include <cmath>


SurfaceTile::SurfaceTile() :
    id_(QUuid::createUuid()),
    origin_(QVector3D(-1.0f, -1.0f, -1.0f)),
    textureId_(0),
    isUpdated_(false),
    isInited_(false)
{}

SurfaceTile::SurfaceTile(QVector3D origin) :
    id_(QUuid::createUuid()),
    origin_(origin),
    textureId_(0),
    isUpdated_(false),
    isInited_(false)
{}

void SurfaceTile::init(int sidePixelSize, int heightMatrixRatio, float resolution)
{
    // image data
    imageData_.resize(sidePixelSize * sidePixelSize, 0);

    // height vertices
    int heightMatSideSize = heightMatrixRatio + 1;
    float heightPixelStep = (sidePixelSize / heightMatrixRatio) * resolution;
    int heightMatSize = std::pow(heightMatSideSize, 2);
    heightVertices_.resize(heightMatSize);
    heightMarkVertices_.resize(heightMatSize);
    for (int i = 0; i < heightMatSideSize; ++i) {
        for (int j = 0; j < heightMatSideSize; ++j) {
            float x = origin_.x() + j * heightPixelStep;
            float y = origin_.y() + i * heightPixelStep;
            int currIndx = i * heightMatSideSize + j;
            heightVertices_[currIndx] = QVector3D(x, y, 0.0f);
            heightMarkVertices_[currIndx] = HeightType::kUndefined;
        }
    }

    // texture vertices
    for (int i = 0; i < heightMatSideSize; ++i) {
        for (int j = 0; j < heightMatSideSize; ++j) {
            textureVertices_.append(QVector2D(float(j) / (heightMatSideSize - 1), float(i) / (heightMatSideSize - 1)));
        }
    }

    isInited_ = true;
}

void SurfaceTile::updateHeightIndices()
{
    // height indices
    heightIndices_.clear();
    int heightMatSideSize = std::sqrt(static_cast<int>(heightVertices_.size()));
    for (int i = 0; i < heightMatSideSize - 1; ++i) { // -1 для норм прохода
        for (int j = 0; j < heightMatSideSize - 1; ++j) {
            int topLeft = i * heightMatSideSize + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * heightMatSideSize + j;
            int bottomRight = bottomLeft + 1;

            if (!checkVerticesDepth(topLeft, topRight, bottomLeft, bottomRight)) {
                continue;
            }

            heightIndices_.append(topLeft);     // 1--3
            heightIndices_.append(bottomLeft);  // | /
            heightIndices_.append(topRight);    // 2
            heightIndices_.append(topRight);    //    1
            heightIndices_.append(bottomLeft);  //  / |
            heightIndices_.append(bottomRight); // 2--3
        }
    }
}

void SurfaceTile::setMosaicTextureId(GLuint val)
{
    textureId_ = val;
}

void SurfaceTile::setIsUpdated(bool state)
{
    isUpdated_ = state;
}

QUuid SurfaceTile::getUuid() const
{
    return id_;
}

QVector3D SurfaceTile::getOrigin() const
{
    return origin_;
}

bool SurfaceTile::getIsInited() const
{
    return isInited_;
}

GLuint SurfaceTile::getMosaicTextureId() const
{
    return textureId_;
}

int SurfaceTile::getIsUpdated() const
{
    return isUpdated_;
}

std::vector<uint8_t>& SurfaceTile::getMosaicImageDataRef()
{
    return imageData_;
}

const std::vector<uint8_t> &SurfaceTile::getMosaicImageDataCRef() const
{
    return imageData_;
}

QVector<QVector3D>& SurfaceTile::getHeightVerticesRef()
{
    return heightVertices_;
}

QVector<HeightType> &SurfaceTile::getHeightMarkVerticesRef()
{
    return heightMarkVertices_;
}

const QVector<QVector2D>& SurfaceTile::getMosaicTextureVerticesCRef() const
{
    return textureVertices_;
}

const QVector<QVector3D>& SurfaceTile::getHeightVerticesCRef() const
{
    return heightVertices_;
}

const QVector<int>& SurfaceTile::getHeightIndicesCRef() const
{
    return heightIndices_;
}

bool SurfaceTile::checkVerticesDepth(int topLeft, int topRight, int bottomLeft, int bottomRight) const
{
    if (qFuzzyIsNull(heightVertices_[topLeft].z())     || // someone zero
        qFuzzyIsNull(heightVertices_[topRight].z())    ||
        qFuzzyIsNull(heightVertices_[bottomLeft].z())  ||
        qFuzzyIsNull(heightVertices_[bottomRight].z())) {
        return false;
    }
    return true;
}
