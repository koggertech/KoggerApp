#include "tile_osm_provider.h"

#include <cmath>
#include <algorithm>

namespace map {

TileOsmProvider::TileOsmProvider() :
    TileProvider(OSM_PROVIDER_ID)
{
}

int32_t TileOsmProvider::heightToTileZ(const float height) const
{
    double z = std::log2(OSM_TILE_CONSTANT / height);
    z = std::max(0.0, std::min(z, static_cast<double>(OSM_MAX_Z)));
    return static_cast<int>(z);
}

int32_t TileOsmProvider::lonToTileX(const double lon, const int z) const
{
    if (z < 0 || z > OSM_MAX_Z) {
        qWarning() << "Invalid zoom level:" << z;
        return -1;
    }

    double normalizedLon = std::fmod(lon + 180.0, 360.0);
    return static_cast<int32_t>(floor(normalizedLon / 360.0 * pow(2.0, z)));
}

std::tuple<int32_t, int32_t, int32_t> TileOsmProvider::lonToTileXWithWrapAndBoundary(const double lonStart, const double lonEnd, const int z) const
{
    if (z < 0 || z > OSM_MAX_Z) {
        qWarning() << "Invalid zoom level:" << z;
        return {-1, -1, -1};
    }

    int32_t numTiles = static_cast<int32_t>(std::pow(2.0, z));

    int32_t tileStart = lonToTileX(lonStart, z);
    int32_t tileEnd = lonToTileX(lonEnd - 1e-9, z);

    bool crossesBoundary = (lonStart < -180.0 || lonEnd > 180.0 || lonStart > lonEnd);

    if (crossesBoundary) {
        int32_t boundaryTile = numTiles - 1;
        return {tileStart, tileEnd, boundaryTile};
    }

    return {tileStart, tileEnd, -1};
}

int32_t TileOsmProvider::latToTileY(const double lat, const int z) const
{
    if (z < 0 || z > OSM_MAX_Z) {
        qWarning() << "Invalid zoom level:" << z;
        return -1;
    }

    double maxLat = 85.05112878;
    double clampedLat = std::max(-maxLat, std::min(maxLat, lat));
    double sinLat = sin(clampedLat * M_PI / 180.0);
    int32_t numTiles = 1 << z;
    double y = (0.5 - std::log((1 + sinLat) / (1 - sinLat)) / (4 * M_PI)) * numTiles;
    int32_t tileY = static_cast<int32_t>(floor(y));
    tileY = std::max(0, std::min(tileY, numTiles - 1));
    return tileY;
}

map::TileInfo TileOsmProvider::indexToTileInfo(map::TileIndex tileIndx, map::TilePosition pos) const
{
    TileInfo info;
    GeoBounds bounds;

    double numTiles = pow(2.0, tileIndx.z_);

    bounds.west = (tileIndx.x_ / numTiles) * 360.0 - 180.0;
    bounds.east = ((tileIndx.x_ + 1) / numTiles) * 360.0 - 180.0;
    double lat_rad_north = atan(sinh(M_PI * (1 - 2.0 * tileIndx.y_ / numTiles)));
    bounds.north = lat_rad_north * (180.0 / M_PI);
    double lat_rad_south = atan(sinh(M_PI * (1 - 2.0 * (tileIndx.y_ + 1) / numTiles)));
    bounds.south = lat_rad_south * (180.0 / M_PI);

    if (pos == TilePosition::kOnLeft) {
        bounds.west += 360.0;
        bounds.east += 360.0;
    } else if (pos == TilePosition::kOnRight) {
        bounds.west -= 360.0;
        bounds.east -= 360.0;
    }

    double lat_center = (bounds.north + bounds.south) / 2.0;
    double resolution = (156543.03392804062 * cos(lat_center * M_PI / 180.0)) / pow(2.0, tileIndx.z_);
    double tileSizePixels = 256.0;
    info.tileSizeMeters = resolution * tileSizePixels;

    info.bounds = bounds;

    return info;
}

QString TileOsmProvider::createURL(const map::TileIndex& tileIndx) const
{
    return QString(QStringLiteral("https://tile.openstreetmap.org/%1/%2/%3.png"))
        .arg(tileIndx.z_)
        .arg(tileIndx.x_)
        .arg(tileIndx.y_);
}

} // namespace map
