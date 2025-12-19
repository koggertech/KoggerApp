#pragma once

#include <array>
#include <QPointF>
#include <QRectF>
#include <QVector>
#include <QSet>
#include "data_processor_defs.h"


struct ZoomInfo {
    int z = 0;
    int tileSizePx = 256;
};

class MosaicIndexProvider
{
public:
    MosaicIndexProvider(uint64_t maxTiles = 2000, bool initByDefault = true);
    void setZooms(QVector<ZoomInfo> zooms);

    QSet<TileKey> tilesInRectNed(const QRectF& rectNed, int z, int padTiles = 0) const;
    QSet<TileKey> tilesInQuadNed(const std::array<QPointF, 4>& quadNed, int z, int padTiles = 0) const;
    bool tileRectNed(const TileKey& tid, QRectF* out) const;

    int getMaxZoom() const { return maxZoom_; };
    int getMinZoom() const { return minZoom_; };

private:
    const ZoomInfo* findZoom(int z) const;
    QVector<ZoomInfo> zooms_;
    static constexpr int maxZoom_ = 1;
    static constexpr int minZoom_ = 7;
    uint64_t maxTiles_;
};

