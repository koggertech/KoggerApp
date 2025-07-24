#include "isobaths_processor.h"

#include <QDebug>
#include "data_processor.h"
#include "bottom_track.h"


IsobathsProcessor::IsobathsProcessor(DataProcessor* parent) :
    dataProcessor_(parent),
    datasetPtr_(nullptr),
    minZ_(std::numeric_limits<float>::max()),
    maxZ_(std::numeric_limits<float>::lowest()),
    levelStep_(3.0f),
    lineStepSize_(3.0f),
    bottomTrackPtr_(nullptr),
    originSet_ (false),
    cellPx_(1),
    surfaceStepSize_(1.0f),
    labelStepSize_(100.0f),
    themeId_(0),
    edgeLimit_(20.0f)
{
}

IsobathsProcessor::~IsobathsProcessor()
{
}

void IsobathsProcessor::clear()
{
    // external buffs
    pts_.clear();
    edgePts_.clear();
    minZ_ = std::numeric_limits<float>::max();
    maxZ_ = std::numeric_limits<float>::lowest();
    colorIntervals_.clear();
    lineSegments_.clear();
    labels_.clear();

    // internal buffs
    del_ = delaunay::Delaunay();
    bTrToTrIndxs_.clear();
    originSet_ = false;
    cellPoints_.clear();
    cellPointsInTri_.clear();
    lastCellPoint_ = QPair<int,int>();
    pointToTris_.clear();
    origin_ = QPointF();
    isoState_.clear();

}

void IsobathsProcessor::onUpdatedBottomTrackData(const QVector<int> &indxs)
{
    if (indxs.empty()) {
        return;
    }

    dataProcessor_->changeState(DataProcessorType::kIsobaths);

    auto &tr = del_.getTriangles();
    auto &pt = del_.getPoints();

    QVector<QVector3D> bTrData;
    {
        QReadLocker rl(&lock_);
        bTrData = bottomTrackPtr_->cdata();//
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
        //qDebug() << "ix" <<ix;
        //qDebug() << "iy" <<iy;
        //qDebug() << "cellPx_" <<cellPx_;

        if (cellPoints_.contains(cid)) {
            if (!cellPointsInTri_.contains(cid)) {
                const auto &lastPoint = cellPoints_[cid];
                const bool currNearest = dist2({ point.x(), point.y() }, center) < dist2({ lastPoint.x(), lastPoint.y() }, center);
                //qDebug() << "point" <<point;
                //qDebug() << "lastPoint" <<lastPoint;

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

    if (pts_.size() < triCount * 3) {
        pts_.resize(triCount * 3);
    }
    if (edgePts_.size() < triCount * 6) {
        edgePts_.resize(triCount * 6);
    }

    double newMinZ = minZ_;
    double newMaxZ = maxZ_;

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

    // TODO
    emit dataProcessor_->sendIsobathsPts(pts_);
    emit dataProcessor_->sendIsobathsEdgePts(edgePts_);



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

        emit dataProcessor_->sendIsobathsMinZ(minZ_);
        emit dataProcessor_->sendIsobathsMaxZ(maxZ_);

        rebuildColorIntervals();
        fullRebuildLinesLabels();
    }
    else if (!updsTrIndx.isEmpty()) {
        incrementalProcessLinesLabels(updsTrIndx);
    }

    dataProcessor_->changeState(DataProcessorType::kUndefined);

}

void IsobathsProcessor::rebuildColorIntervals()
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

    levelStep_ = surfaceStepSize_;

    emit dataProcessor_->sendIsobathsColorIntervalsSize(static_cast<int>(colorIntervals_.size()));
    emit dataProcessor_->sendIsobathsLevelStep(levelStep_);

    updateTexture();
}

QVector<QVector3D> IsobathsProcessor::generateExpandedPalette(int totalColors) const
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

void IsobathsProcessor::fullRebuildLinesLabels()
{
    const auto& pts = del_.getPointsRef();
    float zMin = minZ_;
    float zMax = maxZ_;
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
    QVector<LabelParameters> resLabels;

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
        QVector<LabelParameters> levelLabels;

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
    QVector<LabelParameters> filteredLabels;
    filterNearbyLabels(resLabels, filteredLabels);

    lineSegments_ = resLines;
    labels_ = filteredLabels;

    // TODO
    emit dataProcessor_->sendIsobathsLineSegments(lineSegments_);
    emit dataProcessor_->sendIsobathsLabels(labels_);
}

void IsobathsProcessor::incrementalProcessLinesLabels(const QSet<int> &updsTrIndx)
{
    const auto& tr = del_.getTriangles();
    const auto& pts = del_.getPointsRef();

    float zMin = minZ_;
    float zMax = maxZ_;
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
    QVector<LabelParameters> resLabels;

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
                    resLabels << LabelParameters{ pos, dir, qAbs(depthVal) };
                    next += labelStepSize_;
                }
                accLen += segLen;
            }
        }
    }

    QVector<LabelParameters> filtered;
    filterNearbyLabels(resLabels, filtered);

    lineSegments_ = std::move(resLines);
    labels_       = std::move(filtered);

    // TODO
    emit dataProcessor_->sendIsobathsLineSegments(lineSegments_);
    emit dataProcessor_->sendIsobathsLabels(labels_);

    isoState_.dirtyLevels.clear();
}

QVector<QVector3D> IsobathsProcessor::buildGridTriangles(const QVector<QVector3D> &pts, int gridWidth, int gridHeight) const
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

void IsobathsProcessor::buildPolylines(const IsobathsSegVec &segs, IsobathsPolylines &polylines) const
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

void IsobathsProcessor::edgeIntersection(const QVector3D &vertA, const QVector3D &vertB, float level, QVector<QVector3D> &out) const
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

void IsobathsProcessor::filterNearbyLabels(const QVector<LabelParameters> &inputData, QVector<LabelParameters> &outputData) const
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

void IsobathsProcessor::filterLinesBehindLabels(const QVector<LabelParameters> &filteredLabels, const QVector<QVector3D> &inputData, QVector<QVector3D> &outputData) const
{
    auto segmentIntersectsLabelBox = [](const QVector3D& p1, const QVector3D& p2, const LabelParameters& lbl, float width = 24.0f, float height = 5.0f) -> bool {
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

void IsobathsProcessor::rebuildTrianglesBuffers()
{
    const auto &tri = del_.getTriangles();
    const auto &pt  = del_.getPointsRef();

    const int triCnt = static_cast<int>(tri.size());
    pts_.resize (triCnt * 3);
    edgePts_.resize(triCnt * 6);

    const auto nanVec = [] {
        return QVector3D(std::numeric_limits<float>::quiet_NaN(),
                         std::numeric_limits<float>::quiet_NaN(),
                         std::numeric_limits<float>::quiet_NaN());
    };

    const auto fvec = [](const delaunay::Point &p) {
        return QVector3D(float(p.x), float(p.y), float(p.z));
    };

    minZ_ =  std::numeric_limits<float>::max();
    maxZ_ = -std::numeric_limits<float>::max();

    for (int i = 0; i < triCnt; ++i) {
        const auto &t = tri[i];
        const int  trBase = i * 3;
        const int  edgBase = i * 6;

        bool draw = !(t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > edgeLimit_);

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
        minZ_ = std::fmin(minZ_, std::min({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
        maxZ_ = std::fmax(maxZ_, std::max({ pt[t.a].z, pt[t.b].z, pt[t.c].z }));
    }


    // TODO: optimize
    emit dataProcessor_->sendIsobathsMinZ(minZ_);
    emit dataProcessor_->sendIsobathsMaxZ(maxZ_);
    emit dataProcessor_->sendIsobathsPts(pts_);
    emit dataProcessor_->sendIsobathsEdgePts(edgePts_);
}

void IsobathsProcessor::updateTexture() const
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

    emit dataProcessor_->sendIsobathsTextureTask(textureTask);
}

void IsobathsProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void IsobathsProcessor::setBottomTrackPtr(BottomTrack *bottomTrackPtr)
{
    bottomTrackPtr_ = bottomTrackPtr;
}
