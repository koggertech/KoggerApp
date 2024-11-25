#include "tile_google_provider.h"

#include <cmath>
#include <algorithm>
#include <tuple>


namespace map {


TileGoogleProvider::TileGoogleProvider() :
    TileProvider(GOOGLE_PROVIDER_ID)
{

}

int32_t TileGoogleProvider::heightToTileZ(const float height) const
{
    double z = std::log2(GOOGLE_TILE_CONSTANT / height);

    z = std::max(0.0, std::min(z, 21.0));
    return static_cast<int>(z);
}

int32_t TileGoogleProvider::lonToTileX(const double lon, const int z) const
{
    if (z < 0 || z > 21) {
        qWarning() << "Invalid zoom level:" << z;
        return -1;
    }

    double normalizedLon = std::fmod(lon + 180.0, 360.0);

    return static_cast<int32_t>(floor(normalizedLon / 360.0 * pow(2.0, z)));
}

std::tuple<int32_t, int32_t, int32_t> TileGoogleProvider::lonToTileXWithWrapAndBoundary(const double lonStart, const double lonEnd, const int z) const
{
    if (z < 0 || z > 21) {
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
    else {
        return {tileStart, tileEnd, -1};
    }
}

int32_t TileGoogleProvider::latToTileY(const double lat, const int z) const
{
    if (z < 0 || z > 21) {
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

TileInfo TileGoogleProvider::indexToTileInfo(TileIndex tileIndx, TilePosition pos) const
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

int TileGoogleProvider::generateNum(int x, int y) const
{
    return (x + 2 * y) % 4;
}

void TileGoogleProvider::generateWords(int x, int y, QString& sec1, QString& sec2) const
{
    int setLen = ((x * 3) + y) % 8;
    sec2 = secGoogleWord.left(setLen);
    if (y >= 10000 && y < 100000) {
        sec1 = QStringLiteral("&s=");
    }
}

QString TileGoogleProvider::createURL(const TileIndex& tileIndx) const
{
    QString str1, str2;
    generateWords(tileIndx.x_, tileIndx.y_, str1, str2);
    return QString(QStringLiteral("http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10")).arg(server).arg(generateNum(tileIndx.x_, tileIndx.y_))
        .arg(request).arg(googleSat).arg(language).arg(tileIndx.x_).arg(str1).arg(tileIndx.y_).arg(tileIndx.z_).arg(str2);
}


} // namespace map
