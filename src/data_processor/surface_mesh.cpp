#include "surface_mesh.h"

#include <cmath>


SurfaceMesh::SurfaceMesh(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution) :
    tileResolution_(tileResolution),
    numWidthTiles_(0),
    numHeightTiles_(0),
    tileSidePixelSize_(tileSidePixelSize),
    tileHeightMatrixRatio_(tileHeightMatrixRatio)
{
    tileSideMeterSize_ = tileSidePixelSize_ * tileResolution_;
}

SurfaceMesh::~SurfaceMesh()
{}

void SurfaceMesh::reinit(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
{
    clear();

    tileSidePixelSize_ = tileSidePixelSize;
    tileHeightMatrixRatio_ = tileHeightMatrixRatio;
    tileResolution_ = tileResolution;

    tileSideMeterSize_ = tileSidePixelSize_ * tileResolution_;
}

bool SurfaceMesh::concatenate(kmath::MatrixParams &actualMatParams)
{
    if (!actualMatParams.isValid()) {
        return false;
    }

    if (tileMatrix_.empty()) {
        int newNumWidthTiles = std::ceil(actualMatParams.width * 1.0f / tileSideMeterSize_);
        int newNumHeightTiles = std::ceil(actualMatParams.height * 1.0f / tileSideMeterSize_);

        initializeMatrix(newNumWidthTiles, newNumHeightTiles, actualMatParams);

        return true;
    }

    bool resized = false;

    int columnsToAddLeft = 0, columnsToAddRight = 0, rowsToAddTop = 0, rowsToAddBottom = 0;
    if (actualMatParams.originX < origin_.x()) {
        columnsToAddLeft = std::ceil((origin_.x() - actualMatParams.originX) / tileSideMeterSize_);
    }
    if ((actualMatParams.originX + actualMatParams.width) > (origin_.x() + getWidthMeters())) {
        columnsToAddRight = std::ceil(((actualMatParams.originX + actualMatParams.width) - (origin_.x() + getWidthMeters())) / tileSideMeterSize_);
    }
    if ((actualMatParams.originY + actualMatParams.height) > (origin_.y() + getHeightMeters()) ) {
        rowsToAddTop = std::ceil(((actualMatParams.originY + actualMatParams.height) - (origin_.y() + getHeightMeters())) / tileSideMeterSize_);
    }
    if (actualMatParams.originY < origin_.y()) {
        rowsToAddBottom = std::ceil((origin_.y() - actualMatParams.originY) / tileSideMeterSize_);
    }


    if (columnsToAddLeft > 0) {
        origin_.setX(origin_.x() - columnsToAddLeft * tileSideMeterSize_);
        resizeColumnsLeft(columnsToAddLeft);
        resized = true;
    }
    if (rowsToAddBottom > 0) {
        origin_.setY(origin_.y() - rowsToAddBottom * tileSideMeterSize_);
        resizeRowsBottom(rowsToAddBottom);
        resized = true;
    }

    if (columnsToAddRight > 0) {
        resizeColumnsRight(columnsToAddRight);
        resized = true;
    }
    if (rowsToAddTop > 0) {
        resizeRowsTop(rowsToAddTop);
        resized = true;
    }

    return resized;
}

QVector3D SurfaceMesh::convertPhToPixCoords(QVector3D physicsCoordinate) const
{
    return QVector3D((std::fabs(origin_.x() - physicsCoordinate.x())) / tileResolution_,
                     (std::fabs(origin_.y() - physicsCoordinate.y())) / tileResolution_,
                     0);
}

void SurfaceMesh::printMatrix() const
{
    qDebug() << "origin:" << origin_;
    qDebug() << "tiles (WxH): " << numWidthTiles_ << "x" << numHeightTiles_;;

    for (auto& itmI : tileMatrix_) {
        QString rowOutput;
        for (auto& itmJ : itmI) {
            if (itmJ) {
                auto tileOrigin = itmJ->getOrigin();
                rowOutput += QString("[" + QString::number(tileOrigin.x(), 'f', 0) +
                                     "x" +  QString::number(tileOrigin.y(), 'f', 0) + "]").rightJustified(10);
            }
            else {
                rowOutput += QString("[     ]").rightJustified(5);
            }
        }
        qDebug() << rowOutput;
    }
    qDebug() << "\n";
}

void SurfaceMesh::clear()
{
    origin_ = QVector3D();
    for (auto& itm: tiles_) {
        delete itm;
    }
    tiles_.clear();
    tileMatrix_.clear();

    numWidthTiles_ = 0;
    numHeightTiles_ = 0;
}

void SurfaceMesh::clearHeightData(HeightType heightType)
{
    if (tiles_.empty()) {
        return;
    }

    for (SurfaceTile* tile : tiles_){
        if (!tile || !tile->getIsInited()) {
            continue;
        }

        auto& verts = tile->getHeightVerticesRef();
        auto& marks = tile->getHeightMarkVerticesRef();

        for (int i = 0; i < verts.size(); ++i) {
            if (heightType  == HeightType::kUndefined ||
                marks[i] == heightType) {
                verts[i][2] = 0.0f;
                marks[i]    = HeightType::kUndefined;
            }
        }

        tile->updateHeightIndices();
    }
}

bool SurfaceMesh::hasData() const
{
    return !tiles_.empty();
}

const std::vector<SurfaceTile *> &SurfaceMesh::getTilesCRef() const
{
    return tiles_;
}

std::vector<std::vector<SurfaceTile *> > &SurfaceMesh::getTileMatrixRef()
{
    return tileMatrix_;
}

SurfaceTile* SurfaceMesh::getTilePtrById(QUuid tileId)
{
    for (auto& itm : tiles_) {
        if (itm->getUuid() == tileId) {
            return itm;
        }
    }
    return nullptr;
}

int SurfaceMesh::getPixelWidth() const
{
    return numWidthTiles_ * tileSidePixelSize_;
}

int SurfaceMesh::getPixelHeight() const
{
    return numHeightTiles_ * tileSidePixelSize_;
}

int SurfaceMesh::getTileSidePixelSize() const
{
    return tileSidePixelSize_;
}

int SurfaceMesh::getNumWidthTiles() const
{
    return numWidthTiles_;
}

int SurfaceMesh::getNumHeightTiles() const
{
    return numHeightTiles_;
}

int SurfaceMesh::getStepSizeHeightMatrix() const
{
    return tileSidePixelSize_ / tileHeightMatrixRatio_;
}

bool SurfaceMesh::getIsInited() const
{
    return !tiles_.empty();
}

void SurfaceMesh::initializeMatrix(int numWidthTiles, int numHeightTiles, const kmath::MatrixParams &matrixParams)
{
    numWidthTiles_ = numWidthTiles;
    numHeightTiles_ = numHeightTiles;

    origin_ = QVector3D(matrixParams.originX, matrixParams.originY, 0);

    tileMatrix_.resize(numHeightTiles_);
    for (int i = 0; i < numHeightTiles_; ++i) {
        tileMatrix_[i].resize(numWidthTiles_);
    }

    for (int i = 0; i < numHeightTiles_; ++i) {
        for (int j = 0; j < numWidthTiles_; ++j) {

            if (!tileMatrix_[i][j]) {
                tiles_.push_back(new SurfaceTile( { origin_.x() + j * tileSideMeterSize_,
                                           origin_.y() + ((numHeightTiles_ - 1) - i) * tileSideMeterSize_, 0.0f }));
                tileMatrix_[i][j] = tiles_.back();
            }
        }
    }
}

void SurfaceMesh::resizeColumnsLeft(int columnsToAdd)
{
    for (int i = 0; i < static_cast<int>(tileMatrix_.size()); ++i) {
        int oldSize = tileMatrix_[i].size();
        tileMatrix_[i].resize(oldSize + columnsToAdd);

        for (int j = oldSize - 1; j >= 0; --j) {
            tileMatrix_[i][j + columnsToAdd] = tileMatrix_[i][j];
        }

        for (int j = 0; j < columnsToAdd; ++j) {
            tiles_.push_back(new SurfaceTile({ origin_.x() + j * tileSideMeterSize_,
                                       origin_.y() + ((numHeightTiles_ - 1) - i) * tileSideMeterSize_, 0.0f }));
            tileMatrix_[i][j] = tiles_.back();
        }
    }

    numWidthTiles_ += columnsToAdd;
}

void SurfaceMesh::resizeRowsBottom(int rowsToAdd)
{
    int oldHeight = tileMatrix_.size();
    tileMatrix_.resize(oldHeight + rowsToAdd);

    int cnt = 0;
    for (int i = oldHeight; i < static_cast<int>(tileMatrix_.size()); ++i) {
        tileMatrix_[i].resize(numWidthTiles_);

        for (int j = 0; j < numWidthTiles_; ++j) {
            tiles_.push_back(new SurfaceTile({ origin_.x() + j * tileSideMeterSize_,
                                       origin_.y() + (rowsToAdd - cnt - 1) * tileSideMeterSize_, 0.0f }));
            tileMatrix_[i][j] = tiles_.back();
        }
        cnt++;
    }

    numHeightTiles_ += rowsToAdd;
}

void SurfaceMesh::resizeColumnsRight(int columnsToAdd)
{
    int oldNumWidthTiles = numWidthTiles_;

    for (int i = 0; i < static_cast<int>(tileMatrix_.size()); ++i) {
        tileMatrix_[i].resize(oldNumWidthTiles + columnsToAdd);

        for (int j = oldNumWidthTiles; j < oldNumWidthTiles + columnsToAdd; ++j) {
            tiles_.push_back(new SurfaceTile({ origin_.x() + j * tileSideMeterSize_,
                                       origin_.y() + ((numHeightTiles_ - 1) - i) * tileSideMeterSize_, 0.0f }));
            tileMatrix_[i][j] = tiles_.back();
        }
    }

    numWidthTiles_ += columnsToAdd;
}

void SurfaceMesh::resizeRowsTop(int rowsToAdd)
{
    tileMatrix_.resize(numHeightTiles_ + rowsToAdd);

    for (int i = tileMatrix_.size() - 1; i >= rowsToAdd; --i) {
        tileMatrix_[i] = tileMatrix_[i - rowsToAdd];
    }

    for (int i = 0; i < rowsToAdd; ++i) {

        tileMatrix_[i].resize(numWidthTiles_);

        for (int j = 0; j < numWidthTiles_; ++j) {
            tiles_.push_back(new SurfaceTile({ origin_.x() + j * tileSideMeterSize_,
                                       origin_.y() + ((numHeightTiles_ + rowsToAdd - 1) - i) * tileSideMeterSize_, 0.0f }));
            tileMatrix_[i][j] = tiles_.back();
        }
    }

    numHeightTiles_ += rowsToAdd;
}

float SurfaceMesh::getWidthMeters() const
{
    return numWidthTiles_ * tileSideMeterSize_;
}

float SurfaceMesh::getHeightMeters() const
{
    return numHeightTiles_ * tileSideMeterSize_;
}
