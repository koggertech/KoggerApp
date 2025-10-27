#include "surface_mesh.h"

#include <cmath>


//inline int tileIndexFromCoord(float coordMeters, float tileSideMeters)
//{
//    return floor_div(int(std::floor(coordMeters)), int(std::floor(tileSideMeters)));
//
//}



SurfaceMesh::SurfaceMesh(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution) :
    tileResolution_(tileResolution),
    numWidthTiles_(0),
    numHeightTiles_(0),
    tileSidePixelSize_(tileSidePixelSize),
    tileHeightMatrixRatio_(tileHeightMatrixRatio),
    tick_(1),
    maxTrackedTiles_(0),
    initedCount_(0)
{
    tileSideMeterSize_ = tileSidePixelSize_ * tileResolution_;
}

SurfaceMesh::~SurfaceMesh()
{
    clear();
}

void SurfaceMesh::reinit(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
{
    clear();

    tileSidePixelSize_ = tileSidePixelSize;
    tileHeightMatrixRatio_ = tileHeightMatrixRatio;
    tileResolution_ = tileResolution;
    tileSideMeterSize_ = tileSidePixelSize_ * tileResolution_;

    zoomIndex_ = zoomFromMpp(tileResolution_);
    //qDebug() << "SurfaceMesh::reinit res" << tileResolution_ << "AND ZOOM INDX" << zoomIndex_;
}

bool SurfaceMesh::concatenate(kmath::MatrixParams &actualMatParams)
{
    if (!actualMatParams.isValid()) {
        return false;
    }

    if (tileMatrix_.empty()) {
        const int baseTx = static_cast<int>(std::floor(actualMatParams.originX / tileSideMeterSize_));
        const int baseTy = static_cast<int>(std::floor(actualMatParams.originY / tileSideMeterSize_));
        origin_.setX(baseTx * tileSideMeterSize_);
        origin_.setY(baseTy * tileSideMeterSize_);

        const float right  = actualMatParams.originX + actualMatParams.width;
        const float top    = actualMatParams.originY + actualMatParams.height;
        const int newNumWidthTiles  = static_cast<int>(std::ceil((right  - origin_.x()) / tileSideMeterSize_));
        const int newNumHeightTiles = static_cast<int>(std::ceil((top    - origin_.y()) / tileSideMeterSize_));

        initializeMatrix(newNumWidthTiles, newNumHeightTiles);
        return true;
    }

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

    bool resized = false;
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
    return QVector3D((physicsCoordinate.x() - origin_.x()) / tileResolution_,
                     (physicsCoordinate.y() - origin_.y()) / tileResolution_,
                     0.0f);
}

void SurfaceMesh::printMatrix() const
{
    qDebug() << "origin:" << origin_;
    qDebug() << "tiles (WxH): " << numWidthTiles_ << "x" << numHeightTiles_ << "size" << tiles_.size();
    qDebug() << "resol:" << tileResolution_ << "zomInxd:" << zoomIndex_;

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
    tileByKey_.clear();

    lru_.clear();
    initedKeys_.clear();
    tick_ = 1;
    initedCount_ = 0;

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

SurfaceTile *SurfaceMesh::getTilePtrByKey(const TileKey &key)
{
    auto it = tileByKey_.find(key);
    return it == tileByKey_.end() ? nullptr : it.value();
}

int SurfaceMesh::getCurrentZoom() const
{
    return zoomIndex_;
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

void SurfaceMesh::setMaxInitedTiles(int maxTiles)
{
    maxTrackedTiles_ = std::max(0, maxTiles);
    evictIfNeeded();
}

int SurfaceMesh::getMaxInitedTiles() const
{
    return maxTrackedTiles_;
}

int SurfaceMesh::currentInitedTiles() const
{
    return initedCount_;
}

int SurfaceMesh::scanInitedTiles()
{
    int total = 0;
    int hSize = getNumHeightTiles();
    int wSize = getNumWidthTiles();

    for (int y = 0; y < hSize; ++y) {
        for (int x = 0; x < wSize; ++x) {
            if (auto* t = tileMatrix_[y][x]; t && t->getIsInited()) {
                ++total;
            }
        }
    }

    return total;
}

void SurfaceMesh::onTilesWritten(const QSet<SurfaceTile*>& written)
{
    for (auto* t : written) {
        if (!t) {
            continue;
        }

        const TileKey key = t->getKey();
        touch(key);

        if (t->getIsInited() && !initedKeys_.contains(key)) {
            initedKeys_.insert(key);
            ++initedCount_;
        }
    }

    evictIfNeeded();
}

void SurfaceMesh::touch(const TileKey &key)
{
    auto& m = lru_[key];
    m = ++tick_;
}

void SurfaceMesh::resetTileByKey(const TileKey& key)
{
    if (auto* t = getTilePtrByKey(key)) {
        const bool wasInited = t->getIsInited();
        t->resetInitData();

        if (wasInited && !t->getIsInited()) {
            if (initedKeys_.remove(key) > 0) {
                initedCount_ = std::max(0, initedCount_ - 1);
            }
        }

        lru_.remove(key);
        //qDebug() << "reset" << key.x << key.y << key.zoom;
    }
}

void SurfaceMesh::evictIfNeeded() // ужать до лимита
{
    if (maxTrackedTiles_ <= 0) { // без лимита
        return;
    }

    while (initedCount_ > maxTrackedTiles_) {
        TileKey victim;
        uint64_t best = std::numeric_limits<uint64_t>::max();

        for (auto it = lru_.cbegin(); it != lru_.cend(); ++it) { // ищем старейший
            auto* t = getTilePtrByKey(it.key());

            if (!t || !t->getIsInited()) {
                continue;
            }

            if (it.value() < best) {
                best = it.value();
                victim = it.key();
            }
        }

        if (best == std::numeric_limits<uint64_t>::max()) {
            break;
        }

        resetTileByKey(victim);
    }
}

void SurfaceMesh::initializeMatrix(int numWidthTiles, int numHeightTiles)
{
    numWidthTiles_  = numWidthTiles;
    numHeightTiles_ = numHeightTiles;

    const int baseTx = tileIndexFromCoord(origin_.x(), tileSideMeterSize_);
    const int baseTy = tileIndexFromCoord(origin_.y(), tileSideMeterSize_);
    origin_.setX(baseTx * tileSideMeterSize_);
    origin_.setY(baseTy * tileSideMeterSize_);

    tileMatrix_.assign(numHeightTiles_, std::vector<SurfaceTile*>(numWidthTiles_, nullptr));

    for (int i = 0; i < numHeightTiles_; ++i) {
        for (int j = 0; j < numWidthTiles_; ++j) {
            const QVector3D tileOrigin{
                origin_.x() + j * tileSideMeterSize_,
                origin_.y() + ((numHeightTiles_ - 1) - i) * tileSideMeterSize_,
                0.0f
            };

            const int tx = baseTx + j;
            const int ty = baseTy + ((numHeightTiles_ - 1) - i);
            const TileKey key{ tx, ty, zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);

            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
            registerNewTile(tile);
        }
    }
}

void SurfaceMesh::resizeColumnsLeft(int columnsToAdd)
{
    if (columnsToAdd <= 0) {
        return;
    }
;
    const int baseTx = tileIndexFromCoord(origin_.x(), tileSideMeterSize_);
    const int baseTy = tileIndexFromCoord(origin_.y(), tileSideMeterSize_);

    for (int i = 0; i < static_cast<int>(tileMatrix_.size()); ++i) {
        const int oldSize = static_cast<int>(tileMatrix_[i].size());
        tileMatrix_[i].resize(oldSize + columnsToAdd);

        for (int j = oldSize - 1; j >= 0; --j) { // сдвиг вправо
            tileMatrix_[i][j + columnsToAdd] = tileMatrix_[i][j];
        }

        for (int j = 0; j < columnsToAdd; ++j) { // новые слева
            const QVector3D tileOrigin{
                origin_.x() + j * tileSideMeterSize_,
                origin_.y() + ((numHeightTiles_ - 1) - i) * tileSideMeterSize_,
                0.0f
            };

            const int tx = baseTx + j;
            const int ty = baseTy + ((numHeightTiles_ - 1) - i);
            const TileKey key{ tx, ty, zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);
            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
            registerNewTile(tile);
        }
    }

    numWidthTiles_ += columnsToAdd;
}

void SurfaceMesh::resizeRowsBottom(int rowsToAdd)
{
    if (rowsToAdd <= 0) {
        return;
    }

    const int baseTx = tileIndexFromCoord(origin_.x(), tileSideMeterSize_);
    const int baseTy = tileIndexFromCoord(origin_.y(), tileSideMeterSize_);
    const int oldH = static_cast<int>(tileMatrix_.size());
    const int newH = numHeightTiles_ + rowsToAdd;

    tileMatrix_.resize(oldH + rowsToAdd);

    for (int i = oldH; i < oldH + rowsToAdd; ++i) {
        tileMatrix_[i].resize(numWidthTiles_);

        for (int j = 0; j < numWidthTiles_; ++j) {
            const QVector3D tileOrigin{
                origin_.x() + j * tileSideMeterSize_,
                origin_.y() + ((newH - 1) - i) * tileSideMeterSize_,
                0.0f
            };

            const int tx = baseTx + j;
            const int ty = baseTy + ((newH - 1) - i);
            const TileKey key{ tx, ty, zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);
            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
            registerNewTile(tile);
        }
    }

    numHeightTiles_ = newH;
}

void SurfaceMesh::resizeColumnsRight(int columnsToAdd)
{
    if (columnsToAdd <= 0) {
        return;
    }

    const int baseTx = tileIndexFromCoord(origin_.x(), tileSideMeterSize_);
    const int baseTy = tileIndexFromCoord(origin_.y(), tileSideMeterSize_);
    const int oldW = numWidthTiles_;

    for (int i = 0; i < static_cast<int>(tileMatrix_.size()); ++i) {
        tileMatrix_[i].resize(oldW + columnsToAdd);

        for (int j = oldW; j < oldW + columnsToAdd; ++j) {
            const QVector3D tileOrigin{
                origin_.x() + j * tileSideMeterSize_,
                origin_.y() + ((numHeightTiles_ - 1) - i) * tileSideMeterSize_,
                0.0f
            };

            const int tx = baseTx + j;
            const int ty = baseTy + ((numHeightTiles_ - 1) - i);
            const TileKey key{ tx, ty, zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);
            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
            registerNewTile(tile);
        }
    }

    numWidthTiles_ += columnsToAdd;
}

void SurfaceMesh::resizeRowsTop(int rowsToAdd)
{
    if (rowsToAdd <= 0) {
        return;
    }

    const int baseTx = tileIndexFromCoord(origin_.x(), tileSideMeterSize_);
    const int baseTy = tileIndexFromCoord(origin_.y(), tileSideMeterSize_);
    const int newH = numHeightTiles_ + rowsToAdd;

    tileMatrix_.resize(newH); // расширяем и сдвигаем вниз
    for (int i = newH - 1; i >= rowsToAdd; --i) {
        tileMatrix_[i] = std::move(tileMatrix_[i - rowsToAdd]);
    }

    for (int i = 0; i < rowsToAdd; ++i) { // новые строки сверху
        tileMatrix_[i].resize(numWidthTiles_);

        for (int j = 0; j < numWidthTiles_; ++j) {
            const QVector3D tileOrigin{
                origin_.x() + j * tileSideMeterSize_,
                origin_.y() + ((newH - 1) - i) * tileSideMeterSize_,
                0.0f
            };

            const int tx = baseTx + j;
            const int ty = baseTy + ((newH - 1) - i);
            const TileKey key{ tx, ty, zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);
            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
            registerNewTile(tile);
        }
    }

    numHeightTiles_ = newH;
}

float SurfaceMesh::getWidthMeters() const
{
    return numWidthTiles_ * tileSideMeterSize_;
}

float SurfaceMesh::getHeightMeters() const
{
    return numHeightTiles_ * tileSideMeterSize_;
}

void SurfaceMesh::registerNewTile(SurfaceTile *t)
{
    if (!t) {
        return;
    }

    //qDebug() << "REGISTER TILE" << t->getKey();

    lru_.insert(t->getKey(), 0);
}
