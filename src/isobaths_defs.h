#pragma once

#include <QHash>
#include <QPair>
#include <QSet>
#include <QVector>
#include <QVector3D>


namespace IsobathUtils {

using IsobathsSeg = QPair<QVector3D, QVector3D>;
using IsobathsSegVec = QVector<IsobathsSeg>;
using IsobathsPolyline = QVector<QVector3D>;
using IsobathsPolylines = QVector<IsobathsPolyline>;

// constants
static constexpr float epsilon_ = 1e-6f;

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
            QVector3D(0.0f,    0.0f,    0.3f),
            QVector3D(0.0f,    0.0f,    0.6f),
            QVector3D(0.0f,    0.5f,    1.0f),
            QVector3D(0.0f,    1.0f,    0.5f),
            QVector3D(1.0f,    1.0f,    0.0f),
            QVector3D(1.0f,    0.6f,    0.0f),
            QVector3D(0.8f,    0.2f,    0.0f)
        },
        // 2: blue
        {
            QVector3D(0.0f,    0.0f,    0.0f),
            QVector3D(0.078f,  0.020f,  0.314f),
            QVector3D(0.196f,  0.706f,  0.902f),
            QVector3D(0.745f,  0.941f,  0.980f),
            QVector3D(1.0f,    1.0f,    1.0f)
        },
        // 3: sepia
        {
            QVector3D(0.0f,    0.0f,    0.0f),
            QVector3D(0.1961f, 0.196f,  0.039f),
            QVector3D(0.9020f, 0.784f,  0.392f),
            QVector3D(1.0f,    1.0f,    0.862f)
        },
        // 4: colored
        {
            QVector3D(0.0f,    0.0f,    0.0f),
            QVector3D(0.157f,  0.0f,    0.313f),
            QVector3D(0.0f,    0.117f,  0.588f),
            QVector3D(0.078f,  0.902f,  0.117f),
            QVector3D(1.0f,    0.196f,  0.078f),
            QVector3D(1.0f,    1.0f,    1.0f)
        },
        // 5: bw
        {
            QVector3D(0.0f,    0.0f,    0.0f),
            QVector3D(0.745f,  0.784f,  0.784f),
            QVector3D(0.902f,  1.0f,    1.0f)
        },
        // 6: wb
        {
            QVector3D(0.902f,  1.0f,    1.0f),
            QVector3D(0.274f,  0.274f,  0.274f),
            QVector3D(0.0f,    0.0f,    0.0f)
        }
    };

    return palettes[std::clamp(themeId, 0, palettes.size() - 1)];
}

inline bool fuzzyEq(const QVector3D& a, const QVector3D& b, float eps = epsilon_)
{
    return (a - b).lengthSquared() < eps * eps;
}

// structs
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

struct LLabelInfo
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

    void clear() {
        indxs.clear();
        rebuildLineLabels = false;
    }
};

} // namespace IsobathUtils
