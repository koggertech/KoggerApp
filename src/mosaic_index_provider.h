#pragma once

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
    explicit MosaicIndexProvider(QVector<ZoomInfo> zooms = {});
    void setZooms(QVector<ZoomInfo> zooms);

    QSet<TileKey> tilesInRectNed(const QRectF& rectNed, int z, int padTiles = 0) const;
    bool tileRectNed(const TileKey& tid, QRectF* out) const;

private:
    const ZoomInfo* findZoom(int z) const;
    QVector<ZoomInfo> zooms_;
};

