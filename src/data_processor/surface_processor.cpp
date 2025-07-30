#include "isobaths_processor.h"

#include <QDebug>
#include "data_processor.h"
#include "bottom_track.h"
#include "surface_mesh.h"


SurfaceProcessor::SurfaceProcessor(DataProcessor* parent) :
    dataProcessor_(parent),
    bottomTrackPtr_(nullptr),
    surfaceMeshPtr_(nullptr),
    lastCellPoint_({ -1, -1 }),
    minZ_(std::numeric_limits<float>::max()),
    maxZ_(std::numeric_limits<float>::lowest()),
    edgeLimit_(20.0f),
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

    // TODO: trace these triangles to mest
    QVector<QVector3D> pts_; // render
    QVector<QVector3D> edgePts_; // render

    if (pts_.size() < triCount * 3) {
        pts_.resize(triCount * 3);
    }
    if (edgePts_.size() < triCount * 6) {
        edgePts_.resize(triCount * 6);
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
        const int edgBase = triIdx * 6;

        if (!draw) {
            pts_[trBase] = pts_[trBase + 1] = pts_[trBase + 2] = nanVec();
            std::fill_n(edgePts_.begin() + edgBase, 6, nanVec());
            continue;
        }

        // вершины
        pts_[trBase    ] = fvec(pt[t.a]);
        pts_[trBase + 1] = fvec(pt[t.b]);
        pts_[trBase + 2] = fvec(pt[t.c]);

        // рёбра
        edgePts_[edgBase    ] = pts_[trBase];
        edgePts_[edgBase + 1] = pts_[trBase + 1];
        edgePts_[edgBase + 2] = pts_[trBase + 1];
        edgePts_[edgBase + 3] = pts_[trBase + 2];
        edgePts_[edgBase + 4] = pts_[trBase + 2];
        edgePts_[edgBase + 5] = pts_[trBase];

        // экстремумы
        newMinZ = std::min(newMinZ, std::min({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
        newMaxZ = std::max(newMaxZ, std::max({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
    }

    // // TODO
    // emit dataProcessor_->sendIsobathsPts(pts_);
    // emit dataProcessor_->sendIsobathsEdgePts(edgePts_);

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

    if (zChanged) {
        minZ_ = newMinZ;
        maxZ_ = newMaxZ;

        //emit dataProcessor_->sendIsobathsMinZ(minZ_);
        //emit dataProcessor_->sendIsobathsMaxZ(maxZ_);
        //
        //rebuildColorIntervals();
        //fullRebuildLinesLabels();
    }
    else if (!updsTrIndx.isEmpty()) {
        //incrementalProcessLinesLabels(updsTrIndx);
    }

    dataProcessor_->changeState(DataProcessorType::kUndefined);
}
