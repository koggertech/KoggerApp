#include "mosaic_index_provider.h"

#include "data_processor_defs.h"
#include "surface_tile.h"
#include <QPainterPath>
#include <QPolygonF>
#include <QtMath>
#include <algorithm>


MosaicIndexProvider::MosaicIndexProvider(uint64_t maxTiles, bool initByDefault) :
    maxTiles_(maxTiles)
{
    if (!initByDefault) {
        return;
    }

    QVector<ZoomInfo> zs;
    zs.reserve(minZoom_ - maxZoom_ + 1);

    for (int z = maxZoom_; z <= minZoom_; ++z) {
        const float pxPerMeter = ZL[z - 1].pxPerMeter;
        if (!(pxPerMeter > 0.0f && std::isfinite(pxPerMeter))) continue;

        ZoomInfo zi;
        zi.z          = z;
        zi.tileSizePx = defaultTileSidePixelSize;

        zs.push_back(zi);
    }

    setZooms(std::move(zs));
}

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

    const QRectF rect = rectNedIn.normalized();

    const double S = double(tileSideMetersFromZoom(z, zi->tileSizePx));
    if (!(S > 0.0)) {
        return {};
    }

    const double n0 = rect.left();
    const double n1 = rect.right();
    const double e0 = rect.top();
    const double e1 = rect.bottom();

    auto range1D = [S](double lo, double hi) -> QPair<int,int> {
        const int i0 = int(std::floor(lo / S));
        const int i1 = int(std::ceil (hi / S)) - 1;
        return qMakePair(std::min(i0, i1), std::max(i0, i1));
    };

    auto [ix0, ix1] = range1D(n0, n1);
    auto [iy0, iy1] = range1D(e0, e1);

    ix0 -= padTiles; ix1 += padTiles;
    iy0 -= padTiles; iy1 += padTiles;

    if (ix0 > ix1 || iy0 > iy1) {
        return {};
    }

    const int w = ix1 - ix0 + 1;
    const int h = iy1 - iy0 + 1;
    const uint64_t count = (w > 0 && h > 0) ? uint64_t(w) * uint64_t(h) : 0ull;
    if (count == 0 || count > maxTiles_) {
        return {};
    };

    QSet<TileKey> out;
    out.reserve(int(count));

    for (int ty = iy0; ty <= iy1; ++ty) {
        for (int tx = ix0; tx <= ix1; ++tx) {
            out.insert(TileKey{ tx, ty, z });
        }
    }

    return out;
}

QSet<TileKey> MosaicIndexProvider::tilesInQuadNed(const std::array<QPointF, 4>& quadNed, int z, int padTiles) const
{
    const ZoomInfo* zi = findZoom(z);
    if (!zi) {
        return {};
    }

    const double S = double(tileSideMetersFromZoom(z, zi->tileSizePx));
    if (!(S > 0.0)) {
        return {};
    }

    QPolygonF poly;
    poly.reserve(4);
    for (const auto& p : quadNed) {
        poly << p;
    }

    const QRectF bounds = poly.boundingRect().normalized();

    auto range1D = [S](double lo, double hi) -> QPair<int, int> {
        const int i0 = int(std::floor(lo / S));
        const int i1 = int(std::ceil(hi / S)) - 1;
        return qMakePair(std::min(i0, i1), std::max(i0, i1));
    };

    auto [ix0, ix1] = range1D(bounds.left(), bounds.right());
    auto [iy0, iy1] = range1D(bounds.top(), bounds.bottom());

    ix0 -= padTiles; ix1 += padTiles;
    iy0 -= padTiles; iy1 += padTiles;

    if (ix0 > ix1 || iy0 > iy1) {
        return {};
    }

    const int w = ix1 - ix0 + 1;
    const int h = iy1 - iy0 + 1;
    const uint64_t count = (w > 0 && h > 0) ? uint64_t(w) * uint64_t(h) : 0ull;
    if (count == 0 || count > maxTiles_) {
        return {};
    }

    QPainterPath polyPath;
    polyPath.addPolygon(poly);

    QSet<TileKey> out;
    out.reserve(int(count));

    for (int ty = iy0; ty <= iy1; ++ty) {
        for (int tx = ix0; tx <= ix1; ++tx) {
            const QRectF tileRect(S * tx, S * ty, S, S);
            if (!polyPath.intersects(tileRect)) {
                continue;
            }
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
