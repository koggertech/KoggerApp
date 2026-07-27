#include "tile_manager.h"

#include <algorithm>

#include <QDebug>
#include <QUrl>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTimer>
#include <QRandomGenerator>
#include "map_defs.h"
#include "tile_google_provider.h"
#include "tile_osm_provider.h"
#include "tile_baidu_provider.h"
#include "tile_provider_ids.h"


namespace {
const QString kTileManifestUrl = QStringLiteral("https://raw.githubusercontent.com/KoggerTech/koggerapp/master/resources/tile_manifest.json");
}


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
    mapEnabled_(true),
    versionNam_(new QNetworkAccessManager(this)),
    versionFailureStreak_(0),
    versionResolveInFlight_(false),
    lastVersionResolveMs_(0)
{
    auto downloaderConnType = Qt::AutoConnection;
    // tileDownloader_ -> tileSet_
    QObject::connect(tileDownloader_.get(), &TileDownloader::tileDownloaded,  tileSet_.get(), &TileSet::onTileDownloaded,      downloaderConnType);
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadStopped, tileSet_.get(), &TileSet::onTileDownloadStopped, downloaderConnType);
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadFailed,  tileSet_.get(), &TileSet::onTileDownloadFailed,  downloaderConnType);

    // tileDownloader_ -> imagery version resolver
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadFailed, this, &TileManager::onDownloadFailedForVersion, downloaderConnType);
    QObject::connect(tileDownloader_.get(), &TileDownloader::tileDownloaded, this, &TileManager::onDownloadedForVersion,     downloaderConnType);

    seedProviderVersion();

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

    versionFailureStreak_ = 0;
    versionResolveInFlight_ = false;
    seedProviderVersion();

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
    case kBaiduHybridProviderId:
    default:                      nextProvider = kGoogleProviderId;         break;
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

QString TileManager::versionSettingsKey(const QString& manifestKey)
{
    return QStringLiteral("tiles/version/") + manifestKey;
}

void TileManager::seedProviderVersion()
{
    if (!tileProvider_ || tileProvider_->manifestKey().isEmpty()) {
        return;
    }

    const int floor = tileProvider_->imageryVersion();
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    const int persisted = settings.value(versionSettingsKey(tileProvider_->manifestKey()), floor).toInt();
    tileProvider_->setImageryVersion(std::max(floor, persisted));
}

void TileManager::onDownloadedForVersion(const map::TileIndex& tileIndx, const QImage& image)
{
    Q_UNUSED(tileIndx);
    Q_UNUSED(image);
    versionFailureStreak_ = 0;
}

void TileManager::onDownloadFailedForVersion(const map::TileIndex& tileIndx, const QString& errorString, int httpStatus)
{
    Q_UNUSED(tileIndx);
    Q_UNUSED(errorString);

    if (!tileProvider_ || tileProvider_->manifestKey().isEmpty()) {
        return;
    }
    if (httpStatus != 403 && httpStatus != 404) {
        return;
    }
    if (++versionFailureStreak_ < versionFailureThreshold_) {
        return;
    }
    maybeTriggerVersionResolve();
}

void TileManager::maybeTriggerVersionResolve()
{
    if (versionResolveInFlight_ || !internetAvailable_) {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastVersionResolveMs_ != 0 && now - lastVersionResolveMs_ < versionResolveCooldownMs_) {
        return;
    }

    versionResolveInFlight_ = true;
    lastVersionResolveMs_ = now;
    versionFailureStreak_ = 0;

    const int jitter = QRandomGenerator::global()->bounded(versionJitterMaxMs_);
    QTimer::singleShot(jitter, this, [this] { fetchManifest(); });
}

void TileManager::fetchManifest()
{
    if (!tileProvider_ || tileProvider_->manifestKey().isEmpty()) {
        versionResolveInFlight_ = false;
        return;
    }

    const QString key = tileProvider_->manifestKey();
    const int current = tileProvider_->imageryVersion();

    QNetworkRequest req{ QUrl(kTileManifestUrl) };
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(versionRequestTimeoutMs_);
    QNetworkReply* reply = versionNam_->get(req);

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, key, current] {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QJsonParseError perr;
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &perr);
            if (perr.error == QJsonParseError::NoError && doc.isObject()) {
                const int v = doc.object().value(QStringLiteral("providers")).toObject()
                                  .value(key).toObject().value(QStringLiteral("version")).toInt(-1);
                if (v > current) {
                    probeVersion(v, false);
                    return;
                }
            }
        }

        probeVersion(current, true);
    });
}

void TileManager::probeVersion(int candidate, bool scanUp)
{
    if (!tileProvider_ || tileProvider_->manifestKey().isEmpty()) {
        versionResolveInFlight_ = false;
        return;
    }

    const int current = tileProvider_->imageryVersion();
    if (scanUp && candidate > current + versionProbeSpan_) {
        versionResolveInFlight_ = false;
        return;
    }

    const QString urlStr = tileProvider_->versionedCanaryUrl(candidate);
    if (urlStr.isEmpty()) {
        versionResolveInFlight_ = false;
        return;
    }

    QNetworkRequest req{ QUrl(urlStr) };
    req.setTransferTimeout(versionRequestTimeoutMs_);
    QNetworkReply* reply = versionNam_->get(req);
    reply->setProperty("candidate", candidate);
    reply->setProperty("current", current);
    reply->setProperty("scanUp", scanUp);

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();

        const int candidate = reply->property("candidate").toInt();
        const int current = reply->property("current").toInt();
        const bool scanUp = reply->property("scanUp").toBool();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && httpStatus == 200) {
            QImage img;
            if (img.loadFromData(reply->readAll()) && !img.isNull()) {
                if (candidate > current) {
                    applyResolvedVersion(candidate);
                } else {
                    versionResolveInFlight_ = false;
                }
                return;
            }
        }

        probeVersion(scanUp ? candidate + 1 : current, true);
    });
}

void TileManager::applyResolvedVersion(int version)
{
    versionResolveInFlight_ = false;

    if (!tileProvider_ || tileProvider_->manifestKey().isEmpty()) {
        return;
    }
    if (version <= tileProvider_->imageryVersion()) {
        return;
    }

    tileProvider_->setImageryVersion(version);

    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.setValue(versionSettingsKey(tileProvider_->manifestKey()), version);

    if (tileDownloader_) {
        tileDownloader_->stopAndClearRequests();
    }
    if (tileSet_) {
        tileSet_->resetForProviderSwitch();
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
        const qint64 xTileCountSigned =
            static_cast<qint64>(lonEndTile) - static_cast<qint64>(lonStartTile) + 1;
        const qint64 yTileCountSigned =
            static_cast<qint64>(maxY) - static_cast<qint64>(minY) + 1;
        if (xTileCountSigned <= 0 || yTileCountSigned <= 0) {
            return;
        }
        const uint64_t xTileCount = static_cast<uint64_t>(xTileCountSigned);
        const uint64_t yTileCount = static_cast<uint64_t>(yTileCountSigned);
        reqSize = xTileCount * yTileCount;
        if (reqSize < minTilesCapacity_) {
            for (int x = lonStartTile; x <= lonEndTile; ++x) {
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
