#include "surface_tile.h"

#include <cmath>


SurfaceTile::SurfaceTile() :
    origin_(QVector3D(-1.0f, -1.0f, -1.0f)),
    textureId_(0),
    isUpdated_(false),
    isInited_(false),
    sidePixelSize_(defaultTileSidePixelSize),
    heightMatrixRatio_(defaultTileHeightMatrixRatio),
    resolution_(defaultTileResolution),
    updateHint_(UpdateHint::kUndefined),
    headIndx_(-1),
    inFov_(false)
{}

SurfaceTile::SurfaceTile(QVector3D origin) :
    origin_(origin),
    textureId_(0),
    isUpdated_(false),
    isInited_(false),
    sidePixelSize_(defaultTileSidePixelSize),
    heightMatrixRatio_(defaultTileHeightMatrixRatio),
    resolution_(defaultTileResolution),
    updateHint_(UpdateHint::kUndefined),
    headIndx_(-1),
    inFov_(false)
{}

SurfaceTile::SurfaceTile(const TileKey& key, QVector3D origin) :
    key_(key),
    origin_(origin),
    textureId_(0),
    isUpdated_(false),
    isInited_(false),
    sidePixelSize_(defaultTileSidePixelSize),
    heightMatrixRatio_(defaultTileHeightMatrixRatio),
    resolution_(defaultTileResolution),
    updateHint_(UpdateHint::kUndefined),
    headIndx_(-1),
    inFov_(false)
{}

void SurfaceTile::init(int sidePixelSize, int heightMatrixRatio, float resolution)
{
    // height vertices
    const int   heightMatSideSize = heightMatrixRatio + 1;
    const float heightPixelStep = (sidePixelSize / heightMatrixRatio) * resolution;
    const int   heightMatSize = std::pow(heightMatSideSize, 2);
    heightVertices_.resize(heightMatSize);
    heightMarkVertices_.resize(heightMatSize);
    for (int i = 0; i < heightMatSideSize; ++i) {
        for (int j = 0; j < heightMatSideSize; ++j) {
            const float x = origin_.x() + j * heightPixelStep;
            const float y = origin_.y() + i * heightPixelStep;
            const int   currIndx = i * heightMatSideSize + j;
            heightVertices_[currIndx]     = QVector3D(x, y, 0.0f);
            heightMarkVertices_[currIndx] = HeightType::kUndefined;
        }
    }

    sidePixelSize_     = sidePixelSize;
    heightMatrixRatio_ = heightMatrixRatio;
    resolution_        = resolution;
    isInited_          = true;
    isUpdated_         = false;

    updateHeightIndices();
}

void SurfaceTile::initImageData(int sidePixelSize, int heightMatrixRatio)
{
    const int   heightMatSideSize = heightMatrixRatio + 1;

    // image data
    imageData_.resize(sidePixelSize * sidePixelSize, 0);

    // texture vertices
    textureVertices_.clear();
    textureVertices_.reserve(heightMatSideSize * heightMatSideSize);
    for (int i = 0; i < heightMatSideSize; ++i) {
        for (int j = 0; j < heightMatSideSize; ++j) {
            textureVertices_.append(QVector2D(float(j) / (heightMatSideSize - 1),
                                              float(i) / (heightMatSideSize - 1)));
        }
    }
}

void SurfaceTile::updateHeightIndices()
{
    // height indices
    heightIndices_.clear();

    const int side = int(std::sqrt(double(heightVertices_.size())));
    if (side <= 1) return;

    for (int i = 0; i < side - 1; ++i) { // -1 для норм прохода
        for (int j = 0; j < side - 1; ++j) {
            int topLeft = i * side + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * side + j;
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

void SurfaceTile::resetInitData()
{
    textureId_ = 0;
    isUpdated_ = false;

    std::vector<uint8_t>().swap(imageData_);
    QVector<QVector3D>().swap(heightVertices_);
    QVector<HeightType>().swap(heightMarkVertices_);
    QVector<int>().swap(heightIndices_);
    QVector<QVector2D>().swap(textureVertices_);

    isInited_ = false;
    headIndx_ = -1;
}

void SurfaceTile::setOrigin(const QVector3D &val)
{
    origin_ = val;
}

void SurfaceTile::setHeadIndx(int indx)
{
    headIndx_ = indx;
}

void SurfaceTile::setUpdateHint(UpdateHint h)
{
    updateHint_ = h;
}

void SurfaceTile::setInFov(bool state)
{
    inFov_ = state;
}

UpdateHint SurfaceTile::updateHint() const
{
    return updateHint_;
}

void SurfaceTile::setMosaicTextureId(GLuint val)
{
    textureId_ = val;
}

void SurfaceTile::setIsUpdated(bool state)
{
    isUpdated_ = state;
}

const TileKey &SurfaceTile::getKey() const
{
    return key_;
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

const QVector<HeightType> &SurfaceTile::getHeightMarkVerticesCRef() const
{
    return heightMarkVertices_;
}

const QVector<int>& SurfaceTile::getHeightIndicesCRef() const
{
    return heightIndices_;
}

int SurfaceTile::getHeadIndx() const
{
    return headIndx_;
}

bool SurfaceTile::getInFov() const
{
    return inFov_;
}

int SurfaceTile::sidePixelSize() const
{
    return sidePixelSize_;
}

int SurfaceTile::heightMatrixRatio() const
{
    return heightMatrixRatio_;
}

float SurfaceTile::resolution() const
{
    return resolution_;
}

float SurfaceTile::sideWorld() const
{
    return sidePixelSize_ * resolution_;
}

void SurfaceTile::footprint(float &x0, float &x1, float &y0, float &y1) const
{
    const float s = sideWorld();
    x0 = origin_.x();
    y0 = origin_.y();
    x1 = x0 + s;
    y1 = y0 + s;
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
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
