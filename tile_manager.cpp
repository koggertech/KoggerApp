#include "tile_manager.h"

#include <QDebug>
#include <QUrl>
#include <QThread>

#include "map_defs.h"
#include "tile_google_provider.h"


namespace map {


TileManager::TileManager(QObject *parent) :
    QObject(parent),
    tileProvider_(std::make_shared<TileGoogleProvider>()),
    tileDownloader_(std::make_shared<TileDownloader>(tileProvider_,  10)),
    tileDB_(std::make_shared<TileDB>(tileProvider_)),
    tileSet_(std::make_shared<TileSet>(tileProvider_, tileDB_, tileDownloader_, 500))
{
    // tileDownloader_ -> tileSet_
    QObject::connect(tileDownloader_.get(), &TileDownloader::tileDownloaded,  tileSet_.get(), &TileSet::onTileDownloaded,      Qt::AutoConnection);
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadStopped, tileSet_.get(), &TileSet::onTileDownloadStopped, Qt::AutoConnection);
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadFailed,  tileSet_.get(), &TileSet::onTileDownloadFailed,  Qt::AutoConnection);

    QThread* dbThread = new QThread();
    tileDB_->moveToThread(dbThread);

    // tileDB_ <-> tileSet_
    QObject::connect(tileDB_.get(), &TileDB::tileLoaded,     tileSet_.get(), &TileSet::onTileLoaded, Qt::AutoConnection);
    QObject::connect(tileDB_.get(), &TileDB::tileLoadFailed, tileSet_.get(), &TileSet::onTileLoadFailed, Qt::AutoConnection);
    QObject::connect(tileSet_.get(), &TileSet::requestLoadTiles,    tileDB_.get(), &TileDB::loadTiles, Qt::AutoConnection);
    QObject::connect(tileSet_.get(), &TileSet::requestStopAndClear, tileDB_.get(), &TileDB::stopAndClearRequests, Qt::AutoConnection);
    QObject::connect(tileSet_.get(), &TileSet::requestSaveTile,     tileDB_.get(), &TileDB::saveTile, Qt::AutoConnection);

    QObject::connect(dbThread, &QThread::started, tileDB_.get(), &TileDB::init, Qt::AutoConnection);
    QObject::connect(dbThread, &QThread::finished, tileDB_.get(), &QObject::deleteLater, Qt::AutoConnection);
    QObject::connect(dbThread, &QThread::finished, dbThread, &QThread::deleteLater, Qt::AutoConnection);

    dbThread->start();
}

TileManager::~TileManager()
{

}

std::shared_ptr<TileSet> TileManager::getTileSetPtr() const
{
    return tileSet_;
}

void TileManager::getRectRequest(QVector<QVector3D> request)
{
    QList<TileIndex> indxRequest;

    int minX = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int minY = std::numeric_limits<int>::max();
    int maxY = std::numeric_limits<int>::min();
    int zoomLevel = -1;

    // dimensions
    for (auto& itm : request) {
        // LLARect -> tileIndx
        LLA lla(itm.x(), itm.y(), 0.0f);
        auto tileIndx = tileProvider_.get()->llaToTileIndex(lla, tileProvider_.get()->heightToTileZ(itm.z()));

        minX = std::min(minX, tileIndx.x_);
        maxX = std::max(maxX, tileIndx.x_);
        minY = std::min(minY, tileIndx.y_);
        maxY = std::max(maxY, tileIndx.y_);

        if (zoomLevel == -1) { // for the first element
            zoomLevel = tileIndx.z_;
            if (zoomLevel != lastZoomLevel_) {
                qDebug() << "zoom level chaged to:" << zoomLevel;
                lastZoomLevel_ = zoomLevel;
            }
        }
    }

    // creating TileIndx requests
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            TileIndex tileIndx(x, y, zoomLevel, tileProvider_->getProviderId());
            indxRequest.append(tileIndx);
        }
    }

    tileSet_->onNewRequest(indxRequest);
}


} // namespace map
