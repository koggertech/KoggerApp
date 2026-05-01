#include "tile_baidu_provider.h"

#include <QtMath>
#include <cmath>
#include <algorithm>

namespace map {

namespace {

// BD-MC projection lookup tables (publicly known Baidu Mercator coefficients).
// Bands selected by absolute latitude for LL2MC, by absolute mc_y for MC2LL.
const double LLBAND[6] = {75.0, 60.0, 45.0, 30.0, 15.0, 0.0};
const double MCBAND[6] = {12890594.86, 8362377.87, 5591021.0, 3481989.83, 1678043.12, 0.0};

const double LL2MC[6][10] = {
    {-0.0015702102444, 111320.7020616939, 1704480524535203.0, -10338987376042340.0, 26112667856603880.0, -35149669176653700.0, 26595700718403920.0, -10725012454188240.0, 1800819912950474.0, 82.5},
    {0.0008277824516172526, 111320.7020463578, 6.477955746671608e8, -4.082003173641316e9, 1.077490566351142e10, -1.517187553151559e10, 1.205306533862167e10, -5.124939663577472e9, 9.133119359512032e8, 67.5},
    {0.00337398766765, 111320.7020202162, 4481351.045890365, -2.339375119931662e7, 7.968221547186455e7, -1.159649932797253e8, 9.723671115602145e7, -4.366194633752821e7, 8477230.501135234, 52.5},
    {0.00220636496208, 111320.7020209128, 51751.86112841131, 3796837.749470245, 992013.7397791013, -1221952.21711287, 1340652.697009075, -620943.6990984312, 144416.9293806241, 37.5},
    {-0.0003441963504368392, 111320.7020576856, 278.2353980772752, 2485758.690035394, 6070.750963243378, 54821.18345352118, 9540.606633304236, -2710.55326746645, 1405.483844121726, 22.5},
    {-0.0003218135878613132, 111320.7020701615, 0.00369383431289, 823725.6402795718, 0.46104986909093, 2351.343141331292, 1.58060784298199, 8.77738589078284, 0.37238884252424, 7.45}
};

const double MC2LL[6][10] = {
    {1.410526172116255e-8, 0.00000898305509648872, -1.9939833816331, 200.9824383106796, -187.2403703815547, 91.6087516669843, -23.38765649603339, 2.57121317296198, -0.03801003308653, 17337981.2},
    {-7.435856389565537e-9, 0.000008983055097726239, -0.78625201886289, 96.32687599759846, -1.85204757529826, -59.36935905485877, 47.40033549296737, -16.50741931063887, 2.28786674699375, 10260144.86},
    {-3.030883460898826e-8, 0.00000898305509983578, 0.30071316287616, 59.74293618442277, 7.357984074871, -25.38371002664745, 13.45380521110908, -3.29883767235584, 0.32710905363475, 6856817.37},
    {-1.981981304930552e-8, 0.000008983055099779535, 0.03278182852591, 40.31678527705744, 0.65659298677277, -4.44255534477492, 0.85341911805263, 0.12923347998204, -0.04625736007561, 4482777.06},
    {3.09191371068437e-9, 0.000008983055096812155, 0.00006995724062, 23.10934304144901, -0.00023663490511, -0.6321817810242, -0.00663494467273, 0.03430082397953, -0.00466043876332, 2555164.4},
    {2.890871144776878e-9, 0.000008983055095805407, -3.068298e-8, 7.47137025468032, -0.00000353937994, -0.02145144861037, -0.00001234426596, 0.00010322952773, -0.00000323890364, 826088.5}
};

// BD-09 (lon, lat) -> BD-MC (mc_x, mc_y)
void bd09ToMercator(double lon, double lat, double& mcX, double& mcY)
{
    const double absLat = std::abs(lat);
    int bandIdx = 0;
    for (int i = 0; i < 6; ++i) {
        if (absLat >= LLBAND[i]) { bandIdx = i; break; }
        bandIdx = i;
    }
    const double* cE = LL2MC[bandIdx];

    const double x = lon;
    const double y = lat;
    double xT = cE[0] + cE[1] * std::abs(x);
    double cC = std::abs(y) / cE[9];
    double yT = cE[2] + cE[3] * cC + cE[4] * cC * cC + cE[5] * cC * cC * cC
              + cE[6] * cC * cC * cC * cC + cE[7] * cC * cC * cC * cC * cC
              + cE[8] * cC * cC * cC * cC * cC * cC;
    if (x < 0.0) xT = -xT;
    if (y < 0.0) yT = -yT;

    mcX = xT;
    mcY = yT;
}

// BD-MC (mc_x, mc_y) -> BD-09 (lon, lat)
void mercatorToBd09(double mcX, double mcY, double& lon, double& lat)
{
    const double absY = std::abs(mcY);
    int bandIdx = 0;
    for (int i = 0; i < 6; ++i) {
        if (absY >= MCBAND[i]) { bandIdx = i; break; }
        bandIdx = i;
    }
    const double* cE = MC2LL[bandIdx];

    const double x = mcX;
    const double y = mcY;
    double xT = cE[0] + cE[1] * std::abs(x);
    double cC = std::abs(y) / cE[9];
    double yT = cE[2] + cE[3] * cC + cE[4] * cC * cC + cE[5] * cC * cC * cC
              + cE[6] * cC * cC * cC * cC + cE[7] * cC * cC * cC * cC * cC
              + cE[8] * cC * cC * cC * cC * cC * cC;
    if (x < 0.0) xT = -xT;
    if (y < 0.0) yT = -yT;

    lon = xT;
    lat = yT;
}

constexpr double X_PI = M_PI * 3000.0 / 180.0;
constexpr double GCJ_A = 6378245.0;
constexpr double GCJ_EE = 0.00669342162296594323;

bool outOfChina(double lat, double lon)
{
    if (lon < 72.004 || lon > 137.8347) return true;
    if (lat < 0.8293 || lat > 55.8271) return true;
    return false;
}

double transformLat(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * std::sqrt(std::abs(x));
    ret += (20.0 * std::sin(6.0 * x * M_PI) + 20.0 * std::sin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * std::sin(y * M_PI) + 40.0 * std::sin(y / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (160.0 * std::sin(y / 12.0 * M_PI) + 320.0 * std::sin(y * M_PI / 30.0)) * 2.0 / 3.0;
    return ret;
}

double transformLon(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * std::sqrt(std::abs(x));
    ret += (20.0 * std::sin(6.0 * x * M_PI) + 20.0 * std::sin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * std::sin(x * M_PI) + 40.0 * std::sin(x / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (150.0 * std::sin(x / 12.0 * M_PI) + 300.0 * std::sin(x / 30.0 * M_PI)) * 2.0 / 3.0;
    return ret;
}

// WGS84 -> GCJ-02
void wgs84ToGcj02(double wgsLat, double wgsLon, double& gcjLat, double& gcjLon)
{
    if (outOfChina(wgsLat, wgsLon)) {
        gcjLat = wgsLat;
        gcjLon = wgsLon;
        return;
    }
    double dLat = transformLat(wgsLon - 105.0, wgsLat - 35.0);
    double dLon = transformLon(wgsLon - 105.0, wgsLat - 35.0);
    double radLat = wgsLat / 180.0 * M_PI;
    double magic = std::sin(radLat);
    magic = 1.0 - GCJ_EE * magic * magic;
    double sqrtMagic = std::sqrt(magic);
    dLat = (dLat * 180.0) / ((GCJ_A * (1.0 - GCJ_EE)) / (magic * sqrtMagic) * M_PI);
    dLon = (dLon * 180.0) / (GCJ_A / sqrtMagic * std::cos(radLat) * M_PI);
    gcjLat = wgsLat + dLat;
    gcjLon = wgsLon + dLon;
}

// GCJ-02 -> WGS84 (iterative inverse, ~3 iterations is sub-meter)
void gcj02ToWgs84(double gcjLat, double gcjLon, double& wgsLat, double& wgsLon)
{
    wgsLat = gcjLat;
    wgsLon = gcjLon;
    for (int i = 0; i < 3; ++i) {
        double estLat, estLon;
        wgs84ToGcj02(wgsLat, wgsLon, estLat, estLon);
        wgsLat += (gcjLat - estLat);
        wgsLon += (gcjLon - estLon);
    }
}

// GCJ-02 -> BD-09
void gcj02ToBd09(double gcjLat, double gcjLon, double& bdLat, double& bdLon)
{
    double x = gcjLon;
    double y = gcjLat;
    double z = std::sqrt(x * x + y * y) + 0.00002 * std::sin(y * X_PI);
    double theta = std::atan2(y, x) + 0.000003 * std::cos(x * X_PI);
    bdLon = z * std::cos(theta) + 0.0065;
    bdLat = z * std::sin(theta) + 0.006;
}

// BD-09 -> GCJ-02
void bd09ToGcj02(double bdLat, double bdLon, double& gcjLat, double& gcjLon)
{
    double x = bdLon - 0.0065;
    double y = bdLat - 0.006;
    double z = std::sqrt(x * x + y * y) - 0.00002 * std::sin(y * X_PI);
    double theta = std::atan2(y, x) - 0.000003 * std::cos(x * X_PI);
    gcjLon = z * std::cos(theta);
    gcjLat = z * std::sin(theta);
}

// WGS84 -> BD-MC
void wgs84ToMercator(double wgsLat, double wgsLon, double& mcX, double& mcY)
{
    double gcjLat, gcjLon, bdLat, bdLon;
    wgs84ToGcj02(wgsLat, wgsLon, gcjLat, gcjLon);
    gcj02ToBd09(gcjLat, gcjLon, bdLat, bdLon);
    bd09ToMercator(bdLon, bdLat, mcX, mcY);
}

// BD-MC -> WGS84
void mercatorToWgs84(double mcX, double mcY, double& wgsLat, double& wgsLon)
{
    double bdLon, bdLat, gcjLat, gcjLon;
    mercatorToBd09(mcX, mcY, bdLon, bdLat);
    bd09ToGcj02(bdLat, bdLon, gcjLat, gcjLon);
    gcj02ToWgs84(gcjLat, gcjLon, wgsLat, wgsLon);
}

double tileSizeMetersForZ(int z)
{
    // 1 px = 2^(18 - z) meters in BD-MC. Tile = 256 px.
    return 256.0 * std::pow(2.0, 18 - z);
}

// Baidu's native tile Y axis points NORTH (y up). The rest of the engine
// (tile_set propagation between zoom levels, image-pixel layout) assumes the
// Google/OSM convention where Y points SOUTH (y=0 at the north edge). To stay
// compatible we expose a flipped "internal" Y outside this provider, and
// translate back to the native Baidu Y when building URLs / BD-MC bounds.
//
// internalY = (1 << z) - 1 - rawY  (and reverse). Pyramid relationships hold:
//   parent (z, ipY) has children (z+1, 2*ipY)   = north  (raw 2*rpY+1)
//                                (z+1, 2*ipY+1) = south  (raw 2*rpY)
inline int32_t flipBaiduY(int32_t y, int32_t z)
{
    return (int32_t(1) << z) - 1 - y;
}

int subdomainFor(int32_t x, int32_t rawY)
{
    return std::abs(x + rawY) % 4;
}

} // anonymous namespace


// === TileBaiduProviderBase ===

TileBaiduProviderBase::TileBaiduProviderBase(int32_t providerId) :
    TileProvider(providerId)
{
}

int32_t TileBaiduProviderBase::heightToTileZ(const float height) const
{
    double z = std::log2(BAIDU_TILE_CONSTANT / std::max<double>(height, 1.0));
    z = std::max(static_cast<double>(BAIDU_MIN_Z), std::min(z, static_cast<double>(BAIDU_MAX_Z)));
    return static_cast<int32_t>(z);
}

int32_t TileBaiduProviderBase::lonToTileX(const double lon, const int z) const
{
    if (z < BAIDU_MIN_Z || z > BAIDU_MAX_Z) {
        qWarning() << "Baidu: invalid zoom level:" << z;
        return -1;
    }
    // We don't know latitude here for the WGS->BD conversion, so use lat=0 row.
    // For the global wrap test this is acceptable; precise per-tile X uses
    // llaToTileIndex (overridden) and tile_manager prefers per-LLA min/max X.
    double mcX, mcY;
    wgs84ToMercator(0.0, lon, mcX, mcY);
    return static_cast<int32_t>(std::floor(mcX / tileSizeMetersForZ(z)));
}

std::tuple<int32_t, int32_t, int32_t> TileBaiduProviderBase::lonToTileXWithWrapAndBoundary(const double lonStart, const double lonEnd, const int z) const
{
    if (z < BAIDU_MIN_Z || z > BAIDU_MAX_Z) {
        qWarning() << "Baidu: invalid zoom level:" << z;
        return {-1, -1, -1};
    }
    int32_t tileStart = lonToTileX(lonStart, z);
    int32_t tileEnd = lonToTileX(lonEnd - 1e-9, z);
    // Baidu coverage is regional (China); antimeridian wrap is not meaningful here.
    return {tileStart, tileEnd, -1};
}

int32_t TileBaiduProviderBase::latToTileY(const double lat, const int z) const
{
    if (z < BAIDU_MIN_Z || z > BAIDU_MAX_Z) {
        qWarning() << "Baidu: invalid zoom level:" << z;
        return -1;
    }
    const double clampedLat = std::max(-74.0, std::min(74.0, lat));
    double mcX, mcY;
    wgs84ToMercator(clampedLat, 0.0, mcX, mcY);
    const int32_t rawY = static_cast<int32_t>(std::floor(mcY / tileSizeMetersForZ(z)));
    return flipBaiduY(rawY, z);
}

map::TileInfo TileBaiduProviderBase::indexToTileInfo(map::TileIndex tileIndx, map::TilePosition pos) const
{
    TileInfo info;
    GeoBounds bounds;

    const double tileSize = tileSizeMetersForZ(tileIndx.z_);
    const int32_t rawY = flipBaiduY(tileIndx.y_, tileIndx.z_);
    const double mcXMin = tileIndx.x_ * tileSize;
    const double mcXMax = (tileIndx.x_ + 1) * tileSize;
    const double mcYMin = rawY * tileSize;            // south edge in Baidu (y up)
    const double mcYMax = (rawY + 1) * tileSize;      // north edge
    const double mcXCenter = (mcXMin + mcXMax) * 0.5;
    const double mcYCenter = (mcYMin + mcYMax) * 0.5;

    // Compute the four true WGS84 corners of the BD-MC tile. Adjacent tiles
    // share the *same* BD-MC corner point at their shared edge endpoints, so
    // their WGS84 corners are guaranteed bit-identical and the rendered quads
    // tile seamlessly even though each tile is a non-rectangular quadrilateral.
    GeoCorners corners;
    mercatorToWgs84(mcXMin, mcYMin, corners.swLat, corners.swLon);
    mercatorToWgs84(mcXMin, mcYMax, corners.nwLat, corners.nwLon);
    mercatorToWgs84(mcXMax, mcYMax, corners.neLat, corners.neLon);
    mercatorToWgs84(mcXMax, mcYMin, corners.seLat, corners.seLon);

    // Bounds rectangle is kept consistent with the rest of the engine (used by
    // antimeridian-wrap detection in getTilePosition, etc.). For Baidu it's
    // the bounding box of the four real corners.
    bounds.west  = std::min({corners.swLon, corners.nwLon});
    bounds.east  = std::max({corners.neLon, corners.seLon});
    bounds.south = std::min({corners.swLat, corners.seLat});
    bounds.north = std::max({corners.nwLat, corners.neLat});

    if (pos == TilePosition::kOnLeft) {
        bounds.west += 360.0;
        bounds.east += 360.0;
        corners.swLon += 360.0; corners.nwLon += 360.0;
        corners.neLon += 360.0; corners.seLon += 360.0;
    } else if (pos == TilePosition::kOnRight) {
        bounds.west -= 360.0;
        bounds.east -= 360.0;
        corners.swLon -= 360.0; corners.nwLon -= 360.0;
        corners.neLon -= 360.0; corners.seLon -= 360.0;
    }

    double latCenter = (bounds.north + bounds.south) / 2.0;
    double resolution = (156543.03392804062 * std::cos(latCenter * M_PI / 180.0)) / std::pow(2.0, tileIndx.z_);
    info.tileSizeMeters = resolution * 256.0;
    info.bounds = bounds;
    info.corners = corners;
    return info;
}

map::TileIndex TileBaiduProviderBase::llaToTileIndex(LLA lla, int32_t z)
{
    if (z < BAIDU_MIN_Z || z > BAIDU_MAX_Z) {
        qWarning() << "Baidu: invalid zoom level:" << z;
        return map::TileIndex(-1, -1, z, providerId_);
    }
    const double clampedLat = std::max(-74.0, std::min(74.0, lla.latitude));
    double mcX, mcY;
    wgs84ToMercator(clampedLat, lla.longitude, mcX, mcY);
    const double tileSize = tileSizeMetersForZ(z);
    const int32_t x = static_cast<int32_t>(std::floor(mcX / tileSize));
    const int32_t rawY = static_cast<int32_t>(std::floor(mcY / tileSize));
    return map::TileIndex(x, flipBaiduY(rawY, z), z, providerId_);
}


// === Satellite ===

TileBaiduSatProvider::TileBaiduSatProvider() :
    TileBaiduProviderBase(kBaiduSatProviderId)
{
}

QString TileBaiduSatProvider::createURL(const map::TileIndex& tileIndx) const
{
    const int32_t rawY = flipBaiduY(tileIndx.y_, tileIndx.z_);
    return QString(QStringLiteral("https://maponline%1.bdimg.com/starpic/?qt=satepc&u=x=%2;y=%3;z=%4;v=009;type=sate&fm=46&udt=20231201"))
        .arg(subdomainFor(tileIndx.x_, rawY))
        .arg(tileIndx.x_)
        .arg(rawY)
        .arg(tileIndx.z_);
}


// === Schema (street map) ===

TileBaiduSchemaProvider::TileBaiduSchemaProvider() :
    TileBaiduProviderBase(kBaiduSchemaProviderId)
{
}

QString TileBaiduSchemaProvider::createURL(const map::TileIndex& tileIndx) const
{
    const int32_t rawY = flipBaiduY(tileIndx.y_, tileIndx.z_);
    return QString(QStringLiteral("https://maponline%1.bdimg.com/tile/?qt=vtile&x=%2&y=%3&z=%4&styles=pl&scaler=1&udt=20231201"))
        .arg(subdomainFor(tileIndx.x_, rawY))
        .arg(tileIndx.x_)
        .arg(rawY)
        .arg(tileIndx.z_);
}


// === Hybrid (satellite + roads/labels overlay) ===

TileBaiduHybridProvider::TileBaiduHybridProvider() :
    TileBaiduProviderBase(kBaiduHybridProviderId)
{
}

QString TileBaiduHybridProvider::createURL(const map::TileIndex& tileIndx) const
{
    const int32_t rawY = flipBaiduY(tileIndx.y_, tileIndx.z_);
    return QString(QStringLiteral("https://maponline%1.bdimg.com/starpic/?qt=satepc&u=x=%2;y=%3;z=%4;v=009;type=sate&fm=46&udt=20231201"))
        .arg(subdomainFor(tileIndx.x_, rawY))
        .arg(tileIndx.x_)
        .arg(rawY)
        .arg(tileIndx.z_);
}

QString TileBaiduHybridProvider::createOverlayURL(const map::TileIndex& tileIndx) const
{
    const int32_t rawY = flipBaiduY(tileIndx.y_, tileIndx.z_);
    return QString(QStringLiteral("https://maponline%1.bdimg.com/tile/?qt=satepclabel&styles=sl&x=%2&y=%3&z=%4&v=020&udt=20231201"))
        .arg(subdomainFor(tileIndx.x_, rawY))
        .arg(tileIndx.x_)
        .arg(rawY)
        .arg(tileIndx.z_);
}

} // namespace map
