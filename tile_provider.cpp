#include "tile_provider.h"


namespace map {


TileProvider::TileProvider(int32_t providerId) :
    providerId_(providerId)
{

}

TileIndex TileProvider::llaToTileIndex(LLA lla, int32_t z)
{
    return TileIndex(lonToTileX(lla.longitude, z), latToTileY(lla.latitude, z), z, providerId_);
}


} // namespace map
