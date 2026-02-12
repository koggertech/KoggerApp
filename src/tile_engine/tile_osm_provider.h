#pragma once

#include "tile_provider.h"
#include "map_defs.h"
#include "tile_provider_ids.h"

namespace map {

constexpr double OSM_TILE_CONSTANT = 126543000.03392;
constexpr int OSM_PROVIDER_ID = kOsmProviderId;
constexpr int OSM_MAX_Z = 19;

class TileOsmProvider : public TileProvider
{
public:
    TileOsmProvider();

    int32_t heightToTileZ(float height) const override final;
    int32_t lonToTileX(double lon, int z) const override final;
    std::tuple<int32_t, int32_t, int32_t> lonToTileXWithWrapAndBoundary(const double lonStart, const double lonEnd, const int z) const override final;
    int32_t latToTileY(double lat, int z) const override final;
    map::TileInfo indexToTileInfo(map::TileIndex tileIndx, map::TilePosition pos = map::TilePosition::kFits) const override final;
    QString createURL(const map::TileIndex& tileIndx) const override final;
};

} // namespace map
