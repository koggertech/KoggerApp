#include "mosaic_index_provider.h"
#include <QtMath>
#include <algorithm>


MosaicIndexProvider::MosaicIndexProvider(QVector<ZoomInfo> zooms)
    : zooms_(std::move(zooms))
{}

void MosaicIndexProvider::setZooms(QVector<ZoomInfo> zooms)
{
    zooms_ = std::move(zooms);
}

const ZoomInfo* MosaicIndexProvider::findZoom(int z) const
{
    for (const auto& zi : zooms_) {
        if (zi.z == z) {
            return &zi;
        }
    }

    return nullptr;
}

QSet<TileKey> MosaicIndexProvider::tilesInRectNed(const QRectF& rectNedIn, int z, int padTiles) const
{
    const ZoomInfo* zi = findZoom(z);
    if (!zi) {
        return {};
    }

    QRectF rect = rectNedIn.normalized();

    const double S = double(tileSideMetersFromZoom(z, zi->tileSizePx));

    qDebug() << "m" << S;

    if (!(S > 0.0)) {
        return {};
    }

    const double n0 = rect.left();
    const double n1 = rect.right();
    const double e0 = rect.top();
    const double e1 = rect.bottom();

    auto idxRange = [S](double a, double b) {
        const double lo = std::min(a, b);
        const double hi = std::max(a, b);
        const int i0 = int(std::floor(lo / S));
        const int i1 = int(std::ceil (hi / S)) - 1;
        return qMakePair(i0, i1);
    };

    auto [ix0, ix1] = idxRange(n0, n1);
    auto [iy0, iy1] = idxRange(e0, e1);

    const uint64_t maxTiles = 2000; // TODO
    uint64_t size = (iy1 - iy0) * (ix1 - ix0);
    if (size > maxTiles) {
        return {};
    }

    ix0 -= padTiles;  iy0 -= (1 + padTiles);
    ix1 += padTiles;  iy1 += padTiles;

    if (ix0 > ix1 || iy0 > iy1) {
        return {};
    }

    QSet<TileKey> out;
    out.reserve(std::max(0, (ix1 - ix0 + 1)) * std::max(0, (iy1 - iy0 + 1)));

    for (int ty = iy0; ty <= iy1; ++ty) {
        for (int tx = ix0; tx <= ix1; ++tx) {
            out.insert(TileKey{ tx, ty, z });
        }
    }

    return out;
}

bool MosaicIndexProvider::tileRectNed(const TileKey& tid, QRectF* out) const
{
    const ZoomInfo* zi = findZoom(tid.zoom);
    if (!zi || !out) {
        return false;
    }

    const double S = double(tileSideMetersFromZoom(tid.zoom, zi->tileSizePx));

    const QVector2D o = worldOriginFromKey(tid, zi->tileSizePx);
    const double left   = o.x();
    const double top    = o.y();
    const double right  = left  + S;
    const double bottom = top   + S;

    *out = QRectF(QPointF(left, top), QPointF(right, bottom)).normalized();

    return true;
}
