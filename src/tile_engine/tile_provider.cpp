#include "tile_provider.h"


namespace map {


TileProvider::TileProvider(int32_t providerId) :
    providerId_(providerId)
{

}

TileProvider::~TileProvider()
{

}

map::TileIndex TileProvider::llaToTileIndex(LLA lla, int32_t z)
{
    return map::TileIndex(lonToTileX(lla.longitude, z), latToTileY(lla.latitude, z), z, providerId_);
}

int32_t TileProvider::getProviderId() const
{
    return providerId_;
}

} // namespace map
