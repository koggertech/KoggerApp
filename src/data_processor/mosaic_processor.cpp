#include "mosaic_processor.h"

#include <QtMath>
#include <QDebug>
#include <QThread>
#include "data_processor.h"
#include "dataset.h"


static constexpr int sampleLimiter    = 2;
static constexpr int colorTableSize_  = 255;

static bool checkLength(float dist)
{
    if (qFuzzyIsNull(dist) || (dist < 0.0f)) {
        return false;
    }
    return true;
}

static int sampleIndex(Epoch::Echogram *echogramPtr, float dist)
{
    if (!echogramPtr) {
        return -1;
    }

    const float resolution = echogramPtr->resolution;

    if (resolution <= 0.0f) {
        return -1;
    }

    float resDist = dist / resolution;
    if (resDist < 0.f) {
        return -1;
    }

    int indx = static_cast<int>(std::round(resDist));
    if (indx >= static_cast<int>(echogramPtr->amplitude.size()) - sampleLimiter) {
        return -1;
    }

    return indx;
}

MosaicProcessor::MosaicProcessor(DataProcessor* parent)
    : dataProcessor_(parent),
    datasetPtr_(nullptr),
    surfaceMeshPtr_(nullptr),
    tileResolution_(defaultTileResolution),
    pixOnMeters_(std::pow(tileResolution_, -1)),
    aliasWindow_(100/pixOnMeters_),
    currIndxSec_(0),
    segFSubChannelId_(0),
    segSSubChannelId_(0),
    tileSidePixelSize_(defaultTileSidePixelSize),
    tileHeightMatrixRatio_(defaultTileHeightMatrixRatio),
    lastCalcEpoch_(0),
    lastAcceptedEpoch_(0),
    lAngleOffset_(0.0f),
    rAngleOffset_(0.0f),
    generateGridContour_(false)
{
    qRegisterMetaType<std::vector<uint8_t>>("std::vector<uint8_t>");
    qRegisterMetaType<TileMap>("TileMap");
}

MosaicProcessor::~MosaicProcessor()
{
}

void MosaicProcessor::clear()
{
    //qDebug() << "MosaicProcessor::clear";

    lastMatParams_ = kmath::MatrixParams();
    currIndxSec_ = 0;

    lastCalcEpoch_ = 0;
    lastAcceptedEpoch_ = 0;
}

void MosaicProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void MosaicProcessor::setSurfaceMeshPtr(SurfaceMesh *surfaceMeshPtr)
{
    surfaceMeshPtr_ = surfaceMeshPtr;
}

void MosaicProcessor::setChannels(const ChannelId& firstChId, uint8_t firstSubChId, const ChannelId& secondChId, uint8_t secondSubChId)
{
    //qDebug() << "MosaicProcessor::setChannels" << firstChId.toShortName() << firstSubChId << secondChId.toShortName() << secondSubChId;

    segFChannelId_    = firstChId;
    segFSubChannelId_ = firstSubChId;
    segSChannelId_    = secondChId;
    segSSubChannelId_ = secondSubChId;
}

void MosaicProcessor::updateDataWrapper(const QVector<int>& indxs)
{
    //qDebug() << "MosaicProcessor::updateDataWrapper" << indxs;

    if (!datasetPtr_ || indxs.isEmpty()) {
        return;
    }

    QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kMosaic));

    QVector<int> vec = indxs;
    const int firstNow = vec.first();

    if (lastAcceptedEpoch_ >= 0 && lastAcceptedEpoch_ < firstNow) {
        const int gap = firstNow - lastAcceptedEpoch_;

        if (gap > 1) { // дыра слева [lastAcceptedEpoch_, firstNow)
            QVector<int> left;
            left.reserve(gap);
            for (int i = lastAcceptedEpoch_; i < firstNow; ++i) {
                left.push_back(i);
            }

            left += vec;
            vec.swap(left);
        }
        else { // gap == 1
             vec.prepend(lastAcceptedEpoch_); // стык одним элементом
        }
    }
    else {
        //
    }

    //qDebug() << "task";
    //qDebug() << vec;
    //for (int i = 1; i < vec.size(); ++i) {
    //   if (vec[i] != vec[i - 1] + 1) {
    //       qWarning() << "Hole in mosaic task" << vec[i - 1] << "and" << vec[i];
    //   }
    //}

    const int kStep = 100; // почанково
    QVector<int> chunk;

    int start = 0;
    const int last = vec.size() - 1;

    while (start <= last) {
        int end = qMin(start + kStep, last);

        chunk.clear();
        chunk.reserve(end - start + 1);
        for (int i = start; i <= end; ++i) {
            chunk.push_back(vec[i]);
        }

        updateData(chunk);

        if (end == last) {
            break;
        }
        start = end;
    }

    QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kUndefined));
}

void MosaicProcessor::resetTileSettings(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)//
{
    clear();

    tileSidePixelSize_ = tileSidePixelSize;
    tileHeightMatrixRatio_ = tileHeightMatrixRatio;
    tileResolution_ = tileResolution;
    pixOnMeters_ = std::pow(tileResolution_, -1);
    aliasWindow_ = 100 / pixOnMeters_;

    surfaceMeshPtr_->reinit(tileSidePixelSize, tileHeightMatrixRatio, tileResolution);//
}

void MosaicProcessor::setColorTableThemeById(int id)
{
    if (colorTable_.getTheme() == id) {
        return;
    }

    colorTable_.setTheme(id);

    QMetaObject::invokeMethod(dataProcessor_, "postMosaicColorTable", Qt::QueuedConnection, Q_ARG(std::vector<uint8_t>, colorTable_.getRgbaColors()));
}

void MosaicProcessor::setColorTableLevels(float lowVal, float highVal)
{
    auto levels = colorTable_.getLevels();
    if (qFuzzyCompare(1.0 + levels.first, 1.0 + lowVal) &&
        qFuzzyCompare(1.0 + levels.second, 1.0 + highVal)) {
        return;
    }

    colorTable_.setLevels(lowVal, highVal);

    QMetaObject::invokeMethod(dataProcessor_, "postMosaicColorTable", Qt::QueuedConnection, Q_ARG(std::vector<uint8_t>, colorTable_.getRgbaColors()));
}

void MosaicProcessor::setColorTableLowLevel(float val)
{
    if (qFuzzyCompare(1.0 + colorTable_.getLowLevel(), 1.0 + val)) {
        return;
    }

    colorTable_.setLowLevel(val);

    QMetaObject::invokeMethod(dataProcessor_, "postMosaicColorTable", Qt::QueuedConnection, Q_ARG(std::vector<uint8_t>, colorTable_.getRgbaColors()));
}

void MosaicProcessor::setColorTableHighLevel(float val)
{
    if (qFuzzyCompare(1.0 + colorTable_.getHighLevel(), 1.0 + val)) {
        return;
    }

    colorTable_.setHighLevel(val);

    QMetaObject::invokeMethod(dataProcessor_, "postMosaicColorTable", Qt::QueuedConnection, Q_ARG(std::vector<uint8_t>, colorTable_.getRgbaColors()));
}

void MosaicProcessor::setLAngleOffset(float val)
{
    if (val < -90.0f || val > 90.0f) {
        return;
    }

    lAngleOffset_ = val;
}

void MosaicProcessor::setRAngleOffset(float val)
{
    if (val < -90.0f || val > 90.0f) {
        return;
    }

    rAngleOffset_ = val;
}

void MosaicProcessor::setTileResolution(float tileResolution)
{
    tileResolution_ = tileResolution;
    pixOnMeters_ = std::pow(tileResolution_, -1);
    aliasWindow_ = 100 / pixOnMeters_;
}

void MosaicProcessor::setGenerageGridContour(bool state)
{
    generateGridContour_ = state;
}

void MosaicProcessor::askColorTableForMosaic()
{
    QMetaObject::invokeMethod(dataProcessor_, "postMosaicColorTable", Qt::QueuedConnection, Q_ARG(std::vector<uint8_t>, colorTable_.getRgbaColors()));
}

// Шьём только ВВЕРХ и ВЛЕВО и в тот угол
// Причина: у каждого шва/угла ровно один владелец, это исключает двойную запись, гарантируя одинаковые значения по обе стороны шва
void MosaicProcessor::postUpdate(const QSet<SurfaceTile*>& updatedIn, QSet<SurfaceTile*>& changedOut)
{
    if (!surfaceMeshPtr_ || !surfaceMeshPtr_->getIsInited() || updatedIn.isEmpty()) {
        return;
    }

    auto& matrix = surfaceMeshPtr_->getTileMatrixRef();
    const int tilesY = matrix.size();
    if (tilesY == 0) {
        return;
    }

    const int tilesX = matrix.at(0).size();
    if (tilesX == 0) {
        return;
    }

    // helpers
    auto findIJ = [&](SurfaceTile* t, int& oi, int& oj)->bool {
        for (int i = 0; i < tilesY; ++i) {
            for (int j = 0; j < tilesX; ++j) {
                if (matrix[i][j] == t) { oi = i; oj = j; return true; }
            }
        }
        return false;
    };
    auto copyRow = [&](QVector<QVector3D>& from,
                       int fromRow, QVector<QVector3D>& to, QVector<HeightType>& mTo,
                       int toRow, int hvSide)
    {
        const int fromStart = fromRow * hvSide;
        const int toStart   = toRow   * hvSide;
        for (int k = 0; k < hvSide; ++k) {
            const int si = fromStart + k;
            const int di = toStart   + k;
            if (!qFuzzyIsNull(from[si][2])) {
                to[di][2] = from[si][2];
                mTo[di]   = HeightType::kMosaic;
            }
        }
    };
    auto copyCol = [&](QVector<QVector3D>& from,
                       int fromCol, QVector<QVector3D>& to, QVector<HeightType>& mTo,
                       int toCol, int hvSide)
    {
        for (int k = 0; k < hvSide; ++k) {
            const int si = k * hvSide + fromCol;
            const int di = k * hvSide + toCol;
            if (!qFuzzyIsNull(from[si][2])) {
                to[di][2] = from[si][2];
                mTo[di]   = HeightType::kMosaic;
            }
        }
    };
    auto copyCorner = [&](QVector<QVector3D>& from, int si,
                          QVector<QVector3D>& to,   QVector<HeightType>& mTo,   int di)
    {
        if (!qFuzzyIsNull(from[si][2])) {
            to[di][2] = from[si][2];
            mTo[di]   = HeightType::kMosaic;
        }
    };

    for (SurfaceTile* tile : updatedIn) {
        if (!tile || !tile->getIsUpdated()) {
            continue;
        }

        int i = -1, j = -1;
        if (!findIJ(tile, i, j)) {
            continue;
        }

        auto& vSrc = tile->getHeightVerticesRef();
        const int n = vSrc.size();
        const int hvSide = int(std::sqrt(double(n)));
        if (hvSide * hvSide != n || hvSide <= 1) {
            continue;
        }

        // Индексы углов текущего тайла
        const int TL = 0;
        // const int TR = hvSide - 1;
        // const int BL = hvSide * (hvSide - 1);
        const int BR = hvSide * hvSide - 1;

        // 4 стороны
        // TOP: текущая верхняя строка -> нижняя строка top-соседа (i+1, j)
        if (i + 1 < tilesY) {
            SurfaceTile* top = matrix[i + 1][j];
            if (top && top->getIsInited()) {
                auto& vTop = top->getHeightVerticesRef();
                auto& mTop = top->getHeightMarkVerticesRef();
                copyRow(vSrc,
                        /*fromRow=*/0, vTop, mTop,
                        /*toRow=*/hvSide - 1, hvSide);
                top->setIsUpdated(true);
                changedOut.insert(top);
            }
        }
        // // BOTTOM: текущая нижняя строка -> верхняя строка bottom-соседа (i-1, j)
        // if (i - 1 >= 0) {
        //     SurfaceTile* bottom = matrix[i - 1][j];
        //     if (bottom && bottom->getIsInited()) {
        //         auto& vBtm = bottom->getHeightVerticesRef();
        //         auto& mBtm = bottom->getHeightMarkVerticesRef();
        //         copyRow(vSrc,
        //                 /*fromRow=*/hvSide - 1, vBtm, mBtm,
        //                 /*toRow=*/0, hvSide);
        //         bottom->setIsUpdated(true);
        //         changedOut.insert(bottom);
        //     }
        // }
        // LEFT: текущий левый столбец -> правый столбец left-соседа (i, j-1)
        if (j - 1 >= 0) {
            SurfaceTile* left = matrix[i][j - 1];
            if (left && left->getIsInited()) {
                auto& vLeft = left->getHeightVerticesRef();
                auto& mLeft = left->getHeightMarkVerticesRef();
                copyCol(vSrc,
                        /*fromCol=*/0, vLeft, mLeft,
                        /*toCol=*/hvSide - 1, hvSide);
                left->setIsUpdated(true);
                changedOut.insert(left);
            }
        }

        // // RIGHT: текущий правый столбец -> левый столбец right-соседа (i, j+1)
        // if (j + 1 < tilesX) {
        //     SurfaceTile* right = matrix[i][j + 1];
        //     if (right && right->getIsInited()) {
        //         auto& vRight = right->getHeightVerticesRef();
        //         auto& mRight = right->getHeightMarkVerticesRef();
        //         copyCol(vSrc,
        //                 /*fromCol=*/hvSide - 1, vRight, mRight,
        //                 /*toCol=*/0, hvSide);
        //         right->setIsUpdated(true);
        //         changedOut.insert(right);
        //     }
        // }

        // 4 угла
        // TOP-LEFT (i+1, j-1): TL -> BR
        if (i + 1 < tilesY && j - 1 >= 0) {
            SurfaceTile* diag = matrix[i + 1][j - 1];
            if (diag && diag->getIsInited()) {
                auto& vD = diag->getHeightVerticesRef();
                auto& mD = diag->getHeightMarkVerticesRef();
                copyCorner(vSrc, TL, vD, mD, BR);
                diag->setIsUpdated(true);
                changedOut.insert(diag);
            }
        }
        // // TOP-RIGHT (i+1, j+1): TR -> BL
        // if (i + 1 < tilesY && j + 1 < tilesX) {
        //     SurfaceTile* diag = matrix[i + 1][j + 1];
        //     if (diag && diag->getIsInited()) {
        //         auto& vD = diag->getHeightVerticesRef();
        //         auto& mD = diag->getHeightMarkVerticesRef();
        //         copyCorner(vSrc, TR, vD, mD, BL);
        //         diag->setIsUpdated(true);
        //         changedOut.insert(diag);
        //     }
        // }
        // // BOTTOM-LEFT (i-1, j-1): BL -> TR
        // if (i - 1 >= 0 && j - 1 >= 0) {
        //     SurfaceTile* diag = matrix[i - 1][j - 1];
        //     if (diag && diag->getIsInited()) {
        //         auto& vD = diag->getHeightVerticesRef();
        //         auto& mD = diag->getHeightMarkVerticesRef();
        //         copyCorner(vSrc, BL, vD, mD, TR);
        //         diag->setIsUpdated(true);
        //         changedOut.insert(diag);
        //     }
        // }
        // // BOTTOM-RIGHT (i-1, j+1): BR -> TL
        // if (i - 1 >= 0 && j + 1 < tilesX) {
        //     SurfaceTile* diag = matrix[i - 1][j + 1];
        //     if (diag && diag->getIsInited()) {
        //         auto& vD = diag->getHeightVerticesRef();
        //         auto& mD = diag->getHeightMarkVerticesRef();
        //         copyCorner(vSrc, BR, vD, mD, TL);
        //         diag->setIsUpdated(true);
        //         changedOut.insert(diag);
        //     }
        // }
    }
}

void MosaicProcessor::updateUnmarkedHeightVertices(SurfaceTile* tilePtr) const
{
    if (!tilePtr) {
        return;
    }

    auto& heightVerticesRef = tilePtr->getHeightVerticesRef();
    auto& heightMarkVerticesRef = tilePtr->getHeightMarkVerticesRef();
    int hVSize = heightVerticesRef.size();
    int hMVSize = heightMarkVerticesRef.size();

    if (hVSize != hMVSize) {
        return;
    }

    auto writeHeight = [&](int toIndx, int fromIndx) -> bool {
        if (fromIndx >= 0 && fromIndx < hVSize) {
            if (heightMarkVerticesRef[fromIndx] == HeightType::kMosaic) {
                heightVerticesRef[toIndx][2] = heightVerticesRef[fromIndx][2];
                return true;
            }
        }
        return false;
    };

    int sideSize = std::sqrt(hVSize);
    for (int i = 0; i < hVSize; ++i) {
        if (heightMarkVerticesRef[i] == HeightType::kUndefined) {
            if (i % sideSize) {
                if (writeHeight(i, i - 1) ||
                    writeHeight(i, (i - 1) - sideSize) ||
                    writeHeight(i, (i - 1) + sideSize)) {
                    continue;
                }
            }
            if ((i % sideSize + 1) < sideSize) {
                if (writeHeight(i, i + 1) ||
                    writeHeight(i, (i + 1) - sideSize)||
                    writeHeight(i, (i + 1) + sideSize)) {
                    continue;
                }
            }
            if (writeHeight(i, i - sideSize) ||
                writeHeight(i, i + sideSize)) {
                continue;
            }
        }
    }
}

void MosaicProcessor::updateData(const QVector<int>& indxs)
{
    // qDebug() << "MosaicProcessor::updateData"
    //          << "thread =" << QThread::currentThread()
    //          << "size =" << indxs.size();

    bool bench = false;
    QElapsedTimer et; // test
    if (bench) {
        et.start();
    }

    if (!datasetPtr_ || indxs.empty()) {
        return;
    }
    const bool segFIsValid = segFChannelId_.isValid();
    const bool segSIsValid = segSChannelId_.isValid();
    if (!segFIsValid && !segSIsValid) {
        return;
    }

    kmath::MatrixParams actualMatParams(lastMatParams_);
    kmath::MatrixParams newMatrixParams;
    QVector<QVector3D>  measLinesVertices;
    QVector<int>        measLinesEvenIndices;
    QVector<int>        measLinesOddIndices;
    QVector<char>       isOdds; // 0 - even, 1 - odd
    QVector<int>        epochIndxs;
    QVector3D           lastLeftBeg, lastLeftEnd, lastRightBeg, lastRightEnd;
    bool                haveLastPair = false;

    // update matrix
    for (const auto& i : indxs) {
        auto epoch = datasetPtr_->fromIndexCopy(i);
        if (!epoch.isValid()) {
            continue;
        }

        auto pos = epoch.getSonarPosition().ned;
        auto yaw = epoch.yaw();
        if (isfinite(pos.n) && isfinite(pos.e) && isfinite(yaw)) {
            bool acceptedEven = false, acceptedOdd = false;
            double azRad = qDegreesToRadians(yaw);
            if (segFIsValid) {
                if (auto segFCharts = epoch.chart(segFChannelId_, segFSubChannelId_); segFCharts) {
                    double leftAzRad = azRad - M_PI_2 + qDegreesToRadians(lAngleOffset_);
                    float lDist = segFCharts->range();
                    lastLeftBeg = QVector3D(pos.n + lDist * qCos(leftAzRad), pos.e + lDist * qSin(leftAzRad), 0.0f);
                    lastLeftEnd = QVector3D(pos.n, pos.e, 0.0f);
                    measLinesVertices.append(lastLeftBeg);
                    measLinesVertices.append(lastLeftEnd);
                    measLinesEvenIndices.append(currIndxSec_++);
                    measLinesEvenIndices.append(currIndxSec_++);
                    isOdds.append('0');
                    epochIndxs.append(i);
                    acceptedEven = true;
                }
            }

            if (segSIsValid) {
                if (auto segSCharts = epoch.chart(segSChannelId_, segSSubChannelId_); segSCharts) {
                    double rightAzRad = azRad + M_PI_2 - qDegreesToRadians(rAngleOffset_);
                    float rDist = segSCharts->range();
                    lastRightBeg = QVector3D(pos.n, pos.e, 0.0f);
                    lastRightEnd = QVector3D(pos.n + rDist * qCos(rightAzRad), pos.e + rDist * qSin(rightAzRad), 0.0f);
                    measLinesVertices.append(lastRightBeg);
                    measLinesVertices.append(lastRightEnd);
                    measLinesOddIndices.append(currIndxSec_++);
                    measLinesOddIndices.append(currIndxSec_++);
                    isOdds.append('1');
                    epochIndxs.append(i);
                    acceptedOdd = true;
                }
            }

            if (acceptedEven || acceptedOdd) {
                lastAcceptedEpoch_ = std::max(lastAcceptedEpoch_, i);
            }

            if (acceptedEven && acceptedOdd) {
                haveLastPair = true;
            }
        }
    }
    const float tileSideMeters = tileSidePixelSize_ * tileResolution_;
    newMatrixParams = kmath::getMatrixParams(measLinesVertices, tileSideMeters);
    if (!newMatrixParams.isValid()) {
        return;
    }
    concatenateMatrixParameters(actualMatParams, newMatrixParams);

    // expand surface mesh
    const float marginMeters  = expandMargin_ * tileSideMeters;
    kmath::MatrixParams expanded = actualMatParams;
    expanded.originX -= marginMeters;
    expanded.originY -= marginMeters;
    expanded.width   += 2 * marginMeters;
    expanded.height  += 2 * marginMeters;
    const double S = double(tileSidePixelSize_ * tileResolution_);
    auto snapDown = [&](double v){ return std::floor(v / S) * S; };
    auto snapUp   = [&](double v){ return std::ceil (v / S) * S; };
    {
        const double x0  = snapDown(expanded.originX);
        const double y0  = snapDown(expanded.originY);
        const double x1  = snapUp  (expanded.originX + expanded.width);
        const double y1  = snapUp  (expanded.originY + expanded.height);
        expanded.originX = float(x0);
        expanded.originY = float(y0);
        expanded.width   = int(std::lround(x1 - x0));
        expanded.height  = int(std::lround(y1 - y0));
    }
    surfaceMeshPtr_->concatenate(expanded);

    { // prefetch tiles
        QSet<TileKey> toRestore = forecastTilesToTouch(measLinesVertices, isOdds, epochIndxs, expandMargin_/*for prefetch*/);

        if (!toRestore.isEmpty()) {
            QSet<TileKey> need;
            need.reserve(toRestore.size());
            for (auto it = toRestore.cbegin(); it != toRestore.cend(); ++it) {
                auto cKey = *it;
                if (auto* t = surfaceMeshPtr_->getTilePtrByKey(cKey); t) {
                    if (!t->getIsInited()) {
                        need.insert(cKey);
                    }
                }
            }

            if (!need.isEmpty()) {
                prefetchTiles(need);
            }

            for (auto it = need.cbegin(); it != need.cend(); ++it) {
                if (auto* tile = surfaceMeshPtr_->getTilePtrByKey(*it); tile) {
                    if (!tile->getIsInited()) {
                        tile->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                    }
                }
            }
        }
    }

    // after expand by martix
    const int numHeightTiles = surfaceMeshPtr_->getNumHeightTiles();
    const int stepSizeHeightMatrix = surfaceMeshPtr_->getStepSizeHeightMatrix();

    // helpers
    auto putPixel = [&](int x, int y, uint8_t color) -> SurfaceTile* {
        const int meshX = x / tileSidePixelSize_;
        const int meshY = (numHeightTiles - 1) - (y / tileSidePixelSize_);
        SurfaceTile* t  = surfaceMeshPtr_->getTileByXYIndxs(meshY, meshX);
        if (!t) {
            return nullptr;
        }

        const int tileX = x % tileSidePixelSize_;
        const int tileY = y % tileSidePixelSize_;

        auto& img = t->getMosaicImageDataRef();
        img.data()[tileY * tileSidePixelSize_ + tileX] = color;
        t->setIsUpdated(true);

        return t;
    };

    if (bench) {
        qDebug() << "prep time,    ms" << et.elapsed(); // test
        et.restart();
        et.start();
    }

    // ray tracing
    for (int i = 0; i < measLinesVertices.size(); i += 2) {
        if (canceled()) {
            return;
        }
        if (i + 5 > measLinesVertices.size() - 1) {
            break;
        }

        int segFBegVertIndx = i;
        int segFEndVertIndx = i + 1;
        int segSBegVertIndx = i + 2;
        int segSEndVertIndx = i + 3;

        int segFIndx = i / 2;
        int segSIndx = (i + 2) / 2;

        if (epochIndxs[segFIndx] == epochIndxs[segSIndx] ||
            isOdds[segFIndx] != isOdds[segSIndx]) {
            ++segSIndx;
            segSBegVertIndx += 2;
            segSEndVertIndx += 2;
        }
        if (segSIndx >= epochIndxs.size()) {
            break;
        }

        if (epochIndxs[segFIndx] == epochIndxs[segSIndx] ||
            isOdds[segFIndx] != isOdds[segSIndx]) {
            continue;
        }

        auto segFEpoch = datasetPtr_->fromIndexCopy(epochIndxs[segFIndx]);
        auto segSEpoch = datasetPtr_->fromIndexCopy(epochIndxs[segSIndx]);
        if (!segFEpoch.isValid() || !segSEpoch.isValid()) {
            continue;
        }

        bool segFIsOdd = isOdds[segFIndx] == '1';
        bool segSIsOdd = isOdds[segSIndx] == '1';
        if (segFIsOdd != segSIsOdd) {
            continue;
        }

        auto segFCh  = segFIsOdd ? segSChannelId_    : segFChannelId_;
        auto segFSCh = segFIsOdd ? segSSubChannelId_ : segFSubChannelId_;
        auto segSCh  = segSIsOdd ? segSChannelId_    : segFChannelId_;
        auto segSSCh = segSIsOdd ? segSSubChannelId_ : segFSubChannelId_;
        auto* segFCharts = segFEpoch.chart(segFCh, segFSCh);
        auto* segSCharts = segSEpoch.chart(segSCh, segSSCh);
        if (!segFCharts || !segSCharts) {
            continue;
        }
        if (!isfinite(segFCharts->bottomProcessing.getDistance()) ||
            !isfinite(segSCharts->bottomProcessing.getDistance())) {
            continue;
        }
        if (segFCharts->amplitude.size() != segFCharts->compensated.size()) {
            segFCharts->updateCompesated();
        }
        if (segSCharts->amplitude.size() != segSCharts->compensated.size()) {
            segSCharts->updateCompesated();
        }

        // Bresenham, first segment
        QVector3D segFPhBegPnt = segFIsOdd ? measLinesVertices[segFBegVertIndx] : measLinesVertices[segFEndVertIndx];
        QVector3D segFPhEndPnt = segFIsOdd ? measLinesVertices[segFEndVertIndx] : measLinesVertices[segFBegVertIndx];
        auto segFBegPixPos = surfaceMeshPtr_->convertPhToPixCoords(segFPhBegPnt);
        auto segFEndPixPos = surfaceMeshPtr_->convertPhToPixCoords(segFPhEndPnt);
        int segFPixX1 = segFBegPixPos.x();
        int segFPixY1 = segFBegPixPos.y();
        int segFPixX2 = segFEndPixPos.x();
        int segFPixY2 = segFEndPixPos.y();
        float segFPixTotDist = std::sqrt(std::pow(segFPixX2 - segFPixX1, 2) + std::pow(segFPixY2 - segFPixY1, 2));
        int segFPixDx = std::abs(segFPixX2 - segFPixX1);
        int segFPixDy = std::abs(segFPixY2 - segFPixY1);
        int segFPixSx = (segFPixX1 < segFPixX2) ? 1 : -1;
        int segFPixSy = (segFPixY1 < segFPixY2) ? 1 : -1;
        int segFPixErr = segFPixDx - segFPixDy;
        // second segment
        QVector3D segSPhBegPnt = segSIsOdd ? measLinesVertices[segSBegVertIndx] : measLinesVertices[segSEndVertIndx];
        QVector3D segSPhEndPnt = segSIsOdd ? measLinesVertices[segSEndVertIndx] : measLinesVertices[segSBegVertIndx];
        auto segSBegPixPos = surfaceMeshPtr_->convertPhToPixCoords(segSPhBegPnt);
        auto segSEndPixPos = surfaceMeshPtr_->convertPhToPixCoords(segSPhEndPnt);
        int segSPixX1 = segSBegPixPos.x();
        int segSPixY1 = segSBegPixPos.y();
        int segSPixX2 = segSEndPixPos.x();
        int segSPixY2 = segSEndPixPos.y();
        float segSPixTotDist = std::sqrt(std::pow(segSPixX2 - segSPixX1, 2) + std::pow(segSPixY2 - segSPixY1, 2));
        int segSPixDx = std::abs(segSPixX2 - segSPixX1);
        int segSPixDy = std::abs(segSPixY2 - segSPixY1);
        int segSPixSx = (segSPixX1 < segSPixX2) ? 1 : -1;
        int segSPixSy = (segSPixY1 < segSPixY2) ? 1 : -1;
        int segSPixErr = segSPixDx - segSPixDy;
        // pixel length checking
        if (!checkLength(segFPixTotDist) ||
            !checkLength(segSPixTotDist)) {
            continue;
        }

        float segFDistProc = -1.0f * static_cast<float>(segFIsOdd ? segFEpoch.chart(segSChannelId_, segSSubChannelId_)->bottomProcessing.getDistance() : segFEpoch.chart(segFChannelId_, segFSubChannelId_)->bottomProcessing.getDistance());
        float segSDistProc = -1.0f * static_cast<float>(segSIsOdd ? segSEpoch.chart(segSChannelId_, segSSubChannelId_)->bottomProcessing.getDistance() : segSEpoch.chart(segFChannelId_, segFSubChannelId_)->bottomProcessing.getDistance());
        float segFPhDistX = segFPhEndPnt.x() - segFPhBegPnt.x();
        float segFPhDistY = segFPhEndPnt.y() - segFPhBegPnt.y();
        float segSPhDistX = segSPhEndPnt.x() - segSPhBegPnt.x();
        float segSPhDistY = segSPhEndPnt.y() - segSPhBegPnt.y();

        auto segFInterpNED = segFEpoch.getSonarPosition().ned;
        auto segSInterpNED = segSEpoch.getSonarPosition().ned;
        QVector3D segFBoatPos(segFInterpNED.n, segFInterpNED.e, 0.0f);
        QVector3D segSBoatPos(segSInterpNED.n, segSInterpNED.e, 0.0f);

        // select longest ray
        const bool bigIsF = (segFPixTotDist >= segSPixTotDist);
        int       bigX1  = bigIsF ? segFPixX1  :  segSPixX1;
        int       bigY1  = bigIsF ? segFPixY1  :  segSPixY1;
        const int bigX2  = bigIsF ? segFPixX2  :  segSPixX2;
        const int bigY2  = bigIsF ? segFPixY2  :  segSPixY2;
        int       bigErr = bigIsF ? segFPixErr :  segSPixErr;
        const int bigDx  = bigIsF ? segFPixDx  :  segSPixDx;
        const int bigDy  = bigIsF ? segFPixDy  :  segSPixDy;
        const int bigSx  = bigIsF ? segFPixSx  :  segSPixSx;
        const int bigSy  = bigIsF ? segFPixSy  :  segSPixSy;
        const float bigTot = std::max(1.0f, bigIsF ? segFPixTotDist : segSPixTotDist);

        auto bresStep = [&](int& x0, int& y0, const int& x1, const int& y1, int& err, const int& dx, const int& dy, const int& sx,  const int& sy) -> bool {
            if (x0 == x1 && y0 == y1) {
                return true;
            }

            int e2 = (err << 1);
            if (e2 > -dy) {
                err -= dy; x0 += sx;
            }
            else if (e2 < dx) {
                err += dx; y0 += sy;
            }

            return false;
        };

        while (true) { // follow the biggest segment
            const float rem = std::sqrt(float((bigX2 - bigX1) * (bigX2 - bigX1) + (bigY2 - bigY1) * (bigY2 - bigY1))); // moving on longest line
            const float t   = std::clamp(rem / bigTot, 0.0f, 1.0f);
            QVector3D segFCurrPhPos(segFPhBegPnt.x() + t * segFPhDistX, segFPhBegPnt.y() + t * segFPhDistY, segFDistProc);
            QVector3D segSCurrPhPos(segSPhBegPnt.x() + t * segSPhDistX, segSPhBegPnt.y() + t * segSPhDistY, segSDistProc);

            int segFColorIndx = getColorIndx(segFCharts, sampleIndex(segFCharts, segFCurrPhPos.distanceToPoint(segFBoatPos)));
            int segSColorIndx = getColorIndx(segSCharts, sampleIndex(segSCharts, segSCurrPhPos.distanceToPoint(segSBoatPos)));
            if (!segFColorIndx && !segSColorIndx) {
                if (bresStep(bigX1, bigY1, bigX2, bigY2, bigErr, bigDx, bigDy, bigSx, bigSy)) {
                    break;
                }
                continue;
            }
            auto segFCurrPixPos = surfaceMeshPtr_->convertPhToPixCoords(segFCurrPhPos);
            auto segSCurrPixPos = surfaceMeshPtr_->convertPhToPixCoords(segSCurrPhPos);

            // interp Bres
            int x0 = int(segFCurrPixPos.x());
            int y0 = int(segFCurrPixPos.y());
            const int x1 = int(segSCurrPixPos.x());
            const int y1 = int(segSCurrPixPos.y());
            const int dx = std::abs(x1 - x0);
            const int dy = std::abs(y1 - y0);
            const int sx = (x0 < x1) ? 1 : -1;
            const int sy = (y0 < y1) ? 1 : -1;
            int err = dx - dy;
            const int stepsTot = dx + dy;

            if (checkLength(stepsTot)) { // color interpolation between two pixels
                int stepsDone = 0;

                while (true) { // follow between pixels
                    const float iP       = static_cast<float>(stepsDone) / float(stepsTot); /*stepsTot*/
                    const auto  iClrIndx = static_cast<uint8_t>((1 - iP) * segFColorIndx + iP * segSColorIndx);

                    // рисуем текущий пиксель
                    auto* t = putPixel(x0, y0, iClrIndx);
                    if (!(stepsDone % tileHeightMatrixRatio_)) {
                        const int tileX = x0 % tileSidePixelSize_;
                        const int tileY = y0 % tileSidePixelSize_;
                        const int numSteps = tileSidePixelSize_ / stepSizeHeightMatrix + 1;
                        const int hIdx = (tileY / stepSizeHeightMatrix) * numSteps + (tileX / stepSizeHeightMatrix);
                        auto& hT = t->getHeightMarkVerticesRef()[hIdx];
                        if (hT != HeightType::kTriangulation) {
                            t->getHeightVerticesRef()[hIdx][2] = segFCurrPhPos[2];
                            hT = HeightType::kMosaic;
                        }
                    }

                    if (bresStep(x0, y0, x1, y1, err, dx, dy, sx, sy)) {
                        break;
                    }

                    ++stepsDone;
                }
            }

            if (bresStep(bigX1, bigY1, bigX2, bigY2, bigErr, bigDx, bigDy, bigSx, bigSy)) {
                break;
            }
        } // while interp line
    } // for rays

    lastMatParams_ = actualMatParams;

    if (bench) {
        qDebug() << "trace time,   ms" << et.elapsed();
        et.restart();
        et.start();
    }

    // collect changed tiles
    QSet<SurfaceTile*> updTiles = surfaceMeshPtr_->getUpdatedTiles();
    QSet<SurfaceTile*> changedOut;
    postUpdate(updTiles, changedOut);
    updTiles.unite(changedOut);
    TileMap res;
    res.reserve(updTiles.size());
    for (SurfaceTile* itm : std::as_const(updTiles)) {
        if (!itm || !itm->getIsInited()) {
            continue;
        }
        updateUnmarkedHeightVertices(itm);
        itm->updateHeightIndices();
        itm->setIsUpdated(false);
        res.insert((itm)->getKey(), (*itm));
    }
    surfaceMeshPtr_->setTileUsed(updTiles, true); // метка тайлов/ужимка

    if (bench) {
        qDebug() << "post up time, ms" << et.elapsed();
    }

    // emit data
    QMetaObject::invokeMethod(dataProcessor_, "postSurfaceTiles", Qt::QueuedConnection, Q_ARG(TileMap, res), Q_ARG(bool, true));
    if (haveLastPair) {
        QMetaObject::invokeMethod(dataProcessor_, "postTraceLines", Qt::QueuedConnection,
            Q_ARG(QVector3D, lastLeftBeg),
            Q_ARG(QVector3D, lastLeftEnd),
            Q_ARG(QVector3D, lastRightBeg),
            Q_ARG(QVector3D, lastRightEnd)
            );
    }
}

int MosaicProcessor::getColorIndx(Epoch::Echogram* charts, int ampIndx) const
{
    int retVal{ 0 };

    if (!charts || ampIndx == -1) {
        return retVal;
    }

    const auto ampSize = charts->compensated.size();

    if (ampSize > ampIndx) {
        if (aliasWindow_ == 1) {
            int cVal = charts->compensated[ampIndx] ;
            cVal = std::min(colorTableSize_, cVal);
            retVal = cVal;
        }
        else { // aliasing
            const int half  = aliasWindow_ / 2;
            const int lIndx = ampIndx - half;
            const int rIndx = ampIndx + half;

            if (lIndx > 0 && ampSize > lIndx &&
                rIndx > 0 && ampSize > rIndx) {
                int newColorIndx = 0;
                for (int i = lIndx; i < rIndx; ++i) {
                    int cVal = charts->compensated[i] ;
                    cVal = std::min(colorTableSize_, cVal);
                    newColorIndx += cVal;
                }
                retVal = float(newColorIndx) / float(aliasWindow_);
            }
            else {
                int cVal = charts->compensated[ampIndx] ;
                cVal = std::min(colorTableSize_, cVal);
                retVal = cVal;
            }
        }
    }
    else {
        return retVal; // если вышел за предел - ноль
    }

    if (!retVal) {
        return 1;
    }

    return retVal;
}
bool MosaicProcessor::canceled() const noexcept
{
    return dataProcessor_ && dataProcessor_->isCancelRequested();
}

QSet<TileKey> MosaicProcessor::forecastTilesToTouch(const QVector<QVector3D>& meas, const QVector<char>& isOdds, const QVector<int>& epochIndxs, int marginTiles) const
{
    QSet<TileKey> retVal;

    if (!surfaceMeshPtr_ || meas.size() < 4) {
        return retVal;
    }

    const int tilePx = surfaceMeshPtr_->getTileSidePixelSize();
    const int H      = surfaceMeshPtr_->getNumHeightTiles();
    const int W      = surfaceMeshPtr_->getNumWidthTiles();
    auto clamp = [](int v, int lo, int hi) -> int {
        return std::min(std::max(v, lo), hi);
    };

    const int stridePx = std::max(8, tilePx / 4);
    const int m = std::max(0, marginTiles);

    auto pushIdxWithMargin = [&](int mx, int my){
        const int x0 = clamp(mx - m, 0, W - 1);
        const int x1 = clamp(mx + m, 0, W - 1);
        const int y0 = clamp(my - m, 0, H - 1);
        const int y1 = clamp(my + m, 0, H - 1);
        for (int ty = y0; ty <= y1; ++ty) {
            for (int tx = x0; tx <= x1; ++tx) {
                if (auto* t = surfaceMeshPtr_->getTileByXYIndxs(ty, tx)) {
                    retVal.insert(t->getKey());
                }
            }
        }
    };

    auto addPix = [&](int pixX, int pixY){
        int mx = clamp(pixX / tilePx,          0, W - 1);
        int my = clamp((H - 1) - pixY / tilePx,0, H - 1);
        pushIdxWithMargin(mx, my);
    };

    auto addLineTiles = [&](int x1, int y1, int x2, int y2){
        int ax1 = clamp(x1 / tilePx,           0, W - 1);
        int ay1 = clamp((H - 1) - y1 / tilePx, 0, H - 1);
        int ax2 = clamp(x2 / tilePx,           0, W - 1);
        int ay2 = clamp((H - 1) - y2 / tilePx, 0, H - 1);

        int dx = std::abs(ax2 - ax1), sx = ax1 < ax2 ? 1 : -1;
        int dy = -std::abs(ay2 - ay1), sy = ay1 < ay2 ? 1 : -1;
        int err = dx + dy, x = ax1, y = ay1;

        while (true) {
            pushIdxWithMargin(x, y);
            if (x == ax2 && y == ay2) {
                break;
            }

            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x += sx;
            }

            if (e2 <= dx) {
                err += dx;
                y += sy;
            }
        }
    };

    for (int i = 0; i + 3 < meas.size(); i += 2) {
        int segFBegVertIndx = i,     segFEndVertIndx = i + 1;
        int segSBegVertIndx = i + 2, segSEndVertIndx = i + 3;

        int segFIndx = i / 2;
        int segSIndx = (i + 2) / 2;

        if (epochIndxs[segFIndx] == epochIndxs[segSIndx] || isOdds[segFIndx] != isOdds[segSIndx]) {
            ++segSIndx; segSBegVertIndx += 2; segSEndVertIndx += 2;
        }
        if (segSIndx >= epochIndxs.size()) {
            break;
        }

        if (epochIndxs[segFIndx] == epochIndxs[segSIndx] || isOdds[segFIndx] != isOdds[segSIndx]) {
            continue;
        }

        const bool segFIsOdd = isOdds[segFIndx] == '1';
        const bool segSIsOdd = isOdds[segSIndx] == '1';
        if (segFIsOdd != segSIsOdd) {
            continue;
        }

        auto Fbeg = surfaceMeshPtr_->convertPhToPixCoords(segFIsOdd ? meas[segFBegVertIndx] : meas[segFEndVertIndx]);
        auto Fend = surfaceMeshPtr_->convertPhToPixCoords(segFIsOdd ? meas[segFEndVertIndx] : meas[segFBegVertIndx]);
        auto Sbeg = surfaceMeshPtr_->convertPhToPixCoords(segSIsOdd ? meas[segSBegVertIndx] : meas[segSEndVertIndx]);
        auto Send = surfaceMeshPtr_->convertPhToPixCoords(segSIsOdd ? meas[segSEndVertIndx] : meas[segSBegVertIndx]);

        const int Fx1 = int(Fbeg.x()), Fy1 = int(Fbeg.y());
        const int Fx2 = int(Fend.x()), Fy2 = int(Fend.y());
        const int Sx1 = int(Sbeg.x()), Sy1 = int(Sbeg.y());
        const int Sx2 = int(Send.x()), Sy2 = int(Send.y());

        const float Ftot = std::hypot(Fx2 - Fx1, Fy2 - Fy1);
        const float Stot = std::hypot(Sx2 - Sx1, Sy2 - Sy1);
        const float longest = std::max(Ftot, Stot);
        if (longest <= 0.0f) {
            continue;
        }

        const int steps = std::max(1, int(std::ceil(longest / float(stridePx))));
        for (int s = 0; s <= steps; ++s) {
            float tt = float(s) / float(steps);
            const int Fx = int(std::lround(Fx1 + tt * (Fx2 - Fx1)));
            const int Fy = int(std::lround(Fy1 + tt * (Fy2 - Fy1)));
            const int Sx = int(std::lround(Sx1 + tt * (Sx2 - Sx1)));
            const int Sy = int(std::lround(Sy1 + tt * (Sy2 - Sy1)));
            addPix(Fx, Fy);
            addPix(Sx, Sy);
            addLineTiles(Fx, Fy, Sx, Sy);
        }
    }

    return retVal;
}

void MosaicProcessor::putTilesIntoMesh(const TileMap &tiles) // может вызвать только prefetch, т.к. инитит тайл
{
    if (!surfaceMeshPtr_ || tiles.isEmpty()) {
        return;
    }

    QSet<SurfaceTile*> initedNow;

    for (auto it = tiles.cbegin(); it != tiles.cend(); ++it) {
        const TileKey tKey = it.key();
        SurfaceTile* dst = surfaceMeshPtr_->getTilePtrByKey(tKey);
        if (!dst) {
            continue;
        }
        if (!dst->getIsInited()) {
            dst->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
        }

        const auto& src = it.value();

        // image
        auto& di = dst->getMosaicImageDataRef();
        const auto& si = src.getMosaicImageDataCRef();
        if (di.size() == si.size() && !si.empty()) {
            memcpy(di.data(), si.data(), size_t(di.size()));
        }
        // heights & marks
        dst->getHeightVerticesRef()     = src.getHeightVerticesCRef();
        dst->getHeightMarkVerticesRef() = src.getHeightMarkVerticesCRef();

        dst->updateHeightIndices();
        dst->setIsUpdated(false);

        if (dst->getIsInited()) {
            initedNow.insert(dst);
        }
    }

    if (!initedNow.isEmpty()) {
        surfaceMeshPtr_->setTileUsed(initedNow, false); // поднимет счетчики, эвикт выключен чтобы не выгрузить возможно нужные тайлы
    }

    return;
}

bool MosaicProcessor::prefetchTiles(const QSet<TileKey> &keys) // подгрузка тайлов с hotCache, db (dataprocessor)
{
    if (!dataProcessor_ || keys.isEmpty()) {
        return false;
    }

    Q_ASSERT(QThread::currentThread() != dataProcessor_->thread());

    TileMap allGot;
    QSet<TileKey> missing;

    auto putAllGotTiles = [&]() -> bool {
        if (!allGot.isEmpty()) {
            putTilesIntoMesh(allGot);
        }

        return !allGot.isEmpty();
    };

    // забрать с хот кэша
    if (!QMetaObject::invokeMethod(dataProcessor_, "fetchFromHotCache", Qt::BlockingQueuedConnection, Q_RETURN_ARG(TileMap, allGot), Q_ARG(QSet<TileKey>, keys), Q_ARG(QSet<TileKey>*, &missing))) {
        qWarning() << "[prefetch] invoke fetchFromHotCache failed";
    }

    // вычесть то, чего нет в БД
    QSet<TileKey> waiting;
    if (!QMetaObject::invokeMethod(dataProcessor_, "filterNotFoundOut", Qt::BlockingQueuedConnection, Q_ARG(QSet<TileKey>, missing), Q_ARG(QSet<TileKey>*, &waiting))) {
        qWarning() << "[prefetch] invoke filterNotFoundOut failed";
    }

    // нечего ждать,  бд не готова - выход
    if (waiting.isEmpty() || !dataProcessor_->isDbReady()) {
        return putAllGotTiles();
    }

    // цикл ожидания поступлений из БД
    while (!waiting.isEmpty()) {
        // попросить БД
        QMetaObject::invokeMethod(dataProcessor_, "requestTilesFromDB", Qt::QueuedConnection, Q_ARG(QSet<TileKey>, waiting));

        // засечь тик и уснуть до прогресса
        quint64 t0 = 0;
        if (!QMetaObject::invokeMethod(dataProcessor_, "prefetchProgressTick", Qt::BlockingQueuedConnection, Q_RETURN_ARG(quint64, t0))) {
            qWarning() << "[prefetch] invoke prefetchProgressTick failed";
        }

        // корректное ожидание через паблик-API
        dataProcessor_->prefetchWait(t0);

        if (canceled()) {
            break;
        }

        // попробуем снова забрать с горячего кеша (в него пишет БД)
        QSet<TileKey> stillMissing;
        TileMap newlyGot;

        if (!QMetaObject::invokeMethod(dataProcessor_, "fetchFromHotCache", Qt::BlockingQueuedConnection, Q_RETURN_ARG(TileMap, newlyGot), Q_ARG(QSet<TileKey>, waiting), Q_ARG(QSet<TileKey>*, &stillMissing))) {
            qWarning() << "[prefetch] invoke fetchFromHotCache (retry) failed";
        }

        // что осталось уйдёт в ожидание
        if (!QMetaObject::invokeMethod(dataProcessor_, "filterNotFoundOut", Qt::BlockingQueuedConnection, Q_ARG(QSet<TileKey>, stillMissing), Q_ARG(QSet<TileKey>*, &stillMissing))) {
            qWarning() << "[prefetch] invoke filterNotFoundOut (retry) failed";
        }

        // аккумуляция результата
        for (auto it = newlyGot.cbegin(); it != newlyGot.cend(); ++it) {
            allGot.insert(it.key(), it.value());
        }

        // на следующий поиск
        waiting.swap(stillMissing);
    }

    // выход
    return putAllGotTiles();
}
