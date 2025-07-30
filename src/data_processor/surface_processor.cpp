#include "isobaths_processor.h"

#include <cmath>
#include <QDebug>
#include "data_processor.h"
#include "bottom_track.h"
#include "surface_mesh.h"
#include "surface_tile.h"


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
    lastMatParams_ = mosaic::MatrixParams();
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

    dataProcessor_->changeState(DataProcessorType::kSurface);

    auto &tr = delaunayProc_.getTriangles();
    auto &pt = delaunayProc_.getPoints();

    QVector<QVector3D> bTrData;
    {
        QReadLocker rl(&lock_);
        bTrData = bottomTrackPtr_->cdata(); //
    }

    const auto dist2 = [](const QPointF &a, const QPointF &b) {
        double dx = a.x() - b.x();
        double dy = a.y() - b.y();
        return dx * dx + dy * dy;
    };

    const auto fvec = [](const delaunay::Point &p) {
        return QVector3D(float(p.x), float(p.y), float(p.z));
    };

    const auto nanVec = [] {
        return QVector3D(std::numeric_limits<float>::quiet_NaN(),
                         std::numeric_limits<float>::quiet_NaN(),
                         std::numeric_limits<float>::quiet_NaN());
    };

    const auto registerTriangle = [&](int triIdx) {
        const auto &t = tr[triIdx];
        pointToTris_[t.a] << triIdx;
        pointToTris_[t.b] << triIdx;
        pointToTris_[t.c] << triIdx;
    };

    QSet<int> updsTrIndx;
    bool beenManualChanged = false;

    for (int itm : indxs) { // delaunay
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
                const bool currNearest = dist2({ point.x(), point.y() }, center) < dist2({ lastPoint.x(), lastPoint.y() }, center);

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







    const int triCount = tr.size();

    QVector<QVector3D> pts;
    QVector<QVector3D> edgePts;

    if (pts.size() < triCount * 3) {
        pts.resize(triCount * 3);
    }
    if (edgePts.size() < triCount * 6) {
        edgePts.resize(triCount * 6);
    }

    double newMinZ = std::numeric_limits<float>::max();
    double newMaxZ = std::numeric_limits<float>::lowest();

    for (auto triIdx : updsTrIndx) {
        if (triIdx < 0 || triIdx >= triCount) {
            continue;
        }

        const auto &t = tr[triIdx];
        bool draw = !(t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_);

        const int trBase  = triIdx * 3;
        const int edgBase = triIdx * 3 * 2;

        if (!draw) {
            pts[trBase] = pts[trBase + 1] = pts[trBase + 2] = nanVec();
            std::fill_n(edgePts.begin() + edgBase, 6, nanVec());
            continue;
        }

        // вершины

        QVector3D p1 = fvec(pt[t.a]);
        QVector3D p2 = fvec(pt[t.b]);
        QVector3D p3  = fvec(pt[t.c]);


            pts[trBase    ] = p1;
            pts[trBase + 1] = p2;
            pts[trBase + 2] = p3;



       
        // экстремумы
        newMinZ = std::min(newMinZ, std::min({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
        newMaxZ = std::max(newMaxZ, std::max({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
    }



    // CALC Z RANGE
    if (beenManualChanged) {
        float currMin = std::numeric_limits<float>::max();
        float currMax = std::numeric_limits<float>::lowest();
        for (auto t : tr) {
            bool draw = !(t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_);
            if (!draw) {
                continue;
            }
            currMin = std::fmin(currMin, std::min({ pt[t.a].z, pt[t.b].z , pt[t.c].z }));
            currMax = std::fmax(currMax, std::max({ pt[t.a].z, pt[t.b].z , pt[t.c].z }));
        }
        if (currMin != std::numeric_limits<float>::max()) {
            newMinZ = currMin;
        }
        if (currMax != std::numeric_limits<float>::lowest()) {
            newMaxZ = currMax;
        }
    }

    const bool zChanged = !qFuzzyCompare(1.0 + minZ_, 1.0 + newMinZ) || !qFuzzyCompare(1.0 + maxZ_, 1.0 + newMaxZ);



    // UPDATE MESH BOUNDARIES
    MatrixParams actualMatParams(lastMatParams_);
    MatrixParams newMatrixParams = getMatrixParams(pts);
    if (newMatrixParams.isValid()) {
        concatenateMatrixParameters(actualMatParams, newMatrixParams);
        bool meshSizeUpdated = surfaceMeshPtr_->concatenate(actualMatParams);
        Q_UNUSED(meshSizeUpdated)
        //if (meshSizeUpdated) {
        //    qDebug() << "UPDATED MESH BOUNDARIES";
        //    qDebug() << "actual matrix:";
        //    actualMatParams.print(qDebug());
        //    qDebug() << "surface mesh:";
        //    surfaceMeshPtr_->printMatrix();
        //}
        lastMatParams_ = actualMatParams;
    }



    //auto gMeshWidthPixs = surfaceMeshPtr_->getPixelWidth(); // for bypass
    //auto gMeshHeightPixs = surfaceMeshPtr_->getPixelHeight();

    //static QSet<SurfaceTile*> changedTiles;      // переспользуем между вызовами
    //changedTiles.clear();


    //qDebug() << "TRACE HEIGHTS";
    // TRACE HEIGHT
    for (auto triIdx : updsTrIndx) {
        if (triIdx < 0 || triIdx >= triCount) {
            continue;
        }

        const auto &t = tr[triIdx];
        bool inWork = !(t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_);

        const int trBase  = triIdx * 3;
        const int edgBase = triIdx * 3 * 2;

        if (!inWork) {
            pts[trBase] = pts[trBase + 1] = pts[trBase + 2] = nanVec();
            std::fill_n(edgePts.begin() + edgBase, 6, nanVec());
            continue;
        }

        //auto p1 = pts[trBase];
        //auto p2 = pts[trBase + 1];
        //auto p3 = pts[trBase + 2];

        //qDebug() << "edges";
        //qDebug() << p11 << p12;
        //qDebug() << p21 << p22;
        //qDebug() << p31 << p32;


        //  HERE WE NEED TRACE THIS TRIANGLE TO TILES IN SURFACE MESH

        //writeTriangleToMesh(p1, p2, p3, changedTiles);  // A,B,C – любые три вершины тр‑ка
    }

    //for (SurfaceTile* t : std::as_const(changedTiles)) {
    //    t->updateHeightIndices();
    //}


    if (zChanged) {
        minZ_ = newMinZ;
        maxZ_ = newMaxZ;
        //emit dataProcessor_->sendIsobathsMinZ(minZ_);
        //emit dataProcessor_->sendIsobathsMaxZ(maxZ_);
    }

    dataProcessor_->changeState(DataProcessorType::kUndefined);
}

void SurfaceProcessor::setTileResolution(float tileResolution)
{
    tileResolution_ = tileResolution;
}

void SurfaceProcessor::writeTriangleToMesh(const QVector3D &A, const QVector3D &B, const QVector3D &C, QSet<SurfaceTile *> &updatedTiles)
{
    if (!surfaceMeshPtr_ || !surfaceMeshPtr_->getIsInited()) {
        return;
    }

    const int stepPix = surfaceMeshPtr_->getStepSizeHeightMatrix();
    const int tileSidePix = surfaceMeshPtr_->getTileSidePixelSize();
    const int hvSide = tileSidePix / stepPix + 1;
    const float res = tileResolution_;

    const int meshW = surfaceMeshPtr_->getPixelWidth();
    const int meshH = surfaceMeshPtr_->getPixelHeight();
    const int numTilesY = surfaceMeshPtr_->getNumHeightTiles();

    QVector3D Ap = surfaceMeshPtr_->convertPhToPixCoords(A);
    QVector3D Bp = surfaceMeshPtr_->convertPhToPixCoords(B);
    QVector3D Cp = surfaceMeshPtr_->convertPhToPixCoords(C);

    int minPx = std::floor(std::min({Ap.x(), Bp.x(), Cp.x()}));
    int maxPx = std::ceil (std::max({Ap.x(), Bp.x(), Cp.x()}));
    int minPy = std::floor(std::min({Ap.y(), Bp.y(), Cp.y()}));
    int maxPy = std::ceil (std::max({Ap.y(), Bp.y(), Cp.y()}));

    int startX = std::clamp((minPx / stepPix) * stepPix, 0, meshW-1);
    int endX   = std::clamp((maxPx / stepPix) * stepPix, 0, meshW-1);
    int startY = std::clamp((minPy / stepPix) * stepPix, 0, meshH-1);
    int endY   = std::clamp((maxPy / stepPix) * stepPix, 0, meshH-1);

    auto area2 = [](const QVector3D& p, const QVector3D& q, const QVector3D& r)
    { return (q.x()-p.x())*(r.y()-p.y()) - (q.y()-p.y())*(r.x()-p.x()); };

    const float denom = area2(Ap, Bp, Cp);
    if (qFuzzyIsNull(denom)) {
        return;
    }

    for (int py = startY; py <= endY; py += stepPix) {
        for (int px = startX; px <= endX; px += stepPix) {
            QVector3D Ppix(px, py, 0.0f);

            float w0 = area2(Bp, Cp, Ppix) / denom;
            float w1 = area2(Cp, Ap, Ppix) / denom;
            float w2 = 1.0f - w0 - w1;
            if (w0 < 0.f || w1 < 0.f || w2 < 0.f) {
                continue;
            }

            const float z = w0 * A.z() + w1 * B.z() + w2 * C.z();

            int tileX = px / tileSidePix;
            int tileY = (numTilesY - 1) - py / tileSidePix;

            int locX  = px % tileSidePix;
            int locY  = py % tileSidePix;
            int hvIdx = (locY / stepPix) * hvSide + (locX / stepPix);

            SurfaceTile* tile = surfaceMeshPtr_->getTileMatrixRef()[tileY][tileX];
            if (!tile->getIsInited()) {
                tile->init(tileSidePix, surfaceMeshPtr_->getStepSizeHeightMatrix()-1, res);
            }

            tile->getHeightVerticesRef()[hvIdx][2]  = z;
            tile->getHeightMarkVerticesRef()[hvIdx] = HeightType::kIsobaths;
            tile->setIsPostUpdate(true);
            updatedTiles.insert(tile);
        }
    }
}
