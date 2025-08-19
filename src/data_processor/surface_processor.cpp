#include "isobaths_processor.h"

#include <cmath>
#include <QDebug>
#include "data_processor.h"
#include "bottom_track.h"
#include "surface_mesh.h"
#include "surface_tile.h"
#include "math_defs.h"


SurfaceProcessor::SurfaceProcessor(DataProcessor* parent) :
    dataProcessor_(parent),
    bottomTrackPtr_(nullptr),
    surfaceMeshPtr_(nullptr),
    origin_(0.0f, 0.0f),
    tileResolution_(defaultTileResolution),
    minZ_(std::numeric_limits<float>::max()),
    maxZ_(std::numeric_limits<float>::lowest()),
    edgeLimit_(20.0f),
    surfaceStepSize_(1.0f),
    tileSidePixelSize_(defaultTileSidePixelSize),
    tileHeightMatrixRatio_(defaultTileHeightMatrixRatio),
    themeId_(0),
    cellPx_(1),
    extraWidth_(0),
    originSet_ (false)
{
}

SurfaceProcessor::~SurfaceProcessor()
{
}

void SurfaceProcessor::clear()
{
    delaunayProc_ = delaunay::Delaunay();
    lastMatParams_ = kmath::MatrixParams();
    pointToTris_.clear();
    cellPoints_.clear();
    cellPointsInTri_.clear();
    origin_ = QPointF(0.0f, 0.0f);
    minZ_ = std::numeric_limits<float>::max();
    maxZ_ = std::numeric_limits<float>::lowest();
    originSet_ = false;
}

void SurfaceProcessor::setBottomTrackPtr(BottomTrack *bottomTrackPtr)
{
    bottomTrackPtr_ = bottomTrackPtr;
}

void SurfaceProcessor::setSurfaceMeshPtr(SurfaceMesh *surfaceMeshPtr)
{
    surfaceMeshPtr_ = surfaceMeshPtr;
}
void SurfaceProcessor::onUpdatedBottomTrackData(const QVector<int> &indxs)
{
    if (indxs.empty()) {
        return;
    }

    // Delaunay processing
    QVector<QVector3D> bTrData;
    {
        QReadLocker rl(&lock_);
        bTrData = bottomTrackPtr_ ? bottomTrackPtr_->cdata() : QVector<QVector3D>();
    }
    if (bTrData.empty()) {
        return;
    }

    dataProcessor_->changeState(DataProcessorType::kSurface);

    auto& tr = delaunayProc_.getTriangles();
    auto& pt = delaunayProc_.getPoints();

    const auto registerTriangle = [&](int triIdx) {
        const auto& t = tr[triIdx];
        pointToTris_[t.a] << triIdx;
        pointToTris_[t.b] << triIdx;
        pointToTris_[t.c] << triIdx;
    };

    QSet<int> updsTrIndx;
    bool beenManualChanged = false;

    static QSet<SurfaceTile*> changedTiles;
    changedTiles.clear();

    auto ensureMeshCoversDisk = [&](const QVector3D& P, float radiusM) -> void { // расширение меша для экстраполяции
        QVector<QVector3D> tri(4);
        tri[0] = QVector3D(P.x() + radiusM, P.y(),           P.z());
        tri[1] = QVector3D(P.x(),           P.y() + radiusM, P.z());
        tri[2] = QVector3D(P.x() - radiusM, P.y(),           P.z());
        tri[3] = QVector3D(P.x(),           P.y() - radiusM, P.z());
        kmath::MatrixParams actual(lastMatParams_);
        kmath::MatrixParams mp = kmath::getMatrixParams(tri);
        if (mp.isValid()) {
            concatenateMatrixParameters(actual, mp);
            if (surfaceMeshPtr_->concatenate(actual)) {
                lastMatParams_ = actual;
            }
        }
    };

    auto dirForIndexPix = [&](int idx, QVector2D& udirPix) -> bool { // направление движения
        const QVector3D& p  = bTrData[idx];
        const QVector3D  pP = surfaceMeshPtr_->convertPhToPixCoords(p);

        QVector2D v0(0.f, 0.f); // prev -> curr
        QVector2D v1(0.f, 0.f); // curr -> next

        if (idx > 0) {
            const QVector3D& pr = bTrData[idx - 1];
            const QVector3D  prP = surfaceMeshPtr_->convertPhToPixCoords(pr);
            v0 = QVector2D(pP.x() - prP.x(), pP.y() - prP.y());
            if (v0.lengthSquared() > kmath::fltEps) {
                v0.normalize();
            }
        }

        if (idx + 1 < bTrData.size()) {
            const QVector3D& nx = bTrData[idx + 1];
            const QVector3D  nxP = surfaceMeshPtr_->convertPhToPixCoords(nx);
            v1 = QVector2D(nxP.x() - pP.x(), nxP.y() - pP.y());
            if (v1.lengthSquared() > kmath::fltEps) {
                v1.normalize();
            }
        }

        if (v0.lengthSquared() <= kmath::fltEps && v1.lengthSquared() <= kmath::fltEps) {
            return false;
        }

        QVector2D v = v1.lengthSquared() > kmath::fltEps && v0.lengthSquared() > kmath::fltEps ? (v0 + v1) : (v1.lengthSquared() > kmath::fltEps ? v1 : v0);
        if (v.lengthSquared() <= kmath::fltEps) {
            return false;
        }

        udirPix = v.normalized();
        return true;
    };


    auto paintDiskExtrapolated = [&](const QVector3D& P, const QVector2D* udirOpt, QSet<SurfaceTile*>& changed) { // покраска в радиусе extraWidth_/2
        if (extraWidth_ <= 0) {
            return;
        }

        const float radiusM = 0.5f * static_cast<float>(extraWidth_);
        ensureMeshCoversDisk(P, radiusM); // расширяем меш под диск

        QVector3D Ppix = surfaceMeshPtr_->convertPhToPixCoords(P);
        QVector3D Qpix = surfaceMeshPtr_->convertPhToPixCoords(QVector3D(P.x() + radiusM, P.y(), P.z()));
        const float radiusPxF = std::fabs(Qpix.x() - Ppix.x());
        const int   radiusPx  = std::max(1, static_cast<int>(std::round(radiusPxF)));

        const int stepPix     = surfaceMeshPtr_->getStepSizeHeightMatrix();
        const int tileSidePix = surfaceMeshPtr_->getTileSidePixelSize();
        const int hvSide      = tileSidePix / stepPix + 1;
        const int tilesY      = surfaceMeshPtr_->getNumHeightTiles();
        const int meshW       = surfaceMeshPtr_->getPixelWidth();
        const int meshH       = surfaceMeshPtr_->getPixelHeight();

        int minPx = std::clamp(((int)std::floor(Ppix.x()) - radiusPx) / stepPix * stepPix, 0, meshW - 1);
        int maxPx = std::clamp(((int)std::ceil (Ppix.x()) + radiusPx) / stepPix * stepPix, 0, meshW - 1);
        int minPy = std::clamp(((int)std::floor(Ppix.y()) - radiusPx) / stepPix * stepPix, 0, meshH - 1);
        int maxPy = std::clamp(((int)std::ceil (Ppix.y()) + radiusPx) / stepPix * stepPix, 0, meshH - 1);

        const int cx = static_cast<int>(std::round(Ppix.x()));
        const int cy = static_cast<int>(std::round(Ppix.y()));
        const int r2 = radiusPx * radiusPx;

        const bool useDir = (udirOpt && udirOpt->lengthSquared() > 0.f);
        const float ux = useDir ? udirOpt->x() : 0.f;
        const float uy = useDir ? udirOpt->y() : 0.f;

        for (int py = minPy; py <= maxPy; py += stepPix) {
            for (int px = minPx; px <= maxPx; px += stepPix) {
                const int dx = px - cx;
                const int dy = py - cy;
                if (dx * dx + dy * dy > r2) {
                    continue; // вне круга
                }
                if (dx == 0 && dy == 0) {
                    continue; // центр
                }


                bool isFront = true; // круг на forward/backward
                if (useDir) {
                    const float dot = static_cast<float>(dx) * ux + static_cast<float>(dy) * uy;
                    isFront = (dot >= 0.f);
                }

                int tileX = px / tileSidePix;
                int tileY = (tilesY - 1) - py / tileSidePix;
                int locX  = px % tileSidePix;
                int locY  = py % tileSidePix;
                int hvIdx = (locY / stepPix) * hvSide + (locX / stepPix);

                SurfaceTile* tile = surfaceMeshPtr_->getTileMatrixRef()[tileY][tileX];
                if (!tile->getIsInited()) {
                    tile->init(tileSidePix, tileHeightMatrixRatio_, tileResolution_);
                }

                auto& mark = tile->getHeightMarkVerticesRef()[hvIdx];

                if (isFront) {
                    if (mark == HeightType::kTriangulation || mark == HeightType::kMosaic) {
                        continue;
                    }
                }
                else {
                    if (mark != HeightType::kUndefined) {
                        continue; // сзади — только Undefined
                    }
                }

                tile->getHeightVerticesRef()[hvIdx][2] = P.z();
                mark = HeightType::kExrtapolation;

                tile->setIsUpdated(true);
                changed.insert(tile);
            }
        }
    };

    auto processOneCenter = [&](const QVector3D& pnt) -> void {
        if (!originSet_) {
            origin_    = { pnt.x(), pnt.y() };
            originSet_ = true;
        }

        const int ix = qRound((pnt.x() - origin_.x()) / cellPx_);
        const int iy = qRound((pnt.y() - origin_.y()) / cellPx_);
        const QPair<int,int> cid(ix, iy);

        if (auto it = cellPointsInTri_.find(cid); it != cellPointsInTri_.end()) { // обновление Z координаты у центральной точки
            const int pIdx = it.value();
            auto& dp = delaunayProc_.getPointsRef()[pIdx];
            if (!qFuzzyCompare(static_cast<float>(dp.z), pnt.z())) {
                dp.z = pnt.z();
                beenManualChanged = true;
                for (int triIdx : pointToTris_.value(pIdx)) {
                    updsTrIndx.insert(triIdx);
                }
            }
        }
        else { // добавление новой точки
            const auto res = delaunayProc_.addPoint({ pnt.x(), pnt.y(), pnt.z() });
            for (int triIdx : res.newTriIdx) {
                registerTriangle(triIdx);
                updsTrIndx.insert(triIdx);
            }
            cellPointsInTri_[cid] = res.pointIdx;
        }
    };

    for (int itm : indxs) { // добавление в триангуляцию
        const QVector3D& point = bTrData[itm];
        if (!std::isfinite(point.z())) continue;
        processOneCenter(point);
    }

    const int triCount = static_cast<int>(tr.size());
    if (!triCount) {
        dataProcessor_->changeState(DataProcessorType::kUndefined);
        return;
    }

    float lastMinZ = minZ_;
    float lastMaxZ = maxZ_;
    for (int triIdx : std::as_const(updsTrIndx)) { // трассировака треугольников в меш
        if (triIdx < 0 || triIdx >= triCount) {
            continue;
        }

        const auto& t = tr[triIdx];
        const bool inWork = !(t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_);
        if (!inWork) {
            continue;
        }

        QVector<QVector3D> pts(3, QVector3D(0.0f, 0.0f, 0.0f));
        pts[0] = kmath::fvec(pt[t.a]);
        pts[1] = kmath::fvec(pt[t.b]);
        pts[2] = kmath::fvec(pt[t.c]);

        kmath::MatrixParams actualMatParams(lastMatParams_);
        kmath::MatrixParams newMatrixParams = kmath::getMatrixParams(pts);
        if (newMatrixParams.isValid()) {
            concatenateMatrixParameters(actualMatParams, newMatrixParams);
            if (surfaceMeshPtr_->concatenate(actualMatParams)) {
                lastMatParams_ = actualMatParams;
            }
        }

        writeTriangleToMesh(pts[0], pts[1], pts[2], changedTiles);

        minZ_ = std::min(static_cast<double>(minZ_), std::min({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
        maxZ_ = std::max(static_cast<double>(maxZ_), std::max({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
    }

    for (int itm : indxs) { // экстраполяция кругом
        const QVector3D& point = bTrData[itm];
        QVector2D udirPix;
        const bool haveDir = dirForIndexPix(itm, udirPix);
        paintDiskExtrapolated(point, haveDir ? &udirPix : nullptr, changedTiles);
    }

    propagateBorderHeights(changedTiles);
    for (SurfaceTile* t : std::as_const(changedTiles)) {
        t->updateHeightIndices();
        t->setIsUpdated(false);
    }

    if (beenManualChanged) {
        float currMin = std::numeric_limits<float>::max();
        float currMax = std::numeric_limits<float>::lowest();
        for (const auto& t : tr) {
            const bool inWork = !(t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_);
            if (!inWork) {
                continue;
            }
            currMin = std::fmin(currMin, std::min({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
            currMax = std::fmax(currMax, std::max({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
        }
        if (currMin != std::numeric_limits<float>::max()) {
            minZ_ = currMin;
        }
        if (currMax != std::numeric_limits<float>::lowest()) {
            maxZ_ = currMax;
        }
    }

    const bool zChanged = !qFuzzyCompare(1.0 + minZ_, 1.0 + lastMinZ) || !qFuzzyCompare(1.0 + maxZ_, 1.0 + lastMaxZ);
    if (zChanged) {
        rebuildColorIntervals();
        dataProcessor_->setMinZ(minZ_);
        dataProcessor_->setMaxZ(maxZ_);
        emit dataProcessor_->sendSurfaceMinZ(minZ_);
        emit dataProcessor_->sendSurfaceMaxZ(maxZ_);
    }

    QHash<QUuid, SurfaceTile> res;
    res.reserve(changedTiles.size());
    for (auto it = changedTiles.cbegin(); it != changedTiles.cend(); ++it) {
        res.insert((*it)->getUuid(), (*(*it)));
    }
    emit dataProcessor_->sendMosaicTiles(res, false);

    dataProcessor_->changeState(DataProcessorType::kUndefined);
}

void SurfaceProcessor::setTileResolution(float tileResolution)
{
    tileResolution_ = tileResolution;
}

void SurfaceProcessor::setEdgeLimit(float val)
{
    edgeLimit_ = val;

    refreshAfterEdgeLimitChange();
}

void SurfaceProcessor::rebuildColorIntervals()
{
    int levelCount = static_cast<int>(((maxZ_ - minZ_) / surfaceStepSize_) + 1);
    if (levelCount <= 0) {
        return;
    }

    colorIntervals_.clear();
    QVector<QVector3D> palette = generateExpandedPalette(levelCount);
    colorIntervals_.reserve(levelCount);

    for (int i = 0; i < levelCount; ++i) {
        colorIntervals_.append({ minZ_ + i * surfaceStepSize_, palette[i] });
    }

    emit dataProcessor_->sendSurfaceColorIntervalsSize(static_cast<int>(colorIntervals_.size()));
    emit dataProcessor_->sendSurfaceStepSize(surfaceStepSize_);

    updateTexture();
}

void SurfaceProcessor::setSurfaceStepSize(float val)
{
    surfaceStepSize_ = val;
}

void SurfaceProcessor::setThemeId(int val)
{
    themeId_ = val;
}

void SurfaceProcessor::setExtraWidth(int val)
{
    extraWidth_ = val;
}

float SurfaceProcessor::getEdgeLimit() const
{
    return edgeLimit_;
}

float SurfaceProcessor::getSurfaceStepSize() const
{
    return surfaceStepSize_;
}

int SurfaceProcessor::getThemeId() const
{
    return themeId_;
}

int SurfaceProcessor::getExtraWidth() const
{
    return extraWidth_;
}

void SurfaceProcessor::writeTriangleToMesh(const QVector3D &A, const QVector3D &B, const QVector3D &C, QSet<SurfaceTile*> &updatedTiles)
{
    if (!surfaceMeshPtr_ || !surfaceMeshPtr_->getIsInited()) {
        return;
    }

    const int stepPix     = surfaceMeshPtr_->getStepSizeHeightMatrix();
    const int tileSidePix = surfaceMeshPtr_->getTileSidePixelSize();
    const int hvSide      = tileSidePix / stepPix + 1;
    const int tilesY      = surfaceMeshPtr_->getNumHeightTiles();
    const int meshW       = surfaceMeshPtr_->getPixelWidth();
    const int meshH       = surfaceMeshPtr_->getPixelHeight();

    QVector3D Ap = surfaceMeshPtr_->convertPhToPixCoords(A);
    QVector3D Bp = surfaceMeshPtr_->convertPhToPixCoords(B);
    QVector3D Cp = surfaceMeshPtr_->convertPhToPixCoords(C);

    int minPx = std::floor(std::min({ Ap.x(), Bp.x(), Cp.x() })) - stepPix; // описывающий прямоугольник с запасом stepPix (вершина на границе)
    int maxPx = std::ceil (std::max({ Ap.x(), Bp.x(), Cp.x() })) + stepPix;
    int minPy = std::floor(std::min({ Ap.y(), Bp.y(), Cp.y() })) - stepPix;
    int maxPy = std::ceil (std::max({ Ap.y(), Bp.y(), Cp.y() })) + stepPix;
    minPx = std::clamp((minPx / stepPix) * stepPix, 0, meshW - 1); // сдвигаем описывающий прямоугольник на сетку (кратность stepPix)
    maxPx = std::clamp((maxPx / stepPix) * stepPix, 0, meshW - 1);
    minPy = std::clamp((minPy / stepPix) * stepPix, 0, meshH - 1);
    maxPy = std::clamp((maxPy / stepPix) * stepPix, 0, meshH - 1);

    const float denom = kmath::twiceArea(Ap, Bp, Cp);
    if (qFuzzyIsNull(denom)) { // вырожденный треугольник
        return;
    }

    for (int py = minPy; py <= maxPy; py += stepPix) {
        for (int px = minPx; px <= maxPx; px += stepPix) {
            QVector3D Pp(px, py, 0.f);

            float w0 = kmath::twiceArea(Bp, Cp, Pp) / denom;
            float w1 = kmath::twiceArea(Cp, Ap, Pp) / denom;
            float w2 = 1.f - w0 - w1;

            if (w0 < -kmath::fltEps || w1 < -kmath::fltEps || w2 < -kmath::fltEps) { // вне треугольника
                continue;
            }

            float interpZ = w0 * A.z() + w1 * B.z() + w2 * C.z();

            int tileX = px / tileSidePix;
            int tileY = (tilesY - 1) - py / tileSidePix;
            int locX  = px % tileSidePix;
            int locY  = py % tileSidePix;
            int hvIdx = (locY / stepPix) * hvSide + (locX / stepPix);

            SurfaceTile* tile = surfaceMeshPtr_->getTileMatrixRef()[tileY][tileX];
            if (!tile->getIsInited()) {
                tile->init(tileSidePix, tileHeightMatrixRatio_, tileResolution_);
            }

            tile->getHeightVerticesRef()[hvIdx][2]  = interpZ;
            tile->getHeightMarkVerticesRef()[hvIdx] = HeightType::kTriangulation;
            tile->setIsUpdated(true);
            updatedTiles.insert(tile);
        }
    }
}

QVector<QVector3D> SurfaceProcessor::generateExpandedPalette(int totalColors) const
{
    const auto &palette = colorPalette(themeId_);
    const int paletteSize = palette.size();

    QVector<QVector3D> retVal;

    if (totalColors <= 1 || paletteSize == 0) {
        retVal.append(paletteSize > 0 ? palette.first() : QVector3D(1.0f, 1.0f, 1.0f)); // fallback: white
        return retVal;
    }

    retVal.reserve(totalColors);

    for (int i = 0; i < totalColors; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(totalColors - 1);
        float ft = t * (paletteSize - 1);
        int i0 = static_cast<int>(ft);
        int i1 = std::min(i0 + 1, paletteSize - 1);
        float l = ft - static_cast<float>(i0);
        retVal.append((1.f - l) * palette[i0] + l * palette[i1]);
    }

    return retVal;
}

void SurfaceProcessor::updateTexture() const
{
    int paletteSize = colorIntervals_.size();
    if (paletteSize == 0) {
        return;
    }

    QVector<uint8_t> textureTask;
    textureTask.resize(paletteSize * 4);
    for (int i = 0; i < paletteSize; ++i) {
        const QVector3D &c = colorIntervals_[i].color;
        textureTask[i * 4 + 0] = static_cast<uint8_t>(qBound(0.f, c.x() * 255.f, 255.f));
        textureTask[i * 4 + 1] = static_cast<uint8_t>(qBound(0.f, c.y() * 255.f, 255.f));
        textureTask[i * 4 + 2] = static_cast<uint8_t>(qBound(0.f, c.z() * 255.f, 255.f));
        textureTask[i * 4 + 3] = 255;
    }

    emit dataProcessor_->sendSurfaceTextureTask(textureTask);
}

void SurfaceProcessor::propagateBorderHeights(QSet<SurfaceTile*>& changedTiles)
{
    if (!surfaceMeshPtr_ || !surfaceMeshPtr_->getIsInited()) {
        return;
    }

    const int stepPix  = surfaceMeshPtr_->getStepSizeHeightMatrix();
    const int hvSide   = surfaceMeshPtr_->getTileSidePixelSize() / stepPix + 1;
    const int tilesY   = surfaceMeshPtr_->getNumHeightTiles();
    const int tilesX   = surfaceMeshPtr_->getNumWidthTiles();

    auto& matrix = surfaceMeshPtr_->getTileMatrixRef();

    auto copyRow = [&](SurfaceTile* src, SurfaceTile* dst, int rowFrom, int rowTo) {
        auto& vSrc = src->getHeightVerticesRef();
        auto& vDst = dst->getHeightVerticesRef();
        auto& mDst = dst->getHeightMarkVerticesRef();
        for (int k = 0; k < hvSide; ++k) {
            int iFrom = rowFrom * hvSide + k;
            int iTo   = rowTo   * hvSide + k;
            if (!qFuzzyIsNull(vSrc[iFrom].z())) {
                vDst[iTo][2] = vSrc[iFrom][2];
                mDst[iTo]    = HeightType::kExrtapolation;
            }
        }
    };

    auto copyCol = [&](SurfaceTile* src, SurfaceTile* dst, int colFrom, int colTo) {
        auto& vSrc = src->getHeightVerticesRef();
        auto& vDst = dst->getHeightVerticesRef();
        auto& mDst = dst->getHeightMarkVerticesRef();
        for (int k = 0; k < hvSide; ++k) {
            int iFrom = k * hvSide + colFrom;
            int iTo   = k * hvSide + colTo;
            if (!qFuzzyIsNull(vSrc[iFrom].z())) {
                vDst[iTo][2] = vSrc[iFrom][2];
                mDst[iTo]    = HeightType::kExrtapolation;
            }
        }
    };

    for (int ty = 0; ty < tilesY; ++ty) {
        for (int tx = 0; tx < tilesX; ++tx) {
            SurfaceTile* t = matrix[ty][tx];
            if (!t->getIsUpdated()) {
                continue;
            }

            if (ty + 1 < tilesY) { // вверх, строка 0 в последнюю верхнего тайла
                SurfaceTile* top = matrix[ty + 1][tx];
                if (!top->getIsInited()) {
                    top->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                }
                copyRow(t, top, 0, hvSide - 1);
                top->setIsUpdated(true);
                changedTiles.insert(top);
            }

            if (tx > 0) { // влево, столбец 0 в правый столбец левого тайла
                SurfaceTile* left = matrix[ty][tx - 1];
                if (!left->getIsInited()) {
                    left->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                }
                copyCol(t, left, 0, hvSide - 1);
                left->setIsUpdated(true);
                changedTiles.insert(left);
            }

            if (ty + 1 < tilesY && tx > 0) { // диагональный узел
                SurfaceTile* topLeft = matrix[ty + 1][tx - 1];
                if (!topLeft->getIsInited()) {
                    topLeft->init(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                }
                auto& vSrc = t->getHeightVerticesRef();
                auto& vDst = topLeft->getHeightVerticesRef();
                auto& mDst = topLeft->getHeightMarkVerticesRef();

                const int srcTL = 0;
                const int dstBR = hvSide * hvSide - 1;
                if (!qFuzzyIsNull(vSrc[srcTL].z())) {
                    vDst[dstBR][2] = vSrc[srcTL][2];
                    mDst[dstBR]    = HeightType::kExrtapolation;
                    topLeft->setIsUpdated(true);
                    changedTiles.insert(topLeft);
                }
            }
        }
    }
}

void SurfaceProcessor::refreshAfterEdgeLimitChange()
{
    if (!surfaceMeshPtr_ || !surfaceMeshPtr_->getIsInited()) {
        return;
    }

    surfaceMeshPtr_->clearHeightData(); // TODO: conflict with mosaic heights

    const auto& tr = delaunayProc_.getTriangles();
    const auto& pt = delaunayProc_.getPoints();
    QSet<SurfaceTile*> changedTiles;

    for (std::size_t i = 0; i < tr.size(); ++i) {
        const auto& t = tr[i];
        if (t.is_bad || t.a < 4 || t.b < 4 || t.c < 4) {
            continue;
        }
        if (t.longest_edge_dist > edgeLimit_) {
            continue;
        }

        QVector3D A = kmath::fvec(pt[t.a]);
        QVector3D B = kmath::fvec(pt[t.b]);
        QVector3D C = kmath::fvec(pt[t.c]);
        writeTriangleToMesh(A,B,C, changedTiles);
    }

    propagateBorderHeights(changedTiles);

    for (SurfaceTile* t : std::as_const(changedTiles)) {
        t->updateHeightIndices();
    }

    float lastMinZ = minZ_;
    float lastMaxZ = maxZ_;

    float currMin = std::numeric_limits<float>::max();
    float currMax = std::numeric_limits<float>::lowest();
    for (auto t : tr) {
        bool inWork = !(t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_);
        if (!inWork) {
            continue;
        }
        currMin = std::fmin(currMin, std::min({ pt[t.a].z, pt[t.b].z , pt[t.c].z }));
        currMax = std::fmax(currMax, std::max({ pt[t.a].z, pt[t.b].z , pt[t.c].z }));
    }
    if (currMin != std::numeric_limits<float>::max()) {
        minZ_ = currMin;
    }
    if (currMax != std::numeric_limits<float>::lowest()) {
        maxZ_ = currMax;
    }

    const bool zChanged = !qFuzzyCompare(1.0 + minZ_, 1.0 + lastMinZ) || !qFuzzyCompare(1.0 + maxZ_, 1.0 + lastMaxZ);
    if (zChanged) {
        rebuildColorIntervals();
        dataProcessor_->setMinZ(minZ_);
        dataProcessor_->setMaxZ(maxZ_);
        emit dataProcessor_->sendSurfaceMinZ(minZ_);
        emit dataProcessor_->sendSurfaceMaxZ(maxZ_);
    }

    // to SurfaceView все тайлы
    const auto& tilesRef = surfaceMeshPtr_->getTilesCRef();
    QHash<QUuid, SurfaceTile> res;
    res.reserve(tilesRef.size());
    for (auto it = tilesRef.cbegin(); it != tilesRef.cend(); ++it) {
        res.insert((*it)->getUuid(), (*(*it)));
    }
    emit dataProcessor_->sendMosaicTiles(res, false);
}
