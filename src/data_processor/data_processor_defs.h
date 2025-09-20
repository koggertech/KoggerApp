#pragma once
#include <QHash>
#include <QDebug>
#include <cmath>
#include <QVector2D>


static constexpr struct {
    int zoom;
    float pxPerMeter;
}
ZL[] = {
    {1, 100.0f},
    {2, 50.0f},
    {3, 25.0f},
    {4, 12.5f},
    {5, 6.25f},
    {6, 3.125f},
    {7, 1.5625f}
};

inline int pickZoomByDistance(float meters)
{
    if (meters < 30.f)   return 1;   // 100 px/m
    if (meters < 80.f)   return 2;   // 50
    if (meters < 200.f)  return 3;   // 25
    if (meters < 400.f)  return 4;   // 12.5
    if (meters < 800.f)  return 5;   // 6.25
    if (meters < 1600.f) return 6;   // 3.125
    return 7;                        // 1.5625
}

inline int zoomFromMpp(float mpp)
{
    // mpp = 1 / pxPerMeter - ближайший
    float bestDiff = 1e9f;
    int bestZoom = 1;
    for (auto z : ZL) {
        float mppZ = 1.0f / z.pxPerMeter;
        float d = std::fabs(mpp - mppZ);
        if (d < bestDiff) {
            bestDiff = d;
            bestZoom = z.zoom;
        }
    }

    return bestZoom;
}

inline float mppFromZoom(int zoom)
{
    for (auto z : ZL) {
        if (z.zoom == zoom) {
            return 1.0f / z.pxPerMeter;
        }
    }

    return 1.0f / 12.5f; // by def
}

inline float tileSideMetersFromZoom(int zoom, int tileSidePx = 256)
{
    return tileSidePx * mppFromZoom(zoom);
}


struct TileKey {
    int x{0};
    int y{0};
    int zoom{1}; // 0..6

    bool operator==(const TileKey& o) const noexcept {
        return x==o.x && y==o.y && zoom==o.zoom;
    }
};

inline uint qHash(const TileKey& k, uint seed=0) {
    seed ^= ::qHash(k.x) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    seed ^= ::qHash(k.y) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    seed ^= ::qHash(k.zoom) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    return seed;
}

inline QDebug operator<<(QDebug d, const TileKey& k) {
    QDebugStateSaver s(d);
    d.nospace() << "TileKey(" << k.x << "," << k.y << "," << k.zoom << ")";
    return d;
}

inline int floor_div(int a, int b) {
    int q = a / b;
    int r = a % b;
    if (r != 0 && ((r > 0) != (b > 0))) {
        --q;
    }

    return q;
}

inline void childrenOf(const TileKey& p, TileKey out[4]) {
    out[0] = { 2 * p.x    , 2 * p.y    , p.zoom + 1 };
    out[1] = { 2 * p.x + 1, 2 * p.y    , p.zoom + 1 };
    out[2] = { 2 * p.x    , 2 * p.y + 1, p.zoom + 1 };
    out[3] = { 2 * p.x + 1, 2 * p.y + 1, p.zoom + 1 };
}

inline TileKey parentOf(const TileKey& c) {
    return { floor_div(c.x,2), floor_div(c.y,2), c.zoom-1 };
}


namespace mosaic {
    // enum class Mode {
    //     kUndefined = 0,
    //     kRealtime,
    //     kPerformance
    // };
}

inline int tileIndexFromCoord(double coordMeters, double tileSideMeters) {
    return (int)std::floor(coordMeters / tileSideMeters);
}

inline TileKey tileKeyFromWorld(float worldX, float worldY, int zoom, int tileSidePx = 256)
{
    const float tileSideMeters = tileSideMetersFromZoom(zoom, tileSidePx);
    const int x = tileIndexFromCoord(worldX, tileSideMeters);
    const int y = tileIndexFromCoord(worldY, tileSideMeters);
    return { x, y, zoom };
}

inline QVector2D worldOriginFromKey(const TileKey& k, int tileSidePx = 256)
{
    const float S = tileSideMetersFromZoom(k.zoom, tileSidePx);
    return { k.x * S, k.y * S }; // общий “якорь” = (0,0)
}
