#pragma once

#include <QVector>

#include "map_defs.h"
#include "plotcash.h"


namespace map {

class TileProvider
{
public:
    TileProvider(int32_t providerId);

    virtual int32_t heightToTileZ(const float height) = 0;
    virtual int32_t lonToTileX(const double lon, const int z) = 0;
    virtual int32_t latToTileY(const double lat, const int z) = 0;

    TileIndex llaToTileIndex(LLA lla, int32_t z);

protected:
    int32_t providerId_;
};


} // namespace map

