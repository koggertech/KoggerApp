#include "tile_manager.h"

#include <QDebug>
#include <QUrl>
#include <QThread>
#include "map_defs.h"
#include "tile_google_provider.h"
#include "tile_osm_provider.h"
#include "tile_baidu_provider.h"
#include "tile_provider_ids.h"


namespace map {

TileManager::TileManager(QObject *parent) :
    QObject(parent),
    providerId_(kGoogleProviderId),
    tileProvider_(std::make_shared<TileGoogleProvider>()),
    tileDownloader_(std::make_shared<TileDownloader>(tileProvider_, maxConcurrentDownloads_)),
    tileDB_(std::make_shared<TileDB>(providerId_)),
    tileSet_(std::make_shared<TileSet>(tileProvider_, tileDB_, tileDownloader_, maxTilesCapacity_, minTilesCapacity_)),
    lastZoomLevel_(-1),
    internetAvailable_(false),
    mapEnabled_(true)
{
    auto downloaderConnType = Qt::AutoConnection;
    // tileDownloader_ -> tileSet_
    QObject::connect(tileDownloader_.get(), &TileDownloader::tileDownloaded,  tileSet_.get(), &TileSet::onTileDownloaded,      downloaderConnType);
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadStopped, tileSet_.get(), &TileSet::onTileDownloadStopped, downloaderConnType);
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadFailed,  tileSet_.get(), &TileSet::onTileDownloadFailed,  downloaderConnType);

    QThread* dbThread = new QThread();
    tileDB_->moveToThread(dbThread);
    dbThread->setObjectName("MapDBThread");

    auto dbConnType = Qt::AutoConnection;
    // tileDB_ <-> tileSet_
    QObject::connect(tileDB_.get(),  &TileDB::tileLoaded,           tileSet_.get(), &TileSet::onTileLoaded,        dbConnType);
    QObject::connect(tileDB_.get(),  &TileDB::tileLoadFailed,       tileSet_.get(), &TileSet::onTileLoadFailed,    dbConnType);
    QObject::connect(tileDB_.get(),  &TileDB::tileLoadStopped,      tileSet_.get(), &TileSet::onTileLoadStopped,   dbConnType);
    QObject::connect(tileSet_.get(), &TileSet::dbLoadTiles,         tileDB_.get(),  &TileDB::loadTiles,            dbConnType);
    QObject::connect(tileSet_.get(), &TileSet::dbStopAndClearTasks, tileDB_.get(),  &TileDB::stopAndClearRequests, dbConnType);
    QObject::connect(tileSet_.get(), &TileSet::dbStopLoadingTile,   tileDB_.get(),  &TileDB::stopLoading,          dbConnType);
    QObject::connect(tileSet_.get(), &TileSet::dbSaveTile,          tileDB_.get(),  &TileDB::saveTile,             dbConnType);
    QObject::connect(tileDB_.get(),  &TileDB::tileSaved,            tileSet_.get(), &TileSet::onTileSaved,         dbConnType);

    QObject::connect(dbThread, &QThread::started,  tileDB_.get(), &TileDB::init,         dbConnType);
    QObject::connect(dbThread, &QThread::finished, tileDB_.get(), &QObject::deleteLater, dbConnType);
    QObject::connect(dbThread, &QThread::finished, dbThread,      &QThread::deleteLater, dbConnType);

    tileSet_->setNetworkAvailable(internetAvailable_);
    tileSet_->setMapEnabled(mapEnabled_);

    dbThread->start();
}

TileManager::~TileManager()
{

}

std::shared_ptr<TileSet> TileManager::getTileSetPtr() const
{
    return tileSet_;
}

int32_t TileManager::currentProviderId() const
{
    return providerId_;
}

QString TileManager::currentProviderName() const
{
    return providerNameForId(providerId_);
}

void TileManager::setProvider(int32_t providerId)
{
    if (providerId_ == providerId) {
        return;
    }

    if (providerId != kGoogleProviderId && providerId != kOsmProviderId &&
        providerId != kBaiduSatProviderId && providerId != kBaiduSchemaProviderId &&
        providerId != kBaiduHybridProviderId) {
        qWarning() << "TileManager::setProvider: unsupported providerId" << providerId;
        return;
    }

    providerId_ = providerId;
    lastZoomLevel_ = -1;

    if (tileDownloader_) {
        tileDownloader_->stopAndClearRequests();
    }

    if (tileSet_) {
        tileSet_->resetForProviderSwitch();
    }

    switch (providerId_) {
    case kGoogleProviderId:
        tileProvider_ = std::make_shared<TileGoogleProvider>();
        break;
    case kBaiduSatProviderId:
        tileProvider_ = std::make_shared<TileBaiduSatProvider>();
        break;
    case kBaiduSchemaProviderId:
        tileProvider_ = std::make_shared<TileBaiduSchemaProvider>();
        break;
    case kBaiduHybridProviderId:
        tileProvider_ = std::make_shared<TileBaiduHybridProvider>();
        break;
    case kOsmProviderId:
    default:
        tileProvider_ = std::make_shared<TileOsmProvider>();
        break;
    }

    if (tileDownloader_) {
        tileDownloader_->setProvider(tileProvider_);
    }

    if (tileSet_) {
        tileSet_->setResources(tileProvider_, tileDB_, tileDownloader_);
    }

    if (tileDB_) {
        QMetaObject::invokeMethod(tileDB_.get(), "setProviderId", Qt::QueuedConnection, Q_ARG(int, providerId_));
    }

    emit providerChanged(providerId_);
}

void TileManager::toggleProvider()
{
    int32_t nextProvider = kGoogleProviderId;
    switch (providerId_) {
    case kGoogleProviderId:       nextProvider = kOsmProviderId;            break;
    case kOsmProviderId:          nextProvider = kBaiduSatProviderId;       break;
    case kBaiduSatProviderId:     nextProvider = kBaiduSchemaProviderId;    break;
    case kBaiduSchemaProviderId:  nextProvider = kBaiduHybridProviderId;    break;
    case kBaiduHybridProviderId:  nextProvider = kGoogleProviderId;         break;
    }
    setProvider(nextProvider);
}

void TileManager::setInternetAvailable(bool available)
{
    if (internetAvailable_ == available) {
        return;
    }

    internetAvailable_ = available;

    if (tileSet_) {
        tileSet_->setNetworkAvailable(internetAvailable_);
    }

    emit internetAvailabilityChanged(internetAvailable_);
}

bool TileManager::isInternetAvailable() const
{
    return internetAvailable_;
}

void TileManager::setMapEnabled(bool enabled)
{
    if (mapEnabled_ == enabled) {
        return;
    }

    mapEnabled_ = enabled;

    if (tileSet_) {
        tileSet_->setMapEnabled(mapEnabled_);
    }

    emit mapEnabledChanged(mapEnabled_);
}

bool TileManager::isMapEnabled() const
{
    return mapEnabled_;
}

QString TileManager::providerNameForId(int32_t providerId)
{
    switch (providerId) {
    case kGoogleProviderId:
        return QStringLiteral("Google Satellite");
    case kOsmProviderId:
        return QStringLiteral("OpenStreetMap");
    case kBaiduSatProviderId:
        return QStringLiteral("Baidu Satellite");
    case kBaiduSchemaProviderId:
        return QStringLiteral("Baidu Schema");
    case kBaiduHybridProviderId:
        return QStringLiteral("Baidu Hybrid");
    default:
        return QStringLiteral("Unknown");
    }
}

void TileManager::getRectRequest(QVector<LLA> request, bool isPerspective, LLARef viewLlaRef, bool moveUp, map::CameraTilt tiltCam)
{
    if (!mapEnabled_) {
        return;
    }

    Q_UNUSED(tiltCam);

    int minX = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int minY = std::numeric_limits<int>::max();
    int maxY = std::numeric_limits<int>::min();
    int zoomLevel = -1;

    double minLat = std::numeric_limits<double>::max();
    double maxLat = std::numeric_limits<double>::lowest();
    double minLon = std::numeric_limits<double>::max();
    double maxLon = std::numeric_limits<double>::lowest();

    ZoomState zoomState = ZoomState::kUndefined;

    // dimensions
    for (auto& itm : request) {
        // LLARect -> tileIndx
        LLA lla(itm.latitude, itm.longitude, 0.0f);
        auto tileIndx = tileProvider_.get()->llaToTileIndex(lla, tileProvider_.get()->heightToTileZ(itm.altitude));

        minX = std::min(minX, tileIndx.x_);
        maxX = std::max(maxX, tileIndx.x_);
        minY = std::min(minY, tileIndx.y_);
        maxY = std::max(maxY, tileIndx.y_);

        if (itm.latitude > maxLat) maxLat = itm.latitude;
        if (itm.latitude < minLat) minLat = itm.latitude;
        if (itm.longitude > maxLon) maxLon = itm.longitude;
        if (itm.longitude < minLon) minLon = itm.longitude;

        if (zoomLevel == -1) { // for the first element
            zoomLevel = tileIndx.z_;
            if (zoomLevel != lastZoomLevel_) {
                zoomState = lastZoomLevel_ > zoomLevel ? ZoomState::kOut : ZoomState::kIn;
                //qDebug() << "zoom level chaged to:" << zoomLevel << "isPerspective" << isPerspective << "zoomState" << static_cast<int>(zoomState);
                lastZoomLevel_ = zoomLevel;
            }
            else {
                zoomState = ZoomState::kUnchanged;
            }
        }
    }

    double lonEdge = 180.0;
    if (maxLon > lonEdge)
        maxLon = lonEdge;
    if (maxLon < -lonEdge)
        maxLon = -lonEdge;
    if (minLon > lonEdge)
        minLon = lonEdge;
    if (minLon < -lonEdge)
        minLon = -lonEdge;
    if (qFuzzyCompare(minLon, maxLon)) {
        return;
    }

    auto [lonStartTile, lonEndTile, boundaryTile] = tileProvider_.get()->lonToTileXWithWrapAndBoundary(minLon, maxLon, zoomLevel);

    uint64_t reqSize = 0;
    QSet<TileIndex> indxRequest;

    if (boundaryTile == -1) {
        // Use per-LLA min/max X computed via llaToTileIndex (which knows the real
        // latitude) instead of lonStartTile/lonEndTile. For Google/OSM these are
        // identical (X doesn't depend on lat); for projections with lat-dependent
        // X (e.g. Baidu BD-MC + GCJ-02) it's the only correct range.
        const qint64 xTileCountSigned =
            static_cast<qint64>(maxX) - static_cast<qint64>(minX) + 1;
        const qint64 yTileCountSigned =
            static_cast<qint64>(maxY) - static_cast<qint64>(minY) + 1;
        if (xTileCountSigned <= 0 || yTileCountSigned <= 0) {
            return;
        }
        const uint64_t xTileCount = static_cast<uint64_t>(xTileCountSigned);
        const uint64_t yTileCount = static_cast<uint64_t>(yTileCountSigned);
        reqSize = xTileCount * yTileCount;
        if (reqSize < minTilesCapacity_) {
            for (int x = minX; x <= maxX; ++x) {
                for (int y = minY; y <= maxY; ++y) {
                    TileIndex tileIndx(x, y, zoomLevel, tileProvider_->getProviderId());
                    indxRequest.insert(tileIndx);
                }
            }
        }
    }

    if (!indxRequest.isEmpty()) {
        tileSet_->onNewRequest(indxRequest, zoomState, viewLlaRef, isPerspective, minLon, maxLon, moveUp);
    }
}

void TileManager::getLlaRef(LLARef viewLlaRef)
{
    if (!mapEnabled_) {
        return;
    }

    tileSet_->onNewLlaRef(viewLlaRef);
}

} // namespace map
