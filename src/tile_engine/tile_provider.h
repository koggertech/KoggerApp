#pragma once

#include <QVector>

#include "map_defs.h"


namespace map {


class TileProvider
{
public:
    explicit TileProvider(int32_t providerId);
    virtual ~TileProvider();

    virtual int32_t heightToTileZ(float height)          const = 0;
    virtual int32_t lonToTileX(double lon, int z)        const = 0;
    virtual std::tuple<int32_t, int32_t, int32_t> lonToTileXWithWrapAndBoundary(const double lonStart, const double lonEnd, const int z) const = 0;
    virtual int32_t latToTileY(double lat, int z)        const = 0;
    virtual map::TileInfo indexToTileInfo(map::TileIndex tileIndx, map::TilePosition pos = map::TilePosition::kFits) const = 0;
    virtual QString createURL(const map::TileIndex& tileIndx) const = 0;

    map::TileIndex llaToTileIndex(LLA lla, int32_t z);
    int32_t getProviderId() const;

protected:
    int32_t providerId_;
};


} // namespace map

