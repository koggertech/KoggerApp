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
    lastCellByRole_({{ {-1,-1}, {-1,-1}, {-1,-1} }}),
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
    cellOwner_.clear();
    srcToTri_.clear();
    lastCellByRole_ = {{ {-1,-1}, {-1,-1}, {-1,-1} }};
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

    auto computeLR = [&](int idx, QVector3D& leftP, QVector3D& rightP) -> bool {
        constexpr float kOffsetMeters = 10.0f;
        const QVector3D& p = bTrData[idx];

        QVector2D dir2d(0.f, 0.f);
        if (idx + 1 < bTrData.size()) {
            const QVector3D& n = bTrData[idx + 1];
            dir2d = QVector2D(n.x() - p.x(), n.y() - p.y());
        }
        if (dir2d.lengthSquared() < kmath::fltEps && idx > 0) {
            const QVector3D& pr = bTrData[idx - 1];
            dir2d = QVector2D(p.x() - pr.x(), p.y() - pr.y());
        }

        if (dir2d.lengthSquared() < kmath::fltEps) {
            return false;
        }

        QVector2D u = dir2d.normalized();
        QVector2D left2d(-u.y(),  u.x());
        QVector2D rght2d( u.y(), -u.x());

        leftP  = QVector3D(p.x() + kOffsetMeters * left2d.x(), p.y() + kOffsetMeters * left2d.y(), p.z());
        rightP = QVector3D(p.x() + kOffsetMeters * rght2d.x(), p.y() + kOffsetMeters * rght2d.y(), p.z());

        return true;
    };

    auto processOneShared = [&](const QVector3D& pnt, uint64_t sourceKey, QPair<int,int>& lastCellRef) -> void {
        if (srcToTri_.contains(sourceKey)) {
            const uint64_t pIdx = srcToTri_[sourceKey];
            auto& dp = delaunayProc_.getPointsRef()[pIdx];
            if (!qFuzzyCompare(static_cast<float>(dp.z), pnt.z())) {
                dp.z = pnt.z();
                beenManualChanged = true;
                for (int triIdx : pointToTris_.value(pIdx)) {
                    updsTrIndx.insert(triIdx);
                }
            }
            return;
        }

        if (!originSet_) {
            origin_    = { pnt.x(), pnt.y() };
            originSet_ = true;
        }

        const int ix = qRound((pnt.x() - origin_.x()) / cellPx_);
        const int iy = qRound((pnt.y() - origin_.y()) / cellPx_);
        const QPair<int,int> cid(ix, iy);
        const QPointF center(origin_.x() + static_cast<float>(ix) * cellPx_, origin_.y() + static_cast<float>(iy) * cellPx_);

        const bool cellAlreadyCommitted = cellPointsInTri_.contains(cid);
        if (!cellAlreadyCommitted) {
            if (cellPoints_.contains(cid)) {
                const QVector3D& last = cellPoints_[cid];
                const bool currNearest = kmath::dist2({ pnt.x(), pnt.y() }, center) < kmath::dist2({ last.x(), last.y() }, center);

                if (currNearest) {
                    cellPoints_[cid] = pnt;
                    cellOwner_[cid]  = sourceKey;
                }
            }
            else {
                cellPoints_[cid] = pnt;
                cellOwner_[cid]  = sourceKey;
            }
        }

        if (lastCellRef != cid) {
            const bool lastValid = (lastCellRef.first != -1 || lastCellRef.second != -1);
            if (lastValid && !cellPointsInTri_.contains(lastCellRef) && cellPoints_.contains(lastCellRef)) {
                const QVector3D& commitP = cellPoints_[lastCellRef];
                const auto res = delaunayProc_.addPoint({ commitP.x(), commitP.y(), commitP.z() });
                for (int triIdx : res.newTriIdx) {
                    registerTriangle(triIdx);
                    updsTrIndx.insert(triIdx);
                }

                const uint64_t ownerKey = cellOwner_.value(lastCellRef, 0);
                if (ownerKey) {
                    srcToTri_[ownerKey] = res.pointIdx;
                }
                cellPointsInTri_[lastCellRef] = res.pointIdx;

                cellPoints_.remove(lastCellRef);
                cellOwner_.remove(lastCellRef);
            }

            lastCellRef = cid;
        }
    };

    auto updatePointsOnSegment = [&](const QVector3D& A, const QVector3D& B, float newZ) -> void { // Bresenham
        if (!originSet_) {
            return;
        }

        auto ix = [&](float x) {
            return qRound((x - origin_.x()) / cellPx_);
        };
        auto iy = [&](float y) {
            return qRound((y - origin_.y()) / cellPx_);
        };

        int x0 = ix(A.x());
        int y0 = iy(A.y());
        int x1 = ix(B.x());
        int y1 = iy(B.y());

        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        int x = x0, y = y0;
        while (true) {
            const QPair<int,int> cid(x,y);
            if (auto it = cellPointsInTri_.find(cid); it != cellPointsInTri_.end()) {
                const int pIdx = it.value();
                auto& dp = delaunayProc_.getPointsRef()[pIdx];
                if (!qFuzzyCompare(static_cast<float>(dp.z), newZ)) {
                    dp.z = newZ;
                    beenManualChanged = true;
                    for (int triIdx : pointToTris_.value(pIdx)) {
                        updsTrIndx.insert(triIdx);
                    }
                }
            }

            if (x == x1 && y == y1) {
                break;
            }

            int e2 = err << 1;
            if (e2 > -dy) {
                err -= dy;
                x   += sx;
            }
            if (e2 < dx) {
                err += dx;
                y   += sy;
            }
        }
    };

    for (int itm : indxs) {
        const QVector3D& point = bTrData[itm];
        if (!std::isfinite(point.z())) {
            continue;
        }

        processOneShared(point, makeKey(itm, PointRole::Center), lastCellByRole_[static_cast<int>(PointRole::Center)]);

        QVector3D leftP, rightP;
        if (computeLR(itm, leftP, rightP)) {
            processOneShared(leftP,  makeKey(itm, PointRole::Left),  lastCellByRole_[static_cast<int>(PointRole::Left)]);
            processOneShared(rightP, makeKey(itm, PointRole::Right), lastCellByRole_[static_cast<int>(PointRole::Right)]);

            const float centerZ = point.z();
            updatePointsOnSegment(point, leftP,  centerZ);
            updatePointsOnSegment(point, rightP, centerZ);
        }
    }

    const int triCount = static_cast<int>(tr.size());
    if (!triCount) {
        dataProcessor_->changeState(DataProcessorType::kUndefined);
        return;
    }

    float lastMinZ = minZ_;
    float lastMaxZ = maxZ_;

    static QSet<SurfaceTile*> changedTiles;
    changedTiles.clear();

    for (int triIdx : std::as_const(updsTrIndx)) {
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
            tile->getHeightMarkVerticesRef()[hvIdx] = HeightType::kIsobaths;
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
                mDst[iTo]    = HeightType::kIsobaths;
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
                mDst[iTo]    = HeightType::kIsobaths;
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
                    mDst[dstBR]    = HeightType::kIsobaths;
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
