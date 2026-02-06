#include "isobaths_processor.h"

#include <algorithm>
#include <QtMath>
#include "data_processor.h"
#include "math_defs.h"

IsobathsProcessor::IsobathsProcessor(DataProcessor* dataProcessorPtr):
    dataProcessor_(dataProcessorPtr),
    surfaceMeshPtr_(nullptr),
    minZ_(std::numeric_limits<float>::max()),
    maxZ_(std::numeric_limits<float>::lowest()),
    lineStepSize_(3.0f),
    labelStepSize_(100.f)
{
    qRegisterMetaType<QVector<IsobathUtils::LabelParameters>>("QVector<IsobathUtils::LabelParameters>");
}

void IsobathsProcessor::clear()
{
    lineSegments_.clear();
    labels_.clear();
    //minZ_ = std::numeric_limits<float>::max();
    //maxZ_ = std::numeric_limits<float>::lowest();
}

void IsobathsProcessor::setSurfaceMeshPtr(SurfaceMesh* surfaceMeshPtr)
{
    surfaceMeshPtr_ = surfaceMeshPtr;
}

void IsobathsProcessor::onUpdatedBottomTrackData()
{
    fullRebuildLinesLabels();
}

void IsobathsProcessor::setMinZ(float v)
{
    minZ_ = v;
}

void IsobathsProcessor::setMaxZ(float v)
{
    maxZ_ = v;
}

void IsobathsProcessor::setLineStepSize(float v)
{
    lineStepSize_ = v;
}

void IsobathsProcessor::setLabelStepSize(float v)
{
    labelStepSize_ = v;
}

float IsobathsProcessor::getLineStepSize() const
{
    return lineStepSize_;
}

float IsobathsProcessor::getLabelStepSize() const
{
    return labelStepSize_;
}

void IsobathsProcessor::edgeIntersection(const QVector3D& a,const QVector3D& b, float L, QVector<QVector3D>& out) const
{
    const float zA = a.z();
    const float zB = b.z();
    const float zShift = 0.03f;

    QVector3D A = a;
    A.setZ(zA + zShift);
    QVector3D B = b;
    B.setZ(zB + zShift);

    if (fabsf(zA - L) < kmath::fltEps && fabsf(zB - L) < kmath::fltEps) { // обе на уровне
        out << A << B;
        return;
    }

    if ((zA - L) * (zB - L) > 0.f) { // с одной стороны, нет пересечения
        return;
    }

    if (fabsf(zA - L) < kmath::fltEps) { // a на уровне
        out << A;
        return;
    }

    if (fabsf(zB - L) < kmath::fltEps) { // b на уровне
        out << B;
        return;
    }

    float t = (L - zA) / (zB - zA);
    if (std::isfinite(t)) {
        out << (A + t * (B - A));
    }
}

void IsobathsProcessor::fullRebuildLinesLabels()
{
    //QElapsedTimer t;
    //t.start();

    if (!surfaceMeshPtr_ || (minZ_ >= maxZ_ - kmath::fltEps)) {
        return;
    }

    if (!std::isfinite(minZ_) || !std::isfinite(maxZ_) || !std::isfinite(lineStepSize_) || lineStepSize_ <= kmath::fltEps) {
        return;
    }

    const float lineStep = lineStepSize_;
    const float labelStep = labelStepSize_;
    const bool labelsEnabled = std::isfinite(labelStep) && labelStep > kmath::fltEps;

    QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kSurface));
    auto postUndefined = [&]() {
        QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kUndefined));
    };

    lineSegments_.clear();
    labels_.clear();

    const int levelCnt = static_cast<int>((maxZ_ - minZ_) / lineStep) + 1;
    if (levelCnt <= 0) {
        postUndefined();
        return;
    }
    const float invLineStep = 1.0f / lineStep;

    QVector<IsobathsSegVec> segsByLvl;
    segsByLvl.resize(levelCnt);

    auto addTriSegments = [&](const QVector3D& A, const QVector3D& B, const QVector3D& C,
                              HeightType mA, HeightType mB, HeightType mC) -> bool {
        if (mA == HeightType::kUndefined ||
            mB == HeightType::kUndefined ||
            mC == HeightType::kUndefined) {
            return true;
        }

        const float triMin = std::min(A.z(), std::min(B.z(), C.z()));
        const float triMax = std::max(A.z(), std::max(B.z(), C.z()));
        if (triMax < minZ_ - kmath::fltEps || triMin > maxZ_ + kmath::fltEps) {
            return true;
        }

        int lvlStart = static_cast<int>(std::floor((triMin - minZ_) * invLineStep));
        int lvlEnd   = static_cast<int>(std::ceil((triMax - minZ_) * invLineStep));
        lvlStart = std::clamp(lvlStart, 0, levelCnt - 1);
        lvlEnd   = std::clamp(lvlEnd,   0, levelCnt - 1);

        for (int lvl = lvlStart; lvl <= lvlEnd; ++lvl) {
            if (canceled()) {
                postUndefined();
                return false;
            }

            const float L = minZ_ + lvl * lineStep;
            QVector<QVector3D> ip;
            edgeIntersection(A, B, L, ip);
            edgeIntersection(B, C, L, ip);
            edgeIntersection(C, A, L, ip);

            if (ip.size() == 2) {
                segsByLvl[lvl] << canonSeg(ip[0], ip[1]);
            }
            else if (ip.size() == 3) {
                if (!fuzzyEq(ip[0], ip[1])) {
                    segsByLvl[lvl] << canonSeg(ip[0], ip[1]);
                }
                if (!fuzzyEq(ip[1], ip[2])) {
                    segsByLvl[lvl] << canonSeg(ip[1], ip[2]);
                }
            }
        }

        return true;
    };

    bool hasMesh = false;
    const float minZBound = minZ_ - kmath::fltEps;
    const float maxZBound = maxZ_ + kmath::fltEps;

    for (auto* tile: surfaceMeshPtr_->getTilesCRef()) {
        const auto& V = tile->getHeightVerticesCRef();
        const auto& M = tile->getHeightMarkVerticesCRef();
        const int N  = qRound(std::sqrt(V.size()));

        if (N < 2) {
            continue;
        }

        hasMesh = true;

        if (canceled()) {
            postUndefined();
            return;
        }

        // Fast tile-level range check to skip tiles outside min/max.
        float tileMin = std::numeric_limits<float>::max();
        float tileMax = std::numeric_limits<float>::lowest();
        bool hasDefined = false;

        for (int i = 0; i < V.size(); ++i) {
            if (canceled()) {
                postUndefined();
                return;
            }
            if (M[i] == HeightType::kUndefined) {
                continue;
            }

            hasDefined = true;
            const float z = V[i].z();
            tileMin = std::min(tileMin, z);
            tileMax = std::max(tileMax, z);

            if (tileMin <= maxZBound && tileMax >= minZBound) {
                break;
            }
        }

        if (!hasDefined) {
            continue;
        }
        if (tileMax < minZBound || tileMin > maxZBound) {
            continue;
        }

        for (int y = 0; y < N - 1; ++y) {
            if (canceled()) {
                postUndefined();
                return;
            }
            for (int x = 0; x < N - 1; ++x) {
                const int tl = y * N + x;
                const int tr = tl + 1;
                const int bl = (y + 1) * N + x;
                const int br = bl + 1;

                if (!addTriSegments(V[tl], V[bl], V[tr], M[tl], M[bl], M[tr])) {
                    return;
                }
                if (!addTriSegments(V[tr], V[bl], V[br], M[tr], M[bl], M[br])) {
                    return;
                }
            }
        }
    }

    if (!hasMesh) {
        postUndefined();
        return;
    }

    QVector<IsobathsPolylines> polysByLvl;
    polysByLvl.resize(levelCnt);
    for (int lvl = 0; lvl < levelCnt; ++lvl) {
        if (segsByLvl[lvl].isEmpty()) {
            continue;
        }
        buildPolylines(segsByLvl[lvl], polysByLvl[lvl]);
        if (canceled()) {
            postUndefined();
            return;
        }
    }

    QVector<QVector3D> resLines;
    QVector<LabelParameters> resLabels;

    for (int lvl = 0; lvl < levelCnt; ++lvl) {
        const auto& polys = polysByLvl[lvl];
        if (polys.isEmpty()) {
            continue;
        }
        const float depth = minZ_ + lvl * lineStep;

        // линии
        for (const auto& p : polys) {
            for (int i = 0; i + 1 < p.size(); ++i) {
                resLines << p[i] << p[i + 1];
            }
        }

        // лейбы
        if (labelsEnabled) {
            float distNext = 0.0f;
            for (const auto& p : polys) {
                if (canceled()) {
                    postUndefined();
                    return;
                }

                QVector<float> segLen(p.size() - 1);
                float polyLen = 0.0f;

                for (int i = 0; i + 1 < p.size(); ++i) {
                    segLen[i] = (p[i + 1] - p[i]).length();
                    polyLen += segLen[i];
                }

                int cur = 0;
                float off = 0.0f;

                while(distNext < polyLen - kmath::fltEps) {
                    if (canceled()) {
                        postUndefined();
                        return;
                    }

                    while(cur < segLen.size() && (off + segLen[cur]) < (distNext - kmath::fltEps)) {
                        off += segLen[cur];
                        ++cur;
                    }

                    if (cur >= segLen.size()) {
                        break;
                    }

                    float t = (distNext  - off) / segLen[cur];
                    QVector3D pos = p[cur] + t * (p[cur + 1] - p[cur]);
                    QVector3D dir = (p[cur + 1] - p[cur]).normalized();
                    dir.setZ(0.0f);
                    resLabels << LabelParameters{ pos, dir, fabsf(depth) };
                    distNext += labelStep;
                }

                distNext -= polyLen;
            }
        }
    }

    filterNearbyLabels(resLabels, labels_);
    if (canceled()) {
        postUndefined();
        return;
    }
    lineSegments_ = std::move(resLines);

    QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kUndefined));

    QMetaObject::invokeMethod(dataProcessor_, "postIsobathsLineSegments", Qt::QueuedConnection, Q_ARG(QVector<QVector3D>, lineSegments_));
    QMetaObject::invokeMethod(dataProcessor_, "postIsobathsLabels", Qt::QueuedConnection, Q_ARG(QVector<IsobathUtils::LabelParameters>, labels_));

    //qDebug() << "el" << t.elapsed();
}

void IsobathsProcessor::buildPolylines(const IsobathsSegVec& segs, IsobathsPolylines& polys) const
{
    constexpr float EPS = 0.05f; // 5 см поиск соседа
    auto eq = [&](const QVector3D& a, const QVector3D& b) {
        return (a - b).lengthSquared() < (EPS * EPS);
    };
    const float invCell = 1.0f / EPS;
    auto cellKey = [&](const QVector3D& p) {
        return qMakePair(int(std::floor(p.x() * invCell)),
                         int(std::floor(p.y() * invCell)));
    };
    QHash<QPair<int,int>, QVector<int>> segsByCell;
    segsByCell.reserve(segs.size() * 2);
    for (int i = 0; i < segs.size(); ++i) {
        const auto k1 = cellKey(segs[i].first);
        const auto k2 = cellKey(segs[i].second);
        segsByCell[k1].append(i);
        if (k2 != k1) {
            segsByCell[k2].append(i);
        }
    }
    QVector<char> used(segs.size(), 0);
    auto tryExtend = [&](QList<QVector3D>& poly, bool atBack) -> bool {
        const QVector3D& p = atBack ? poly.back() : poly.front();
        const auto base = cellKey(p);
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                auto it = segsByCell.constFind(qMakePair(base.first + dx, base.second + dy));
                if (it == segsByCell.cend()) {
                    continue;
                }
                const QVector<int>& cand = it.value();
                for (int idx : cand) {
                    if (used[idx]) {
                        continue;
                    }
                    const auto& s = segs[idx];
                    if (eq(p, s.first)) {
                        used[idx] = 1;
                        if (atBack) {
                            poly << s.second;
                        }
                        else {
                            poly.prepend(s.second);
                        }
                        return true;
                    }
                    if (eq(p, s.second)) {
                        used[idx] = 1;
                        if (atBack) {
                            poly << s.first;
                        }
                        else {
                            poly.prepend(s.first);
                        }
                        return true;
                    }
                }
            }
        }
        return false;
    };
    for (int i = 0; i < segs.size(); ++i) {
        if (canceled()) {
            return;
        }
        if (used[i]) {
            continue;
        }
        QList<QVector3D> poly{ segs[i].first, segs[i].second };
        used[i] = 1;
        bool again = true;
        while (again) {
            if (canceled()) {
                return;
            }
            bool extended = false;
            extended |= tryExtend(poly, true);
            extended |= tryExtend(poly, false);
            again = extended;
        }
        if (poly.size() > 1) {
            polys << QVector<QVector3D>(poly.begin(), poly.end());
        }
    }
}
void IsobathsProcessor::filterNearbyLabels(const QVector<LabelParameters>& in, QVector<LabelParameters>& out) const
{
    const float cell = 20.f;
    const float inv = 1.f / cell;
    const float min2 = cell * cell;

    QHash<QPair<int,int>,QVector<QVector3D>> grid;
    for (const auto& lbl : in) {
        if (canceled()) {
            return;
        }

        int cx = int(std::floor(lbl.pos.x() * inv));
        int cy = int(std::floor(lbl.pos.y() * inv));
        bool isNear = false;
        for (int dx = -1; dx <= 1 && !isNear; ++dx) {
            for(int dy = -1; dy <= 1 && !isNear; ++dy) {
                auto k = qMakePair(cx + dx, cy + dy);
                if (!grid.contains(k)) {
                    continue;
                }
                const auto& pGr = grid[k];
                for (const auto& p : pGr) {
                    if ((lbl.pos - p).lengthSquared() < min2) {
                        isNear = true;
                        break;
                    }
                }
            }
        }

        if (!isNear){
            out << lbl;
            grid[qMakePair(cx, cy)] << lbl.pos;
        }
    }
}

bool IsobathsProcessor::canceled() const noexcept
{
    return dataProcessor_ && dataProcessor_->isCancelRequested();
}
