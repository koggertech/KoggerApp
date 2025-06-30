#include "surface_view.h"

#include <algorithm>
#include <QtMath>
#include "text_renderer.h"


SurfaceView::SurfaceView(QObject* parent)
    : SceneObject(new SurfaceViewRenderImplementation, parent),
    bottomTrackPtr_(nullptr),
    originSet_(false),
    cellPx_(1),
    surfaceStepSize_(1.0f),
    lineStepSize_(1.0f),
    labelStepSize_(100.0f),
    textureId_(0),
    toDeleteId_(0),
    themeId_(0),
    processState_(false),
    edgeLimit_(20.0f),
    updCnt_(0),
    handleXCall_(1)
{}

SurfaceView::~SurfaceView()
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        toDeleteId_ = r->textureId_;
    }
}

void SurfaceView::clear()
{
    if (workerFuture_.isRunning()) {
        workerFuture_.cancel();
        workerFuture_.waitForFinished();
    }

    {
        QMutexLocker lk(&pendingMtx_);
        pending_.clear();
    }

    auto* r = RENDER_IMPL(SurfaceView);

    r->pts_.clear();
    r->edgePts_.clear();
    r->minZ_ = std::numeric_limits<float>::max();
    r->maxZ_ = std::numeric_limits<float>::lowest();
    r->colorIntervals_.clear();
    r->textureId_ = 0;
    r->lineSegments_.clear();
    r->labels_.clear();

    del_ = delaunay::Delaunay();
    bTrToTrIndxs_.clear();
    originSet_ = false;
    cellPoints_.clear();
    cellPointsInTri_.clear();
    lastCellPoint_ = QPair<int,int>();
    pointToTris_.clear();
    textureTask_.clear();
    origin_ = QPointF();
    cellPoints_.clear();
    isoState_.clear();
    updCnt_ = 0;

    Q_EMIT changed();
}

void SurfaceView::setBottomTrackPtr(BottomTrack* ptr)
{
    bottomTrackPtr_ = ptr;
}

QVector<uint8_t> &SurfaceView::getTextureTasksRef()
{
    return textureTask_;
}

GLuint SurfaceView::getDeinitTextureTask() const
{
    return toDeleteId_;
}

GLuint SurfaceView::getTextureId() const
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        return r->textureId_;
    }

    return 0;
}

void SurfaceView::setTextureId(GLuint textureId)
{
    textureId_ = textureId;

    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->textureId_ = textureId;
    }

    Q_EMIT changed();
}

void SurfaceView::setColorTableThemeById(int id)
{
    if (themeId_ == id) {
        return;
    }

    themeId_ = id;

    rebuildColorIntervals();

    Q_EMIT changed();
}

float SurfaceView::getSurfaceStepSize() const
{
    return surfaceStepSize_;
}

void SurfaceView::setSurfaceStepSize(float val)
{
    if (qFuzzyCompare(surfaceStepSize_, val)) {
        return;
    }

    surfaceStepSize_ = val;

    rebuildColorIntervals();

    Q_EMIT changed();
}

float SurfaceView::getLineStepSize() const
{
    return lineStepSize_;
}

void SurfaceView::setLineStepSize(float val)
{
    if (qFuzzyCompare(lineStepSize_, val)) {
        return;
    }

    lineStepSize_ = val;

    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->lineStepSize_ = lineStepSize_;
    }

    enqueueWork({}, true);

    Q_EMIT changed();
}

float SurfaceView::getLabelStepSize() const
{
    return labelStepSize_;
}

void SurfaceView::setLabelStepSize(float val)
{
    if (qFuzzyCompare(labelStepSize_, val)) {
        return;
    }

    labelStepSize_ = val;

    enqueueWork({}, true);

    Q_EMIT changed();
}

void SurfaceView::setCameraDistToFocusPoint(float val)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->distToFocusPoint_ = val;
    }
}

void SurfaceView::setEdgeLimit(int val)
{
    edgeLimit_ = val;
}

void SurfaceView::setHandleXCall(int val)
{
    handleXCall_ = val;
}

void SurfaceView::onUpdatedBottomTrackData(const QVector<int>& indxs) // инкрементальное обновление ребер и вершин
{
    if (indxs.empty()) {
        return;
    }

    auto *r  = RENDER_IMPL(SurfaceView);
    auto &tr = del_.getTriangles();
    auto &pt = del_.getPoints();
    const auto &bTrDataRef = bottomTrackPtr_->cdata();

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
        const auto &point = bTrDataRef[itm];
        if (!std::isfinite(point.z())) {
            continue;
        }

        if (bTrToTrIndxs_.contains(itm)) { // точка уже была в триангуляции
            const uint64_t pIdx = bTrToTrIndxs_[itm];
            auto &p = del_.getPointsRef()[pIdx];

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
                    const auto res = del_.addPoint({ lastPoint.x(), lastPoint.y(), lastPoint.z() });
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
                const auto res = del_.addPoint({ lastPoint.x(), lastPoint.y(), lastPoint.z() });
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
    if (r->pts_.size() < triCount * 3) {
        r->pts_.resize(triCount * 3);
    }
    if (r->edgePts_.size() < triCount * 6) {
        r->edgePts_.resize(triCount * 6);
    }

    double newMinZ = r->minZ_;
    double newMaxZ = r->maxZ_;

    for (auto triIdx : updsTrIndx) {
        if (triIdx < 0 || triIdx >= triCount) {
            continue;
        }

        const auto &t = tr[triIdx];
        bool draw = !(t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_);

        const int trBase  = triIdx * 3;
        const int edgBase = triIdx * 6;

        if (!draw) {
            r->pts_[trBase] = r->pts_[trBase + 1] = r->pts_[trBase + 2] = nanVec();
            std::fill_n(r->edgePts_.begin() + edgBase, 6, nanVec());
            continue;
        }

        // вершины
        r->pts_[trBase    ] = fvec(pt[t.a]);
        r->pts_[trBase + 1] = fvec(pt[t.b]);
        r->pts_[trBase + 2] = fvec(pt[t.c]);

        // рёбра
        r->edgePts_[edgBase    ] = r->pts_[trBase];
        r->edgePts_[edgBase + 1] = r->pts_[trBase + 1];
        r->edgePts_[edgBase + 2] = r->pts_[trBase + 1];
        r->edgePts_[edgBase + 3] = r->pts_[trBase + 2];
        r->edgePts_[edgBase + 4] = r->pts_[trBase + 2];
        r->edgePts_[edgBase + 5] = r->pts_[trBase];

        // экстремумы
        newMinZ = std::min(newMinZ, std::min({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
        newMaxZ = std::max(newMaxZ, std::max({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
    }

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

    const bool zChanged = !qFuzzyCompare(1.0 + r->minZ_, 1.0 + newMinZ) || !qFuzzyCompare(1.0 + r->maxZ_, 1.0 + newMaxZ);

    if (zChanged) {
        r->minZ_ = newMinZ;
        r->maxZ_ = newMaxZ;
        rebuildColorIntervals();
        fullRebuildLinesLabels();
    }
    else if (!updsTrIndx.isEmpty()) {
        incrementalProcessLinesLabels(updsTrIndx);
    }
}

void SurfaceView::onAction()
{
    auto& pts = del_.getPoints();

    for (auto& itm : pts) {
        qDebug() << itm.x << itm.y;
    }
}

void SurfaceView::onUpdatedBottomTrackDataWrapper(const QVector<int> &indxs)
{
    if (!bottomTrackPtr_ || !processState_) {
        return;
    }

    if (++updCnt_ % handleXCall_) { // fake decimator by count
        return;
    }
    updCnt_ = 0;

    enqueueWork(indxs, false);
}

void SurfaceView::handleWorkerFinished()
  {
        Q_EMIT changed();

        {
            QMutexLocker lk(&pendingMtx_);
            if (!pending_.indxs.isEmpty() || pending_.rebuildLineLabels) {
                PendingWork copy = pending_;
                pending_ = PendingWork{};
                lk.unlock();
                enqueueWork(copy.indxs, copy.rebuildLineLabels);
            }
        }
}

void SurfaceView::fullRebuildLinesLabels()
{
    auto* r = RENDER_IMPL(SurfaceView);
    const auto& pts = del_.getPointsRef();
    float zMin = r->minZ_;
    float zMax = r->maxZ_;
    isoState_.clear();

    const int levelCnt = int((zMax - zMin) / lineStepSize_) + 1;

    // сегменты изолиний
    for (size_t idx = 0; idx < del_.getTriangles().size(); ++idx) {
        uint64_t triIdx = idx;
        const auto& t = del_.getTriangles()[triIdx];
        if (t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_) {
            continue;
        }

        QVector3D A(pts[t.a].x, pts[t.a].y, pts[t.a].z);
        QVector3D B(pts[t.b].x, pts[t.b].y, pts[t.b].z);
        QVector3D C(pts[t.c].x, pts[t.c].y, pts[t.c].z);

        for (int lvl = 0; lvl < levelCnt; ++lvl) {
            float L = zMin + lvl * lineStepSize_;
            QVector<QVector3D> segPoints;
            edgeIntersection(A, B, L, segPoints);
            edgeIntersection(B, C, L, segPoints);
            edgeIntersection(C, A, L, segPoints);

            IsobathsSegVec newSegs;
            if (segPoints.size() == 2) {
                newSegs.append(canonSeg(segPoints[0], segPoints[1]));
            }
            else if (segPoints.size() == 3) {
                if (!fuzzyEq(segPoints[0], segPoints[1])) {
                    newSegs.append(canonSeg(segPoints[0], segPoints[1]));
                }
                if (!fuzzyEq(segPoints[1], segPoints[2])) {
                    newSegs.append(canonSeg(segPoints[1], segPoints[2]));
                }
            }

            if (!newSegs.isEmpty()) {
                isoState_.hashSegsByLvl[lvl].append(newSegs);
                isoState_.triangleSegs[triIdx][lvl] = newSegs;
            }
        }
    }

    // полилинии
    for (auto it = isoState_.hashSegsByLvl.begin(); it != isoState_.hashSegsByLvl.end(); ++it) {
        buildPolylines(it.value(), isoState_.polylinesByLevel[it.key()]);
    }

    QVector<QVector3D> resLines;
    QVector<LLabelInfo> resLabels;

    // линии и подписи
    for (auto it = isoState_.polylinesByLevel.begin(); it != isoState_.polylinesByLevel.end(); ++it) {
        int level = it.key();
        const auto& polylines = it.value();

        if (polylines.isEmpty()) {
            continue;
        }

        // линии
        for (const auto& polyline : polylines) {
            for (int i = 0; i < polyline.size() - 1; ++i) {
                resLines.append(polyline[i]);
                resLines.append(polyline[i + 1]);
            }
        }

        // подписи
        float nextMark = 0.0f;
        float cumulativeShift = 0.0f;
        QVector<LLabelInfo> levelLabels;

        for (const auto& polyline : polylines) {
            QVector<float> segLen(polyline.size() - 1);
            float polylineLen = 0.0f;

            for (int i = 0; i < polyline.size() - 1; ++i) {
                segLen[i] = (polyline[i + 1] - polyline[i]).length();
                polylineLen += segLen[i];
            }

            int curSeg = 0;
            float curOff = 0.0f;
            bool placedLabel = false;

            while (nextMark < cumulativeShift + polylineLen + epsilon_) {
                while (curSeg < segLen.size() && curOff + segLen[curSeg] < nextMark - cumulativeShift - epsilon_) {
                    curOff += segLen[curSeg];
                    ++curSeg;
                }

                if (curSeg >= segLen.size()) {
                    break;
                }

                float localMark = nextMark - cumulativeShift;
                float t = (localMark - curOff) / segLen[curSeg];

                QVector3D interpPoint = polyline[curSeg] + t * (polyline[curSeg + 1] - polyline[curSeg]);
                QVector3D direction = (polyline[curSeg + 1] - polyline[curSeg]).normalized();
                direction.setZ(0);

                float depthValue = zMin + level * lineStepSize_;
                levelLabels.append({interpPoint, direction, std::fabs(depthValue)});
                nextMark += labelStepSize_;
                placedLabel = true;
            }

            if (!placedLabel && polyline.size() >= 2) {
                QVector3D point = polyline.first();
                QVector3D direction = (polyline[1] - polyline[0]).normalized();
                direction.setZ(0);
                float depthValue = zMin + level * lineStepSize_;
                levelLabels.append({point, direction, std::fabs(depthValue)});
            }

            cumulativeShift += polylineLen;
        }

        resLabels.append(levelLabels);
    }

    // фильтрация подписей
    QVector<LLabelInfo> filteredLabels;
    filterNearbyLabels(resLabels, filteredLabels);

    r->lineSegments_ = resLines;
    r->labels_ = filteredLabels;
}

void SurfaceView::incrementalProcessLinesLabels(const QSet<int> &updsTrIndx) // инкрементальное обновление линий и лейбов
{
    auto* r = RENDER_IMPL(SurfaceView);
    const auto& tr = del_.getTriangles();
    const auto& pts = del_.getPointsRef();

    float zMin = r->minZ_;
    float zMax = r->maxZ_;
    const int levelCnt = static_cast<int>((zMax - zMin) / lineStepSize_) + 1;

    // удаление старых отрезков этого треугольника
    for (int triIdx : updsTrIndx) {
        auto itTri = isoState_.triangleSegs.find(triIdx);
        if (itTri == isoState_.triangleSegs.end()) {
            continue;
        }

        for (auto itLvl = itTri->begin(); itLvl != itTri->end(); ++itLvl) {
            int lvl = itLvl.key();
            auto& bucket = isoState_.hashSegsByLvl[lvl];

            const auto& valu = itLvl.value();
            for (const auto& oldSeg : valu) {
                IsobathsSeg cs = canonSeg(oldSeg.first, oldSeg.second);
                int idx = bucket.indexOf(cs);
                if (idx != -1) {
                    bucket.remove(idx);
                }
            }
            isoState_.dirtyLevels.insert(lvl);
        }
        isoState_.triangleSegs.erase(itTri);
    }

    // добавление новых сегментов из обновлённых треугольников
    for (int triIdx : updsTrIndx) {
        const auto& t = tr[triIdx];
        if (t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_) {
            continue;
        }

        QVector3D A(pts[t.a].x, pts[t.a].y, pts[t.a].z);
        QVector3D B(pts[t.b].x, pts[t.b].y, pts[t.b].z);
        QVector3D C(pts[t.c].x, pts[t.c].y, pts[t.c].z);

        for (int lvl = 0; lvl < levelCnt; ++lvl) {
            float L = zMin + lvl * lineStepSize_;
            QVector<QVector3D> spts;
            edgeIntersection(A, B, L, spts);
            edgeIntersection(B, C, L, spts);
            edgeIntersection(C, A, L, spts);

            IsobathsSegVec newSegs;
            if (spts.size() == 2) {
                newSegs.append(canonSeg(spts[0], spts[1]));
            }
            else if (spts.size() == 3) {
                if (!fuzzyEq(spts[0], spts[1])) {
                    newSegs.append(canonSeg(spts[0], spts[1]));
                }
                if (!fuzzyEq(spts[1], spts[2])) {
                    newSegs.append(canonSeg(spts[1], spts[2]));
                }
            }

            if (newSegs.isEmpty()) {
                continue;
            }

            auto& bucket = isoState_.hashSegsByLvl[lvl];
            appendUnique(bucket, newSegs);
            isoState_.triangleSegs[triIdx][lvl] = newSegs;
            isoState_.dirtyLevels.insert(lvl);
        }
    }

    // перестройка полилиний и подписей только для dirty уровней
    for (int lvl : std::as_const(isoState_.dirtyLevels)) {
        auto& polylines = isoState_.polylinesByLevel[lvl];
        polylines.clear();
        buildPolylines(isoState_.hashSegsByLvl[lvl], polylines);
    }

    QVector<QVector3D> resLines;
    QVector<LLabelInfo> resLabels;

    for (auto it = isoState_.polylinesByLevel.begin(); it != isoState_.polylinesByLevel.end(); ++it) {
        int lvl = it.key();
        float depthVal = zMin + lvl * lineStepSize_;
        const auto& polys = it.value();

        for (const auto& poly : polys) {
            // линии
            for (int i = 0; i + 1 < poly.size(); ++i) {
                resLines << poly[i] << poly[i + 1];
            }

            // подписи
            if (poly.size() < 2) {
                continue;
            }

            float accLen = 0.f;
            float next = 0.f;
            for (int i = 0; i + 1 < poly.size(); ++i) {
                float segLen = (poly[i + 1] - poly[i]).length();
                while (accLen + segLen >= next - epsilon_) {
                    float t = (next - accLen) / segLen;
                    QVector3D pos = poly[i] + t * (poly[i + 1] - poly[i]);
                    QVector3D dir = (poly[i + 1] - poly[i]).normalized();
                    dir.setZ(0);
                    resLabels << LLabelInfo{ pos, dir, qAbs(depthVal) };
                    next += labelStepSize_;
                }
                accLen += segLen;
            }
        }
    }

    QVector<LLabelInfo> filtered;
    filterNearbyLabels(resLabels, filtered);

    r->lineSegments_ = std::move(resLines);
    r->labels_       = std::move(filtered);

    isoState_.dirtyLevels.clear();
}

void SurfaceView::rebuildColorIntervals()
{
    auto *r = RENDER_IMPL(SurfaceView);
    int levelCount = static_cast<int>(((r->maxZ_ - r->minZ_) / surfaceStepSize_) + 1);
    if (levelCount <= 0) {
        return;
    }

    r->colorIntervals_.clear();
    QVector<QVector3D> palette = generateExpandedPalette(levelCount);
    r->colorIntervals_.reserve(levelCount);

    for (int i = 0; i < levelCount; ++i) {
        r->colorIntervals_.append({r->minZ_ + i * surfaceStepSize_, palette[i]});
    }

    r->levelStep_ = surfaceStepSize_; // ???

    updateTexture();
}

QVector<QVector3D> SurfaceView::generateExpandedPalette(int totalColors) const
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

void SurfaceView::updateTexture()
{
    auto* r = RENDER_IMPL(SurfaceView);
    if (!r) {
        return;
    }

    int paletteSize = r->colorIntervals_.size();

    if (paletteSize == 0) {
        return;
    }

    textureTask_.clear();

    textureTask_.resize(paletteSize * 4);
    for (int i = 0; i < paletteSize; ++i) {
        const QVector3D &c = r->colorIntervals_[i].color;
        textureTask_[i * 4 + 0] = static_cast<uint8_t>(qBound(0.f, c.x() * 255.f, 255.f));
        textureTask_[i * 4 + 1] = static_cast<uint8_t>(qBound(0.f, c.y() * 255.f, 255.f));
        textureTask_[i * 4 + 2] = static_cast<uint8_t>(qBound(0.f, c.z() * 255.f, 255.f));
        textureTask_[i * 4 + 3] = 255;
    }
}

QVector<QVector3D> SurfaceView::buildGridTriangles(const QVector<QVector3D> &pts, int gridWidth, int gridHeight) const
{
    if (pts.size() < 3 || gridWidth <= 1 || gridHeight <= 1) {
        return {};
    }

    // Карта
    QVector<int> grid(gridWidth * gridHeight, -1);

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();

    for (const auto& v : pts) {
        minX = std::min(minX, v.x());
        minY = std::min(minY, v.y());
        maxX = std::max(maxX, v.x());
        maxY = std::max(maxY, v.y());
    }

    const float stepX = (maxX - minX) / (gridWidth - 1);
    const float stepY = (maxY - minY) / (gridHeight - 1);

    const float invStepX = 1.f / stepX;
    const float invStepY = 1.f / stepY;

    for (int k = 0; k < pts.size(); ++k) {
        int ix = int(std::round((pts[k].x() - minX) * invStepX));
        int iy = int(std::round((pts[k].y() - minY) * invStepY));
        if (ix >= 0 && ix < gridWidth && iy >= 0 && iy < gridHeight) {
            grid[ix + iy * gridWidth] = k;
        }
    }

    // Генерация треугольников по клеткам сетки
    QVector<QVector3D> triangles;
    triangles.reserve((gridWidth - 1) * (gridHeight - 1) * 6);

    auto id = [&](int x, int y) -> int {
        return grid[x + y * gridWidth];
    };

    for (int j = 0; j < gridHeight - 1; ++j) {
        for (int i = 0; i < gridWidth - 1; ++i) {
            int i00 = id(i, j);
            int i10 = id(i + 1, j);
            int i01 = id(i, j + 1);
            int i11 = id(i + 1, j + 1);
            if (i00 < 0 || i10 < 0 || i01 < 0 || i11 < 0) {
                continue;
            }
            triangles.append(pts[i00]);
            triangles.append(pts[i10]);
            triangles.append(pts[i11]);
            triangles.append(pts[i00]);
            triangles.append(pts[i11]);
            triangles.append(pts[i01]);
        }
    }

    return triangles;
}

void SurfaceView::buildPolylines(const IsobathsSegVec &segs, IsobathsPolylines &polylines) const
{
    auto eq = [](const QVector3D& a,const QVector3D& b){ return fuzzyEq(a,b); };

    QVector<bool> used(segs.size(),false);
    for (int i=0;i<segs.size();++i){
        if (used[i]) continue;
        QList<QVector3D> poly;
        poly << segs[i].first << segs[i].second;
        used[i]=true;

        bool again=true;
        while (again){
            again=false;
            for (int j=0;j<segs.size();++j){
                if (used[j]) continue;
                if (eq(poly.back(), segs[j].first )){
                    poly << segs[j].second; used[j]=true; again=true;
                }else if (eq(poly.back(), segs[j].second)){
                    poly << segs[j].first;  used[j]=true; again=true;
                }else if (eq(poly.front(), segs[j].second)){
                    poly.prepend(segs[j].first); used[j]=true; again=true;
                }else if (eq(poly.front(), segs[j].first)){
                    poly.prepend(segs[j].second); used[j]=true; again=true;
                }
            }
        }
        if (poly.size()>1) polylines.append(QVector<QVector3D>(poly.begin(), poly.end()));


    }
}

void SurfaceView::edgeIntersection(const QVector3D &vertA, const QVector3D &vertB, float level, QVector<QVector3D> &out) const
{
    const float zShift = 0.03; // смещение по Z

    float zA = vertA.z();
    float zB = vertB.z();

    QVector3D fVertA = { vertA.x(), vertA.y(), vertA.z() + zShift };
    QVector3D fVertB = { vertB.x(), vertB.y(), vertB.z() + zShift };

    if (std::fabs(zA - level) < epsilon_ && std::fabs(zB - level) < epsilon_) { // обе на уровне
        out.append(fVertA);
        out.append(fVertB);
        return;
    }
    if ((zA - level) * (zB - level) > 0.f) { // нет пересечения
        return;
    }
    if (std::fabs(zA - level) < epsilon_) { // a на уровне
        out.append(fVertA);
        return;
    }
    if (std::fabs(zB - level) < epsilon_) { // b на уровне
        out.append(fVertB);
        return;
    }

    float diff = zB - zA;
    if (!qFuzzyIsNull(diff)) {
        QVector3D intersec = (fVertA + ((level - zA) / (zB - zA)) * (fVertB - fVertA));
        if (std::isfinite(intersec.x()) && std::isfinite(intersec.y()) && std::isfinite(intersec.z())) {
            out.append(intersec);
        }
    }
}

void SurfaceView::filterNearbyLabels(const QVector<LLabelInfo> &inputData, QVector<LLabelInfo> &outputData) const
{
    const float cellSize = 20.0f;
    const float cellSizeInv = 1.0f / cellSize;
    const float minLabelDist2 = cellSize * cellSize;

    QHash<QPair<int, int>, QVector<QVector3D>> spatialGrid;

    for (const auto& label : inputData) {
        int cellX = int(std::floor(label.pos.x() * cellSizeInv));
        int cellY = int(std::floor(label.pos.y() * cellSizeInv));

        bool tooClose = false;

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                QPair<int, int> neighbor(cellX + dx, cellY + dy);

                if (spatialGrid.contains(neighbor)) {
                    const auto& list = spatialGrid[neighbor];
                    for (const auto& p : list) {
                        if ((label.pos - p).lengthSquared() < minLabelDist2) {
                            tooClose = true;
                            break;
                        }
                    }
                }

                if (tooClose) {
                    break;
                }
            }
            if (tooClose) {
                break;
            }
        }

        if (!tooClose) {
            outputData.append(label);
            spatialGrid[qMakePair(cellX, cellY)].append(label.pos);
        }
    }
}

void SurfaceView::filterLinesBehindLabels(const QVector<LLabelInfo> &filteredLabels, const QVector<QVector3D> &inputData, QVector<QVector3D> &outputData) const
{
    auto segmentIntersectsLabelBox = [](const QVector3D& p1, const QVector3D& p2, const LLabelInfo& lbl, float width = 24.0f, float height = 5.0f) -> bool {
        QVector2D a(p1.x(), p1.y());
        QVector2D b(p2.x(), p2.y());

        float labelOffset = 10;

        QVector2D right = QVector2D(-lbl.dir.y(), -lbl.dir.x()).normalized();
        QVector2D center = QVector2D(lbl.pos.x(), lbl.pos.y()) + right * labelOffset;

        QVector2D boxMin = center - QVector2D(width * 0.5f, height * 0.5f);
        QVector2D boxMax = center + QVector2D(width * 0.5f, height * 0.5f);

        float minX = std::min(a.x(), b.x());
        float maxX = std::max(a.x(), b.x());
        float minY = std::min(a.y(), b.y());
        float maxY = std::max(a.y(), b.y());

        if (maxX < boxMin.x() || minX > boxMax.x() ||
            maxY < boxMin.y() || minY > boxMax.y()) {
            return false;
        }

        QVector2D mid = (a + b) * 0.5f;
        if (mid.x() >= boxMin.x() && mid.x() <= boxMax.x() &&
            mid.y() >= boxMin.y() && mid.y() <= boxMax.y()) {
            return true;
        }

        return false;
    };

    for (int i = 0; i + 1 < inputData.size(); i += 2) {
        const QVector3D& p1 = inputData[i];
        const QVector3D& p2 = inputData[i + 1];

        bool skip = false;
        for (const auto& lbl : filteredLabels) {
            if (segmentIntersectsLabelBox(p1, p2, lbl)) {
                skip = true;
                break;
            }
        }

        if (!skip) {
            outputData.append(p1);
            outputData.append(p2);
        }
    }
}

void SurfaceView::enqueueWork(const QVector<int> &indxs, bool rebuildLinesLabels)
{
    {
        QMutexLocker lk(&pendingMtx_);
        pending_.indxs += indxs;
        pending_.rebuildLineLabels |= rebuildLinesLabels;
    }

    if (workerFuture_.isRunning()) {
        return;
    }

    workerFuture_ = QtConcurrent::run([this] {
        if (workerFuture_.isCanceled()) {
            return;
        }

        PendingWork todo;
        {
            QMutexLocker lk(&pendingMtx_);
            todo = std::move(pending_);
            pending_.clear();
        }

        // TODO: more accuracy
        if (!todo.indxs.isEmpty()) {
            onUpdatedBottomTrackData(todo.indxs);
        }
        if (todo.rebuildLineLabels) {
            fullRebuildLinesLabels();
        }
    });

    if (!workerWatcher_.isRunning()) {
        connect(&workerWatcher_, &QFutureWatcher<void>::finished, this, &SurfaceView::handleWorkerFinished, Qt::QueuedConnection);
    }

    workerWatcher_.setFuture(workerFuture_);
}

SurfaceView::SurfaceViewRenderImplementation::SurfaceViewRenderImplementation()
    : minZ_(std::numeric_limits<float>::max()),
    maxZ_(std::numeric_limits<float>::lowest()),
    trianglesVisible_(true),
    edgesVisible_(true),
    levelStep_(3.0f),
    lineStepSize_(3.0f),
    textureId_(0),
    distToFocusPoint_(10.0f),
    debugMode_(false)
{}

void SurfaceView::SurfaceViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &model, const QMatrix4x4 &view, const QMatrix4x4 &projection, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &spMap) const
{
    if (!debugMode_) {
        if (!m_isVisible ) {
            return;
        }

        if (!spMap.contains("isobaths")) {
            return;
        }

        auto& sp = spMap["isobaths"];
        if (!sp->bind()) {
            qCritical() << "isobaths shader bind failed";
            return;
        }

        QMatrix4x4 mvp = projection * view * model;

        sp->setUniformValue("matrix",        mvp);
        sp->setUniformValue("depthMin",      minZ_);
        sp->setUniformValue("levelStep",     levelStep_);
        sp->setUniformValue("levelCount",    colorIntervals_.size());
        sp->setUniformValue("linePass",      false);
        sp->setUniformValue("lineColor",     color_);

        ctx->glActiveTexture(GL_TEXTURE0);
        ctx->glBindTexture(GL_TEXTURE_2D, textureId_);
        sp->setUniformValue("paletteSampler", 0);

        int pos = sp->attributeLocation("position");
        sp->enableAttributeArray(pos);
        sp->setAttributeArray(pos, pts_.constData());
        ctx->glDrawArrays(GL_TRIANGLES, 0, pts_.size());
        sp->disableAttributeArray(pos);

        if (!lineSegments_.isEmpty()) {
            sp->setUniformValue("linePass", true);
            sp->disableAttributeArray(pos);
            sp->enableAttributeArray(pos);
            sp->setAttributeArray(pos, lineSegments_.constData());
            ctx->glLineWidth(1.f);
            ctx->glDrawArrays(GL_LINES, 0, lineSegments_.size());
            sp->disableAttributeArray(pos);
            sp->setUniformValue("linePass", false);

            if (!labels_.isEmpty()) {
                glDisable(GL_DEPTH_TEST); // TODO: artifacts

                auto oldTextColor = TextRenderer::instance().getColor();
                TextRenderer::instance().setColor(QColor(color_.x(), color_.y(), color_.z()));

                // scale
                float sizeFromStep = lineStepSize_ * 0.2f;
                float sizeFromDist = distToFocusPoint_ * 0.0015f;
                float scale = qMin(sizeFromStep, sizeFromDist);
                scale = qBound(0.15f, scale, 0.3f);

                for (const auto& lbl : labels_) {
                    QString text = QString::number(lbl.depth, 'f', 1);
                    TextRenderer::instance().render3D(text,
                                                      scale,
                                                      lbl.pos,
                                                      lbl.dir,
                                                      ctx,
                                                      mvp,
                                                      spMap);
                }

                TextRenderer::instance().setColor(oldTextColor);
                glEnable(GL_DEPTH_TEST);
            }
        }

        sp->release();
    }
    else { // debug
        if (!m_isVisible || !spMap.contains("height") || !spMap.contains("static")) {
            return;
        }

        if (trianglesVisible_) {
            if (!pts_.empty()) {
                auto shaderProgram = spMap["height"];
                if (shaderProgram->bind()) {
                    int posLoc    = shaderProgram->attributeLocation("position");
                    int maxZLoc   = shaderProgram->uniformLocation("max_z");
                    int minZLoc   = shaderProgram->uniformLocation("min_z");
                    int matrixLoc = shaderProgram->uniformLocation("matrix");

                    shaderProgram->setUniformValue(minZLoc, minZ_);
                    shaderProgram->setUniformValue(maxZLoc, maxZ_);
                    shaderProgram->setUniformValue(matrixLoc, projection * view * model);

                    shaderProgram->enableAttributeArray(posLoc);
                    shaderProgram->setAttributeArray(posLoc, pts_.constData());

                    ctx->glDrawArrays(GL_TRIANGLES, 0, pts_.size());

                    shaderProgram->disableAttributeArray(posLoc);
                    shaderProgram->release();
                }
            }
        }

        if (edgesVisible_) {
            if (!edgePts_.isEmpty()) {
                ctx->glDisable(GL_DEPTH_TEST);

                auto lineShader = spMap["static"];
                if (lineShader->bind()) {
                    int linePosLoc  = lineShader->attributeLocation("position");
                    int colorLoc    = lineShader->uniformLocation("color");
                    int matrixLoc   = lineShader->uniformLocation("matrix");
                    int widthLoc    = lineShader->uniformLocation("width");

                    lineShader->setUniformValue(matrixLoc, projection * view * model);
                    lineShader->setUniformValue(colorLoc, QVector4D(0, 0, 0, 1));
                    lineShader->setUniformValue(widthLoc, 1.0f);

                    lineShader->enableAttributeArray(linePosLoc);
                    lineShader->setAttributeArray(linePosLoc, edgePts_.constData());

                    ctx->glLineWidth(1.0f);
                    ctx->glDrawArrays(GL_LINES, 0, edgePts_.size());
                    lineShader->disableAttributeArray(linePosLoc);
                    lineShader->release();
                }
                ctx->glEnable(GL_DEPTH_TEST);
            }
        }
    }
}
