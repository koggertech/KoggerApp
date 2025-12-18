#include "surface_mesh.h"

#include <QDateTime>
#include <cmath>


SurfaceMesh::SurfaceMesh(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution) :
    tileResolution_(tileResolution),
    invRes_(1.0f / tileResolution_),
    numWidthTiles_(0),
    numHeightTiles_(0),
    tileSidePixelSize_(tileSidePixelSize),
    tileHeightMatrixRatio_(tileHeightMatrixRatio),
    highWM_(512),
    lowWM_(256)
{
    tileSideMeterSize_ = tileSidePixelSize_ * tileResolution_;
}

SurfaceMesh::~SurfaceMesh()
{
    clear();
}

QSet<SurfaceTile*> SurfaceMesh::getUpdatedTiles() const
{
    QSet<SurfaceTile*> retVal;
    retVal.reserve(tiles_.size());

    for (auto& itm : tiles_) {
        if (itm->getIsUpdated()) {
            retVal.insert(itm /*->getKey()*/);
        }
    }

    return retVal;
}

void SurfaceMesh::reinit(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
{
    clear();

    tileSidePixelSize_ = tileSidePixelSize;
    tileHeightMatrixRatio_ = tileHeightMatrixRatio;
    tileResolution_ = tileResolution;
    invRes_ = 1.0f / tileResolution_;

    tileSideMeterSize_ = tileSidePixelSize_ * tileResolution_;

    zoomIndex_ = zoomFromMpp(tileResolution_);
    //qDebug() << "SurfaceMesh::reinit res" << tileResolution_ << "AND ZOOM INDX" << zoomIndex_;
}

bool SurfaceMesh::concatenate(kmath::MatrixParams &actualMatParams)
{
    if (!actualMatParams.isValid()) {
        return false;
    }

    const double S = double(tileSideMeterSize_);

    auto ceilTiles = [&](double delta)->int {
        if (delta <= 0.0) return 0;
        const double epsS = S * 1e-6;
        return int(std::floor((delta + epsS) / S));
    };

    if (tileMatrix_.empty()) {
        const int baseTx = int(std::floor(actualMatParams.originX / S));
        const int baseTy = int(std::floor(actualMatParams.originY / S));
        origin_.setX(baseTx * S);
        origin_.setY(baseTy * S);

        const double right  = actualMatParams.originX + actualMatParams.width;
        const double top    = actualMatParams.originY + actualMatParams.height;
        const int newNumWidthTiles  = int(std::ceil((right  - origin_.x()) / S));
        const int newNumHeightTiles = int(std::ceil((top    - origin_.y()) / S));

        initializeMatrix(newNumWidthTiles, newNumHeightTiles);
        return true;
    }

    const double currRight = origin_.x() + double(getWidthMeters());
    const double currTop   = origin_.y() + double(getHeightMeters());

    const int columnsToAddLeft  = ceilTiles(origin_.x() - actualMatParams.originX);
    const int columnsToAddRight = ceilTiles((actualMatParams.originX + actualMatParams.width)  - currRight);
    const int rowsToAddTop      = ceilTiles((actualMatParams.originY + actualMatParams.height) - currTop);
    const int rowsToAddBottom   = ceilTiles(origin_.y() - actualMatParams.originY);

    bool resized = false;

    if (columnsToAddLeft > 0) {
        origin_.setX(origin_.x() - columnsToAddLeft * S);
        resizeColumnsLeft(columnsToAddLeft);
        resized = true;
    }
    if (rowsToAddBottom > 0) {
        origin_.setY(origin_.y() - rowsToAddBottom * S);
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
    tiles_.shrink_to_fit();

    for (auto& itm : tileMatrix_) {
        itm.clear();
        itm.shrink_to_fit();
    }
    tileMatrix_.clear();
    tileMatrix_.shrink_to_fit();

    tileByKey_.clear();
    lru_.clear();

    numWidthTiles_ = 0;
    numHeightTiles_ = 0;

    baseTx_ = 0;
    baseTy_ = 0;
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

const SurfaceTile *SurfaceMesh::getTileCPtrByKey(const TileKey &key) const
{
    auto it = tileByKey_.constFind(key);
    return it == tileByKey_.cend() ? nullptr : it.value();
}

std::pair<bool, SurfaceTile> SurfaceMesh::getTileCopyByKey(const TileKey &key) const
{
    SurfaceTile retVal;
    auto it = tileByKey_.find(key);

    if  (it != tileByKey_.end()) {
        return std::make_pair(true, *it.value());
    }
    return std::make_pair(false, retVal);
}

int SurfaceMesh::getCurrentZoom() const
{
    return zoomIndex_;
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

void SurfaceMesh::setLRUWatermarks(int highA, int lowB)
{
    highWM_ = std::max(0, highA);
    lowWM_  = std::max(0, std::min(lowB, highWM_));

    evictIfNeeded();
}

int SurfaceMesh::currentInitedTiles() const
{
    return lru_.size();
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

void SurfaceMesh::setTileUsed(const QSet<SurfaceTile*>& written, bool evict)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (auto* t : written) {
        if (!t || !t->getIsInited()) {
            continue;
        }

        lru_[t->getKey()] = now;
    }

    if (evict) {
        evictIfNeeded();
    }
}

void SurfaceMesh::touch(const TileKey &key)
{
    if (auto* t = getTilePtrByKey(key); t && t->getIsInited()) {
        lru_[key] = QDateTime::currentMSecsSinceEpoch();
    }
}

void SurfaceMesh::resetTileByKey(const TileKey& key)
{
    if (auto* t = getTilePtrByKey(key)) {
        t->resetInitData();
        lru_.remove(key);

        //qDebug() << "reset" << key.x << key.y << key.zoom;
    }
}

void SurfaceMesh::evictIfNeeded() // ужать до лимита
{
    if (highWM_ <= 0 || lru_.size() <= highWM_) {
        return;
    }
    while (lru_.size() > lowWM_) {
        TileKey victim;
        qint64 oldest = std::numeric_limits<qint64>::max();
        for (auto it = lru_.cbegin(); it != lru_.cend(); ++it) {
            auto* t = getTilePtrByKey(it.key());
            if (!t || !t->getIsInited()) {
                continue;
            }
            if (it.value() < oldest) {
                oldest = it.value();
                victim = it.key();
            }
        }
        if (oldest == std::numeric_limits<qint64>::max()) {
            break;
        }
        resetTileByKey(victim);
    }
}

void SurfaceMesh::initializeMatrix(int newW, int newH)
{
    numWidthTiles_  = newW;
    numHeightTiles_ = newH;

    const double S = double(tileSideMeterSize_);

    baseTx_ = tileIndexFromCoord(origin_.x(), S);
    baseTy_ = tileIndexFromCoord(origin_.y(), S);

    origin_.setX(float(baseTx_ * S));
    origin_.setY(float(baseTy_ * S));

    tileMatrix_.assign(numHeightTiles_, std::vector<SurfaceTile*>(numWidthTiles_, nullptr));

    for (int i = 0; i < numHeightTiles_; ++i) {
        for (int j = 0; j < numWidthTiles_; ++j) {
            const long long keyX = baseTx_ + j;
            const long long keyY = baseTy_ + ((numHeightTiles_ - 1) - i);

            const QVector3D tileOrigin(
                float(keyX * S),
                float(keyY * S),
                0.0f
                );

            const TileKey key{ int(keyX), int(keyY), zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);
            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
        }
    }
}

void SurfaceMesh::resizeColumnsLeft(int add)
{
    if (add <= 0) {
        return;
    }

    const double S = double(tileSideMeterSize_);

    baseTx_ -= add; // смещение базового индекса, а не origin через double
    origin_.setX(float(baseTx_ * S));

    for (int i = 0; i < numHeightTiles_; ++i) {
        const int oldW = int(tileMatrix_[i].size());
        tileMatrix_[i].resize(oldW + add);
        for (int j = oldW - 1; j >= 0; --j) { // сдвиг вправо
            tileMatrix_[i][j + add] = tileMatrix_[i][j];
        }

        for (int j = 0; j < add; ++j) { // новые слева
            const long long keyX = baseTx_ + j;
            const long long keyY = baseTy_ + ((numHeightTiles_ - 1) - i);

            const QVector3D tileOrigin(float(keyX * S), float(keyY * S), 0.0f);
            const TileKey key{ int(keyX), int(keyY), zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);
            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
        }
    }

    numWidthTiles_ += add;
}

void SurfaceMesh::resizeRowsBottom(int add)
{
    if (add <= 0) {
        return;
    }

    const double S = double(tileSideMeterSize_);

    baseTy_ -= add; // базовый индекс вниз
    origin_.setY(float(baseTy_ * S));

    const int oldH = numHeightTiles_;
    const int newH = oldH + add;

    tileMatrix_.resize(newH);
    for (int i = oldH; i < newH; ++i) {
        tileMatrix_[i].resize(numWidthTiles_);

        for (int j = 0; j < numWidthTiles_; ++j) {
            const long long keyX = baseTx_ + j;
            const long long keyY = baseTy_ + ((newH - 1) - i);

            const QVector3D tileOrigin(float(keyX * S), float(keyY * S), 0.0f);
            const TileKey key{ int(keyX), int(keyY), zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);
            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
        }
    }

    numHeightTiles_ = newH;
}

void SurfaceMesh::resizeColumnsRight(int add)
{
    if (add <= 0) {
        return;
    }

    const double S = double(tileSideMeterSize_);
    const int oldW = numWidthTiles_;

    for (int i = 0; i < numHeightTiles_; ++i) {
        tileMatrix_[i].resize(oldW + add);

        for (int j = oldW; j < oldW + add; ++j) {
            const long long keyX = baseTx_ + j;
            const long long keyY = baseTy_ + ((numHeightTiles_ - 1) - i);

            const QVector3D tileOrigin(float(keyX * S), float(keyY * S), 0.0f);
            const TileKey key{ int(keyX), int(keyY), zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);
            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
        }
    }

    numWidthTiles_ += add;
}

void SurfaceMesh::resizeRowsTop(int add)
{
    if (add <= 0) {
        return;
    }

    const double S = double(tileSideMeterSize_);
    const int newH = numHeightTiles_ + add;

    tileMatrix_.resize(newH);
    for (int i = newH - 1; i >= add; --i) {
        tileMatrix_[i] = std::move(tileMatrix_[i - add]);
    }

    for (int i = 0; i < add; ++i) {
        tileMatrix_[i].resize(numWidthTiles_);

        for (int j = 0; j < numWidthTiles_; ++j) {
            const long long keyX = baseTx_ + j;
            const long long keyY = baseTy_ + ((newH - 1) - i);

            const QVector3D tileOrigin(float(keyX * S), float(keyY * S), 0.0f);
            const TileKey key{ int(keyX), int(keyY), zoomIndex_ };

            auto* tile = new SurfaceTile(key, tileOrigin);
            tiles_.push_back(tile);
            tileMatrix_[i][j] = tile;
            tileByKey_.insert(key, tile);
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
