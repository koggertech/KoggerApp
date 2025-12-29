#include "isobaths_processor.h"

#include <algorithm>
#include <unordered_map>
#include <QtMath>
#include "data_processor.h"
#include "math_defs.h"


static int findOrAddVertex(const QVector3D& vertice,
                           HeightType heightType,
                           std::unordered_map<VKey, int>& dict,
                           std::vector<QVector3D>& vertPool,
                           std::vector<HeightType>& vertMark)
{
    const double SCALE = 100.0;
    VKey key{ int64_t(std::llround(vertice.x() * SCALE)),
              int64_t(std::llround(vertice.y() * SCALE)) };

    if (auto it = dict.find(key); it != dict.end()) {
        return it->second;
    }

    int idx = int(vertPool.size());
    dict.emplace(key, idx);
    vertPool.push_back(vertice);
    vertMark.push_back(heightType);
    return idx;
}

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
    vertPool_.clear();
    vertMark_.clear();
    tris_.clear();
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
    if (!surfaceMeshPtr_ || (minZ_ >= maxZ_ - kmath::fltEps)) {
        return;
    }

    if (!std::isfinite(minZ_) || !std::isfinite(maxZ_) || !std::isfinite(lineStepSize_) || lineStepSize_ <= kmath::fltEps) {
        return;
    }

    const float lineStep = lineStepSize_;
    const float labelStep = labelStepSize_;
    const bool labelsEnabled = std::isfinite(labelStep) && labelStep > kmath::fltEps;

    QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kIsobaths));
    auto postUndefined = [&]() {
        QMetaObject::invokeMethod(dataProcessor_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kUndefined));
    };

    lineSegments_.clear();
    labels_.clear();

    //  уникальные вершины, список треугольников
    vertPool_.clear();
    vertMark_.clear();
    tris_.clear();
    std::unordered_map<VKey,int> vDict;
    vDict.reserve(1 << 20); // 1 Mb

    for (auto* tile: surfaceMeshPtr_->getTilesCRef()) {
        const auto& V = tile->getHeightVerticesCRef();
        const auto& M = tile->getHeightMarkVerticesRef();
        const int N  = qRound(std::sqrt(V.size()));

        if (N < 2) {
            continue;
        }

        if (canceled()) {
            postUndefined();
            return;
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

                int v0 = findOrAddVertex(V[tl], M[tl], vDict, vertPool_, vertMark_);
                int v1 = findOrAddVertex(V[bl], M[bl], vDict, vertPool_, vertMark_);
                int v2 = findOrAddVertex(V[tr], M[tr], vDict, vertPool_, vertMark_);
                int v3 = findOrAddVertex(V[br], M[br], vDict, vertPool_, vertMark_);

                tris_.push_back({v0, v1, v2});
                tris_.push_back({v2, v1, v3});
            }
        }
    }

    if (vertPool_.empty()) {
        postUndefined();
        return;
    }

    const int levelCnt = static_cast<int>((maxZ_ - minZ_) / lineStep) + 1;
    if (levelCnt <= 0) {
        postUndefined();
        return;
    }
    const float invLineStep = 1.0f / lineStep;

    QHash<int, IsobathsSegVec> segsByLvl;

    for (const TrIndxs& t : tris_) { // пересечение для треугольника
        const QVector3D A = vertPool_[t.a];
        const QVector3D B = vertPool_[t.b];
        const QVector3D C = vertPool_[t.c];
        const HeightType mA = vertMark_[t.a];
        const HeightType mB = vertMark_[t.b];
        const HeightType mC = vertMark_[t.c];

        if (canceled()) {
            postUndefined();
            return;
        }

        if (mA == HeightType::kUndefined ||
            mB == HeightType::kUndefined ||
            mC == HeightType::kUndefined) {
            continue;
        }

        const float triMin = std::min(A.z(), std::min(B.z(), C.z()));
        const float triMax = std::max(A.z(), std::max(B.z(), C.z()));
        if (triMax < minZ_ - kmath::fltEps || triMin > maxZ_ + kmath::fltEps) {
            continue;
        }

        int lvlStart = static_cast<int>(std::floor((triMin - minZ_) * invLineStep));
        int lvlEnd   = static_cast<int>(std::ceil((triMax - minZ_) * invLineStep));
        lvlStart = std::clamp(lvlStart, 0, levelCnt - 1);
        lvlEnd   = std::clamp(lvlEnd,   0, levelCnt - 1);

        for (int lvl = lvlStart; lvl <= lvlEnd; ++lvl) {
            if (canceled()) {
                postUndefined();
                return;
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
    }

    QHash<int, IsobathsPolylines> polysByLvl; // полилинии и лейбы
    for (auto it = segsByLvl.begin(); it != segsByLvl.end(); ++it) {
        buildPolylines(it.value(), polysByLvl[it.key()]);
        if (canceled()) {
            postUndefined();
            return;
        }
    }

    QVector<QVector3D> resLines;
    QVector<LabelParameters> resLabels;

    for (auto it = polysByLvl.begin(); it != polysByLvl.end(); ++it) {
        const int lvl = it.key();
        const float depth = minZ_ + lvl * lineStep;
        const auto& polys = it.value();

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
}

void IsobathsProcessor::buildPolylines(const IsobathsSegVec& segs, IsobathsPolylines& polys) const
{
    constexpr float EPS = 0.05f; // 5 см
    auto eq = [&](const QVector3D& a, const QVector3D& b) {
        return (a - b).lengthSquared() < (EPS * EPS);
    };

    QVector<char> used(segs.size(), 0);

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
            again = false;
            if (canceled()) {
                return;
            }
            for (int j = 0; j < segs.size(); ++j) {
                if (used[j]) {
                    continue;
                }

                if (eq(poly.back(), segs[j].first)) {
                    poly << segs[j].second;
                    used[j] = 1;
                    again = true;
                }
                else if (eq(poly.back(), segs[j].second)) {
                    poly << segs[j].first;
                    used[j] = 1;
                    again = true;
                }
                else if (eq(poly.front(),segs[j].second)) {
                    poly.prepend(segs[j].first);
                    used[j] = 1;
                    again = true;
                }
                else if (eq(poly.front(),segs[j].first )) {
                    poly.prepend(segs[j].second);
                    used[j] = 1;
                    again = true;
                }
            }
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
