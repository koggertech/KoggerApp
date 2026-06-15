#pragma once

#include "tile_provider.h"
#include "map_defs.h"
#include "tile_provider_ids.h"

namespace map {

constexpr double BAIDU_TILE_CONSTANT = 211859587.0;
constexpr int BAIDU_MIN_Z = 3;
constexpr int BAIDU_MAX_Z = 19;

// Shared base for all Baidu tile providers. Implements BD-MC projection,
// WGS84 ↔ GCJ-02 ↔ BD-09 conversions, Y-axis flip into Google convention,
// and request planning. Subclasses just provide the tile URL(s).
class TileBaiduProviderBase : public TileProvider
{
public:
    int32_t heightToTileZ(float height) const final;
    int32_t lonToTileX(double lon, int z) const final;
    std::tuple<int32_t, int32_t, int32_t> lonToTileXWithWrapAndBoundary(const double lonStart, const double lonEnd, const int z) const final;
    int32_t latToTileY(double lat, int z) const final;
    map::TileInfo indexToTileInfo(map::TileIndex tileIndx, map::TilePosition pos = map::TilePosition::kFits) const final;

    map::TileIndex llaToTileIndex(LLA lla, int32_t z) override;

protected:
    explicit TileBaiduProviderBase(int32_t providerId);
};

class TileBaiduSatProvider : public TileBaiduProviderBase
{
public:
    TileBaiduSatProvider();
    QString createURL(const map::TileIndex& tileIndx) const final;
};

class TileBaiduSchemaProvider : public TileBaiduProviderBase
{
public:
    TileBaiduSchemaProvider();
    QString createURL(const map::TileIndex& tileIndx) const final;
};

class TileBaiduHybridProvider : public TileBaiduProviderBase
{
public:
    TileBaiduHybridProvider();
    QString createURL(const map::TileIndex& tileIndx) const final;
    QString createOverlayURL(const map::TileIndex& tileIndx) const final;
};

} // namespace map
