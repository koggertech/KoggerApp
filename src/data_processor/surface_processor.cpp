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
    lastCellPoint_({ -1, -1 }),
    tileResolution_(defaultTileResolution),
    minZ_(std::numeric_limits<float>::max()),
    maxZ_(std::numeric_limits<float>::lowest()),
    edgeLimit_(20.0f),
    tileSidePixelSize_(defaultTileSidePixelSize),
    tileHeightMatrixRatio_(defaultTileHeightMatrixRatio),
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
    pointToTris_.clear();
    cellPoints_.clear();
    cellPointsInTri_.clear();
    bTrToTrIndxs_.clear();
    origin_ = QPointF();
    lastCellPoint_ = QPair<int,int>();
    minZ_ = std::numeric_limits<float>::max();
    maxZ_ = std::numeric_limits<float>::lowest();
    originSet_ = false;
    lastMatParams_ = kmath::MatrixParams();
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
    //qDebug() << "SurfaceProcessor::onUpdatedBottomTrackData";

    if (indxs.empty()) {
        return;
    }

    // Delaunay processing
    QVector<QVector3D> bTrData;
    {
        QReadLocker rl(&lock_);
        bTrData = bottomTrackPtr_->cdata();
    }    
    if (bTrData.empty()) {
        return;
    }

    dataProcessor_->changeState(DataProcessorType::kSurface);
    auto &tr = delaunayProc_.getTriangles();
    auto &pt = delaunayProc_.getPoints();

    const auto registerTriangle = [&](int triIdx) {
        const auto &t = tr[triIdx];
        pointToTris_[t.a] << triIdx;
        pointToTris_[t.b] << triIdx;
        pointToTris_[t.c] << triIdx;
    };

    QSet<int> updsTrIndx;
    bool beenManualChanged = false;

    for (int itm : indxs) {
        const auto &point = bTrData[itm];

        if (!std::isfinite(point.z())) {
            continue;
        }

        if (bTrToTrIndxs_.contains(itm)) { // точка уже была в триангуляции
            const uint64_t pIdx = bTrToTrIndxs_[itm];
            auto &p = delaunayProc_.getPointsRef()[pIdx];

            if (!qFuzzyCompare(float(p.z), point.z())) {
                p.z = point.z();
                beenManualChanged = true;
                for (int triIdx : pointToTris_.value(pIdx)) {
                    updsTrIndx.insert(triIdx);
                }
            }
            continue;
        }

        if (!originSet_) {
            origin_ = { point.x(), point.y() };
            originSet_ = true;
        }

        const int ix = qRound((point.x() - origin_.x()) / cellPx_);
        const int iy = qRound((point.y() - origin_.y()) / cellPx_);
        const QPair<int,int> cid(ix, iy);
        const QPointF center(origin_.x() + float(ix) * cellPx_, origin_.y() + float(iy) * cellPx_);

        if (cellPoints_.contains(cid)) {
            if (!cellPointsInTri_.contains(cid)) {
                const auto &lastPoint = cellPoints_[cid];
                const bool currNearest = kmath::dist2({ point.x(), point.y() }, center) < kmath::dist2({ lastPoint.x(), lastPoint.y() }, center);

                if (currNearest) {
                    cellPoints_[cid] = point;
                }
                else {
                    const auto res = delaunayProc_.addPoint({ lastPoint.x(), lastPoint.y(), lastPoint.z() });
                    for (int triIdx : res.newTriIdx) {
                        registerTriangle(triIdx);
                        updsTrIndx.insert(triIdx);
                    }

                    bTrToTrIndxs_[itm]   = res.pointIdx;
                    cellPointsInTri_[cid] = res.pointIdx;
                }
            }
        }
        else {
            cellPoints_[cid] = point;
        }

        if (lastCellPoint_ != cid) {
            if (!cellPointsInTri_.contains(lastCellPoint_)) {
                const auto &lastPoint  = cellPoints_[lastCellPoint_];
                const auto res = delaunayProc_.addPoint({ lastPoint.x(), lastPoint.y(), lastPoint.z() });
                for (int triIdx : res.newTriIdx) {
                    registerTriangle(triIdx);
                    updsTrIndx.insert(triIdx);
                }

                bTrToTrIndxs_[itm] = res.pointIdx;
                cellPointsInTri_[lastCellPoint_] = res.pointIdx;
            }

            lastCellPoint_ = cid;
        }
    }

    // collect triangles
    const int triCount = tr.size();

    if (!triCount) {
        dataProcessor_->changeState(DataProcessorType::kUndefined);
        return;
    }

    float lastMinZ = minZ_;
    float lastMaxZ = maxZ_;

    static QSet<SurfaceTile*> changedTiles;
    changedTiles.clear();

    for (auto triIdx : updsTrIndx) {
        if (triIdx < 0 || triIdx >= triCount) {
            continue;
        }

        const auto &t = tr[triIdx];
        bool inWork = !(t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_);

        if (inWork) {
            // треугольник
            QVector<QVector3D> pts(3, QVector3D(0.0f, 0.0f, 0.0f));
            pts[0] = kmath::fvec(pt[t.a]);
            pts[1] = kmath::fvec(pt[t.b]);
            pts[2] = kmath::fvec(pt[t.c]);

            // update mesh size
            kmath::MatrixParams actualMatParams(lastMatParams_);
            kmath::MatrixParams newMatrixParams = kmath::getMatrixParams(pts);
            if (newMatrixParams.isValid()) {
                concatenateMatrixParameters(actualMatParams, newMatrixParams);
                if (surfaceMeshPtr_->concatenate(actualMatParams)) {
                    lastMatParams_ = actualMatParams;
                }
            }

            writeTriangleToMesh(pts[0], pts[1], pts[2], changedTiles);

            // экстремумы
            minZ_ = std::min(static_cast<double>(minZ_), std::min({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
            maxZ_ = std::max(static_cast<double>(maxZ_), std::max({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
        }
    }

    for (SurfaceTile* t : std::as_const(changedTiles)) {
        t->updateHeightIndices();
    }

    if (beenManualChanged) {
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
    }

    const bool zChanged = !qFuzzyCompare(1.0 + minZ_, 1.0 + lastMinZ) || !qFuzzyCompare(1.0 + maxZ_, 1.0 + lastMaxZ);
    if (zChanged) {
        //emit dataProcessor_->sendIsobathsMinZ(minZ_); // maybe calc in BottomTrackProcessing?
        //emit dataProcessor_->sendIsobathsMaxZ(maxZ_);
    }

    dataProcessor_->changeState(DataProcessorType::kUndefined);
}

void SurfaceProcessor::setTileResolution(float tileResolution)
{
    tileResolution_ = tileResolution;
}

void SurfaceProcessor::setEdgeLimit(float val)
{
    edgeLimit_ = val;
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
            tile->setIsPostUpdate(true);
            updatedTiles.insert(tile);
        }
    }
}
