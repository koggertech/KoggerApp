#pragma once

#include <stdint.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <QHash>
#include <QPair>
#include <QSet>
#include <QVector>
#include <QVector3D>
#include "math_defs.h"


namespace IsobathUtils {

using IsobathsSeg = QPair<QVector3D, QVector3D>;
using IsobathsSegVec = QVector<IsobathsSeg>;
using IsobathsPolyline = QVector<QVector3D>;
using IsobathsPolylines = QVector<IsobathsPolyline>;


// functions
template <typename T>
inline void appendUnique(QVector<T>& dst, const QVector<T>& src)
{
    for (const T& v : src) {
        if (!dst.contains(v)) {
            dst.append(v);
        }
    }
}

inline QPair<QVector3D, QVector3D> canonSeg(const QVector3D& p1, const QVector3D& p2)
{
    if (p1.x() < p2.x()) return { p1, p2 };
    if (p1.x() > p2.x()) return { p2, p1 };
    if (p1.y() < p2.y()) return { p1, p2 };
    if (p1.y() > p2.y()) return { p2, p1 };
    if (p1.z() < p2.z()) return { p1, p2 };
    if (p1.z() > p2.z()) return { p2, p1 };
    return { p1, p2 };
}

inline const QVector<QVector3D>& colorPalette(int themeId)
{
    static const QVector<QVector<QVector3D>> palettes = {
        // 0: midnight
        {
            QVector3D(0.2f,    0.5f,    1.0f),
            QVector3D(0.2f,    0.4f,    0.9f),
            QVector3D(0.3f,    0.3f,    0.8f),
            QVector3D(0.4f,    0.2f,    0.7f),
            QVector3D(0.5f,    0.2f,    0.6f),
            QVector3D(0.6f,    0.3f,    0.5f),
            QVector3D(0.7f,    0.4f,    0.4f),
            QVector3D(0.8f,    0.5f,    0.3f),
            QVector3D(0.9f,    0.6f,    0.2f),
            QVector3D(1.0f,    0.7f,    0.1f)
        },
        // 1: default
        {

            QVector3D(60/255.0f,30/255.0f,130/255.0f),
            QVector3D(5/255.0f,55/255.0f,195/255.0f),
            QVector3D(15/255.0f,107/255.0f,176/255.0f),
            QVector3D(40/255.0f,170/255.0f,195/255.0f),
            QVector3D(65/255.0f,180/255.0f,150/255.0f),
            QVector3D(78/255.0f,210/255.0f,110/255.0f),
            QVector3D(41/255.0f,164/255.0f,53/255.0f),
            QVector3D(131/255.0f,164/255.0f,21/255.0f),
            QVector3D(174/255.0f,164/255.0f,89/255.0f),
            QVector3D(200/255.0f,190/255.0f,135/255.0f),
            QVector3D(230/255.0f,220/255.0f,180/255.0f),
        },
        // 2: blue
        {
            // QVector3D(0.1f,    0.0f,    0.1f),
            QVector3D(0.0f,  0.1f,  0.314f),
            QVector3D(0.1f,  0.206f,  0.402f),
            QVector3D(0.2f,  0.506f,  0.702f),
            QVector3D(0.396f,  0.706f,  0.902f),
            QVector3D(0.745f,  0.941f,  0.980f),
            QVector3D(1.0f,    1.0f,    1.0f)
        },
        // 3: sepia
        {
            QVector3D(0.314f,  0.1f,  0.0f),
            QVector3D(0.402f,  0.206f,  0.1f),
            QVector3D(0.702f,  0.506f,  0.2f),
            QVector3D(0.902f,  0.706f,  0.396f),
            QVector3D(0.980f,  0.941f,  0.745f),
            QVector3D(1.0f,    1.0f,    1.0f)
        },
        // 4: colored
        {
            QVector3D(0.4f,  0.1f,  0.5f),
            QVector3D(0.0f,  0.4f,  0.7f),
            QVector3D(0.1f,  0.7f,  0.9f),
            QVector3D(0.1f,  0.9f,  0.1f),
            QVector3D(0.6f,  1.0f,  0.0f),
            QVector3D(0.8f,  1.0f,  0.0f),
            QVector3D(1.0f,  0.9f,  0.0f),
            QVector3D(1.0f,  0.5f,  0.0f),
            QVector3D(1.0f,  0.0f,  0.2f),
            QVector3D(1.0f,  0.8f,  0.8f)
        },
        // 5: bw
        {
            QVector3D(0.1f,    0.1f,    0.1f),
            QVector3D(0.4f,  0.4f,  0.4f),
            QVector3D(0.745f,  0.784f,  0.784f),
            QVector3D(0.95f,  1.0f,    1.0f)
        },
        // 6: standard
        {
            QVector3D(0.0f,    0.0f,    0.3f),
            QVector3D(0.0f,    0.0f,    0.6f),
            QVector3D(0.0f,    0.5f,    1.0f),
            QVector3D(0.0f,    1.0f,    0.5f),
            QVector3D(1.0f,    1.0f,    0.0f),
            QVector3D(1.0f,    0.6f,    0.0f),
            QVector3D(0.8f,    0.2f,    0.0f)
        }
    };

    return palettes[std::clamp(themeId, 0, static_cast<int>(palettes.size() - 1))];
}

inline bool fuzzyEq(const QVector3D& a, const QVector3D& b, float eps = kmath::fltEps)
{
    return (a - b).lengthSquared() < eps * eps;
}

// structs
struct VKey {
    int64_t x = -1;
    int64_t y = -1;
    bool operator==(const VKey& o) const { return x==o.x && y==o.y; }
};

struct IsoState {
    QHash<int, IsobathsSegVec> hashSegsByLvl;
    QHash<int, IsobathsPolylines> polylinesByLevel;
    QHash<int, QHash<int, IsobathsSegVec>> triangleSegs; // triIdx -> (level -> segs)
    QSet<int> dirtyLevels;

    void clear() {
        hashSegsByLvl.clear();
        polylinesByLevel.clear();
        triangleSegs.clear();
        dirtyLevels.clear();
    }

    bool isEmpty() const {
        return hashSegsByLvl.isEmpty();
    }
};

struct LabelParameters
{
    QVector3D pos;
    QVector3D dir;
    float depth;
};

struct ColorInterval
{
    float depth = 0.0f;
    QVector3D color;
    ColorInterval() = default;
    ColorInterval(float d, const QVector3D &c) : depth(d), color(c) {}
};

struct PendingWork {
    QVector<int> indxs; // новые индексы точек
    bool         rebuildLineLabels = false; // полный линий и лейб
    bool         rebuildAll        = false; // вершины треугольников

    void clear() {
        indxs.clear();
        rebuildLineLabels = false;
        rebuildAll = false;
    }
};

struct TrIndxs {
    int a = -1;
    int b = -1;
    int c = -1;
};

} // namespace IsobathUtils


namespace std {

template<>
struct hash<IsobathUtils::VKey>{
    size_t operator()(const IsobathUtils::VKey& k) const noexcept {
        return (uint64_t(k.x) << 32) ^ uint64_t(k.y);
    }
};

} // namespace std
