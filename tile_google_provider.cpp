#include "tile_google_provider.h"

#include <cmath>
#include <algorithm>


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
    if (lon < -180.0 || lon > 180.0) {
        qWarning() << "Invalid longitude:" << lon;
        return -1;
    }

    return static_cast<int32_t>(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

int32_t TileGoogleProvider::latToTileY(const double lat, const int z) const
{
    if (z < 0 || z > 21) {
        qWarning() << "Invalid zoom level:" << z;
        return -1;
    }
    if (lat < -90.0 || lat > 90.0) {
        qWarning() << "Invalid latitude:" << lat;
        return -1;
    }

    return static_cast<int32_t>(floor((1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, z)));
}

TileInfo TileGoogleProvider::indexToTileInfo(TileIndex tileIndx) const
{
    TileInfo info;
    GeoBounds bounds;

    double n = pow(2.0, tileIndx.z_);

    bounds.west = (tileIndx.x_ / n) * 360.0 - 180.0;
    bounds.east = ((tileIndx.x_ + 1) / n) * 360.0 - 180.0;
    double lat_rad_north = atan(sinh(M_PI * (1 - 2.0 * tileIndx.y_ / n)));
    bounds.north = lat_rad_north * (180.0 / M_PI);
    double lat_rad_south = atan(sinh(M_PI * (1 - 2.0 * (tileIndx.y_ + 1) / n)));
    bounds.south = lat_rad_south * (180.0 / M_PI);

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
