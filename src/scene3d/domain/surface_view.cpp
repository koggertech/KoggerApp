#include "surface_view.h"

#include <QtMath>
#include "text_renderer.h"

static const float epsilon_ = 1e-6f;

static const QVector<QVector3D>& colorPalette(int themeId)
{
    static const QVector<QVector<QVector3D>> palettes = {
        // 0: midnight
        {
            QVector3D(0.2f, 0.5f, 1.0f),
            QVector3D(0.2f, 0.4f, 0.9f),
            QVector3D(0.3f, 0.3f, 0.8f),
            QVector3D(0.4f, 0.2f, 0.7f),
            QVector3D(0.5f, 0.2f, 0.6f),
            QVector3D(0.6f, 0.3f, 0.5f),
            QVector3D(0.7f, 0.4f, 0.4f),
            QVector3D(0.8f, 0.5f, 0.3f),
            QVector3D(0.9f, 0.6f, 0.2f),
            QVector3D(1.0f, 0.7f, 0.1f)
        },
        // 1: default
        {
            QVector3D(0.0f, 0.0f, 0.3f),
            QVector3D(0.0f, 0.0f, 0.6f),
            QVector3D(0.0f, 0.5f, 1.0f),
            QVector3D(0.0f, 1.0f, 0.5f),
            QVector3D(1.0f, 1.0f, 0.0f),
            QVector3D(1.0f, 0.6f, 0.0f),
            QVector3D(0.8f, 0.2f, 0.0f)
        },
        // 2: blue
        {
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(0.078f, 0.020f, 0.314f),
            QVector3D(0.196f, 0.706f, 0.902f),
            QVector3D(0.745f, 0.941f, 0.980f),
            QVector3D(1.0f, 1.0f, 1.0f)
        },
        // 3: sepia
        {
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(50/255.0f, 50/255.0f, 10/255.0f),
            QVector3D(230/255.0f, 200/255.0f, 100/255.0f),
            QVector3D(255/255.0f, 255/255.0f, 220/255.0f)
        },
        // 4: colored
        {
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(40/255.0f, 0.0f, 80/255.0f),
            QVector3D(0.0f, 30/255.0f, 150/255.0f),
            QVector3D(20/255.0f, 230/255.0f, 30/255.0f),
            QVector3D(255/255.0f, 50/255.0f, 20/255.0f),
            QVector3D(255/255.0f, 255/255.0f, 255/255.0f)
        },
        // 5: bw
        {
            QVector3D(0.0f, 0.0f, 0.0f),
            QVector3D(190/255.0f, 200/255.0f, 200/255.0f),
            QVector3D(230/255.0f, 255/255.0f, 255/255.0f)
        },
        // 6: wb
        {
            QVector3D(230/255.0f, 255/255.0f, 255/255.0f),
            QVector3D(70/255.0f, 70/255.0f, 70/255.0f),
            QVector3D(0.0f, 0.0f, 0.0f)
        }
    };

    return palettes[std::clamp(themeId, 0, palettes.size() - 1)];
}


SurfaceView::SurfaceView(QObject* parent)
    : SceneObject(new SurfaceViewRenderImplementation, parent)
{}

SurfaceView::~SurfaceView()
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        toDeleteId_ = r->textureId_;
    }
}

void SurfaceView::clear()
{
    auto* r = RENDER_IMPL(SurfaceView);

    r->pts_.clear();
    r->edgePts_.clear();
    r->minZ_ = std::numeric_limits<float>::max();
    r->maxZ_ = std::numeric_limits<float>::lowest();

    r->lineSegments_.clear();
    r->labels_.clear();

    bTrToTrIndxs_.clear();

    resetTriangulation();

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

    processLinesLabels();

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

    processLinesLabels();

    Q_EMIT changed();
}

void SurfaceView::setCameraDistToFocusPoint(float val)
{
    if (auto* r = RENDER_IMPL(SurfaceView); r) {
        r->distToFocusPoint_ = val;
    }
}

void SurfaceView::onUpdatedBottomTrackData(const QVector<int>& indxs)
{
    if (!bottomTrackPtr_ || indxs.empty() || !processState_) {
        return;
    }

    auto* r = RENDER_IMPL(SurfaceView);
    const auto& bTrDataRef = bottomTrackPtr_->cdata();

    auto dist2 = [](const QPointF&a,const QPointF&b) {
        double dx = a.x()-b.x();
        double dy = a.y()-b.y();
        return dx * dx + dy * dy;
    };

    for (auto itm : indxs) {
        //if (r->m_data.size() <= itm) {
        //    r->m_data.resize(itm + 1);
        //}
        //r->m_data[itm] = bTrDataRef[itm]; // ?

        auto& point = bTrDataRef[itm];

        if (!std::isfinite(point.z())) {
            continue;
        }

        if (bTrToTrIndxs_.contains(itm)) {
            uint64_t trIndx = bTrToTrIndxs_[itm];
            del_.getPointsRef()[trIndx].z = point.z();
        }
        else {
            if (!originSet_) {
                origin_ = { point.x(), point.y() };
                originSet_ = true;
            }

            const int cellPxVal = cellPx_;
            /* индекс ячейки */
            int ix = qRound((point.x() - origin_.x()) / cellPxVal);
            int iy = qRound((point.y() - origin_.y()) / cellPxVal);
            QPair<int,int> cid(ix,iy);

            QPointF center(origin_.x() + float(ix) * cellPxVal, origin_.y() + float(iy) * cellPxVal);

            if (cellPoints_.contains(cid)) {
                if(!cellPointsInTri_.contains(cid)) {
                    auto& lastPoint = cellPoints_[cid];
                    bool currNearest = dist2({ point.x(), point.y() }, center) < dist2({ lastPoint.x(), lastPoint.y() }, center);

                    if (currNearest) {
                        cellPoints_[cid] = point;
                    } else {
                        // for low-delay: when the point is moving away from the nearest one
                        // may not have the best possible alignment
                        auto& lastPoint = cellPoints_[cid];
                        delaunay::TriResult res = del_.addPoint(delaunay::Point(lastPoint.x(),lastPoint.y(), lastPoint.z()));
                        bTrToTrIndxs_[itm] = res.pointIdx;
                        int p_idx = res.pointIdx;
                        cellPointsInTri_[cid] = p_idx;
                    }
                }
            } else {
                cellPoints_[cid] = point;
            }

            if(lastCellPoint_ != cid) {
                // check if the last cell wasn't triangulated
                if(!cellPointsInTri_.contains(lastCellPoint_)) {
                    auto& lastPoint = cellPoints_[lastCellPoint_];
                    delaunay::TriResult res = del_.addPoint(delaunay::Point(lastPoint.x(),lastPoint.y(), lastPoint.z()));
                    bTrToTrIndxs_[itm] = res.pointIdx;
                    int p_idx = res.pointIdx;
                    cellPointsInTri_[lastCellPoint_] = p_idx;
                }

                lastCellPoint_ = cid;
            }
        }
    }

    // again
    auto& pt = del_.getPoints();

    // треуг
    r->pts_.clear();

    double lastMinZ = r->minZ_;
    double lastMaxZ = r->maxZ_;

    for (const auto& t : del_.getTriangles()) {
        if (t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > 20.0) {
            continue;
        }

        r->minZ_ = std::min(static_cast<double>(r->minZ_), pt[t.a].z);
        r->minZ_ = std::min(static_cast<double>(r->minZ_), pt[t.b].z);
        r->minZ_ = std::min(static_cast<double>(r->minZ_), pt[t.c].z);
        r->maxZ_ = std::max(static_cast<double>(r->maxZ_), pt[t.a].z);
        r->maxZ_ = std::max(static_cast<double>(r->maxZ_), pt[t.b].z);
        r->maxZ_ = std::max(static_cast<double>(r->maxZ_), pt[t.c].z);

        r->pts_.append(QVector3D(pt[t.a].x, pt[t.a].y, pt[t.a].z));
        r->pts_.append(QVector3D(pt[t.b].x, pt[t.b].y, pt[t.b].z));
        r->pts_.append(QVector3D(pt[t.c].x, pt[t.c].y, pt[t.c].z));
    }

    // ребра
    r->edgePts_.clear();
    for (int i = 0; i + 2 < r->pts_.size(); i += 3) {
        const QVector3D& a = r->pts_[i];
        const QVector3D& b = r->pts_[i + 1];
        const QVector3D& c = r->pts_[i + 2];

        r->edgePts_ << a << b;
        r->edgePts_ << b << c;
        r->edgePts_ << c << a;
    }

    if (!(qFuzzyCompare(1.0 + lastMinZ, 1.0 + r->minZ_) &&
          qFuzzyCompare(1.0 + lastMaxZ, 1.0 + r->maxZ_))) {
        rebuildColorIntervals();
    }

    processLinesLabels();

    Q_EMIT changed();
}

void SurfaceView::onAction()
{
    auto& pts = del_.getPoints();

    for (auto& itm : pts) {
        qDebug() << itm.x << itm.y;
    }
}

void SurfaceView::resetTriangulation()
{
    del_ = delaunay::Delaunay();
    cellPoints_.clear();
    originSet_ = false;
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
        r->colorIntervals_.append({minDepth_ + i * surfaceStepSize_, palette[i]});
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

void SurfaceView::processLinesLabels()
{
    //result_.data.clear();

    auto* r = RENDER_IMPL(SurfaceView);
    auto& pts = del_.getPointsRef();

    float zMin = r->minZ_;
    float zMax = r->maxZ_;
    QHash<int, IsobathsSegVec> hashSegsByLvl;

    //auto& tris = ; // уже «правильный» список


    const int levelCnt = int((zMax - zMin) / lineStepSize_) + 1;

    for (const auto& t : del_.getTriangles()) {
        if (t.a < 4 || t.b < 4 || t.c < 4 || t.is_bad || t.longest_edge_dist > 20.0) {
            continue;
        }

        QVector3D A(pts[t.a].x, pts[t.a].y, pts[t.a].z);
        QVector3D B(pts[t.b].x, pts[t.b].y, pts[t.b].z);
        QVector3D C(pts[t.c].x, pts[t.c].y, pts[t.c].z);

        for (int lvl = 0; lvl < levelCnt; ++lvl) {
            float L = zMin + lvl * lineStepSize_;
            QVector<QVector3D> seg;  seg.reserve(3);

            edgeIntersection(A, B, L, seg);
            edgeIntersection(B, C, L, seg);
            edgeIntersection(C, A, L, seg);

            if (seg.size() == 2)          hashSegsByLvl[lvl].append({seg[0], seg[1]});
            else if (seg.size() == 3) {   // случай ровно по вершине
                if (seg[0] != seg[1]) hashSegsByLvl[lvl].append({seg[0], seg[1]});
                if (seg[1] != seg[2]) hashSegsByLvl[lvl].append({seg[1], seg[2]});
            }
        }
    }

    QVector<QVector3D> resLines;
    QVector<LLabelInfo> resLabels;

    for (auto it = hashSegsByLvl.begin(); it != hashSegsByLvl.end(); ++it) {
        IsobathsPolylines polylines; // несколько на уровень
        buildPolylines(it.value(), polylines);

        if (polylines.isEmpty()) {
            continue;
        }

        const auto& cPolylines = polylines;
        for (const auto& polyline : cPolylines) {
            for (int i = 0; i < polyline.size() - 1; ++i) {
                resLines.append(polyline[i]);
                resLines.append(polyline[i + 1]);
            }
        }

        // Подписи по объединённой длине
        float nextMark = 0.0f;
        float cumulativeShift = 0.0f;

        for (const auto& polyline : cPolylines) {
            // Длины сегментов текущей полилинии
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
                // Сдвигаем указатель сегмента до нужного отрезка
                while (curSeg < segLen.size() &&
                       curOff + segLen[curSeg] < nextMark - cumulativeShift - epsilon_) {
                    curOff += segLen[curSeg];
                    ++curSeg;
                }

                if (curSeg >= segLen.size()) {
                    break;
                }

                // Положение подписи внутри сегмента
                float localMark = nextMark - cumulativeShift;
                float t = (localMark - curOff) / segLen[curSeg];

                QVector3D interpPoint = polyline[curSeg] + t * (polyline[curSeg + 1] - polyline[curSeg]);
                QVector3D direction = (polyline[curSeg + 1] - polyline[curSeg]).normalized();
                direction.setZ(0);

                // смещение от линии
                //QVector3D perp(-direction.y(), direction.x(), 0.0f);
                //perp.normalize();
                //float labelOffset = 10.0f;
                //interpPoint = interpPoint + perp * labelOffset;

                resLabels.append(LLabelInfo{ interpPoint, direction, std::fabs(zMin + it.key() * lineStepSize_) });
                nextMark += labelStepSize_;
                placedLabel = true;
            }

            // Если не было поставлено ни одной метки — ставим одну в начало
            if (!placedLabel && polyline.size() >= 2) {
                QVector3D point = polyline.first();
                QVector3D direction = (polyline[1] - polyline[0]).normalized();
                direction.setZ(0);

                resLabels.append(LLabelInfo{ point, direction, std::fabs(zMin + it.key() * lineStepSize_) });
            }

            cumulativeShift += polylineLen;
        }
    }

    // filter labels
    QVector<LLabelInfo> filteredLabels;
    filteredLabels.reserve(resLabels.size());
    filterNearbyLabels(resLabels, filteredLabels);

    ////result_.labels = std::move(filteredLabels);

    // filter lines
    //QVector<QVector3D> filteredLines;
    //filteredLines.reserve(resLines.size());
    //filterLinesBehindLabels(result_.labels, resLines, filteredLines);
    //result_.data = std::move(filteredLines);

    ////result_.data = std::move(resLines);

    // test
    r->lineSegments_ = std::move(resLines); //result_.data;
    r->labels_ = std::move(filteredLabels); //result_.labels;
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
    auto isEqual = [&](const QVector3D& a,const QVector3D& b) -> bool {
        return (a - b).lengthSquared() < std::pow(epsilon_, 2);
    };

    QVector<bool> usedSegs(segs.size(), false);
    for (int s = 0; s < segs.size(); ++s) {
        if (usedSegs[s]) {
            continue;
        }

        QList<QVector3D> polyline;
        polyline.append(segs[s].first);
        polyline.append(segs[s].second);
        usedSegs[s] = true;

        bool extended = true;
        while (extended) {
            extended = false;
            for (int t = 0; t < segs.size(); ++t) {
                if (!usedSegs[t]) {
                    if (isEqual(polyline.last(), segs[t].first )) {
                        polyline.append(segs[t].second);
                        usedSegs[t] = true;
                        extended = true;
                    }
                    else if (isEqual(polyline.last(), segs[t].second)) {
                        polyline.append(segs[t].first);
                        usedSegs[t] = true;
                        extended = true;
                    }
                    else if (isEqual(polyline.first(), segs[t].second)) {
                        polyline.prepend(segs[t].first);
                        usedSegs[t] = true;
                        extended = true;
                    }
                    else if (isEqual(polyline.first(), segs[t].first)) {
                        polyline.prepend(segs[t].second);
                        usedSegs[t] = true;
                        extended = true;
                    }
                }
            }
        }

        if (polyline.size() > 1) {
            polylines.append(QVector<QVector3D>(polyline.begin(), polyline.end()));
        }
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

SurfaceView::SurfaceViewRenderImplementation::SurfaceViewRenderImplementation()
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
        sp->setUniformValue("invDepthRange", 1.f / (maxZ_-minZ_));
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

                    ctx->glEnable(GL_DEPTH_TEST);
                    ctx->glDrawArrays(GL_TRIANGLES, 0, pts_.size());
                    ctx->glDisable(GL_DEPTH_TEST);

                    shaderProgram->disableAttributeArray(posLoc);
                    shaderProgram->release();
                }
            }
        }

        if (edgesVisible_) {
            if (!edgePts_.isEmpty()) {
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
            }
        }
    }
}
