#include "tile_manager.h"

#include <QDebug>
#include <QUrl>

#include "map_defs.h"
#include "tile_google_provider.h"


namespace map {


TileManager::TileManager(QObject *parent) :
    QObject(parent),
    tileProvider_(std::make_shared<TileGoogleProvider>()),
    tileSet_(std::make_shared<TileSet>(tileProvider_)),
    tileDownloader_(std::make_unique<TileDownloader>(tileProvider_,  10)),
    tileDB_(std::make_unique<TileDB>())
{
    QObject::connect(tileDownloader_.get(), &TileDownloader::tileDownloaded, tileSet_.get(), &TileSet::onTileDownloaded, Qt::AutoConnection);
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadFailed, tileSet_.get(), &TileSet::onTileDownloadFailed, Qt::AutoConnection);
    QObject::connect(tileDownloader_.get(), &TileDownloader::downloadStopped, tileSet_.get(), &TileSet::onTileDownloadStopped, Qt::AutoConnection);
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
    tileDownloader_->stopAndClearRequests(); // ?

    QList<TileIndex> resps;

    int minX = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int minY = std::numeric_limits<int>::max();
    int maxY = std::numeric_limits<int>::min();
    int zoomLevel = -1;
    bool zoomChanged{false}; //

    // dimensions
    for (auto& itm : request) {
        // LLARect -> tileIndx
        LLA lla(itm.x(), itm.y(), 0.0f);
        auto tileIndx = tileProvider_.get()->llaToTileIndex(lla, tileProvider_.get()->heightToTileZ(itm.z()));

        minX = std::min(minX, tileIndx.x_);
        maxX = std::max(maxX, tileIndx.x_);
        minY = std::min(minY, tileIndx.y_);
        maxY = std::max(maxY, tileIndx.y_);

        if (zoomLevel == -1) { // for first elem
            zoomLevel = tileIndx.z_;
            if (zoomLevel != lastZoomLevel_) {
                qDebug() << "zoomLevel chaged to:" << zoomLevel;
                lastZoomLevel_ = zoomLevel;
                zoomChanged = true;
            }
        }
    }

    // TileIndx responses
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            TileIndex tileIndx(x, y, zoomLevel, tileProvider_->getProviderId());
            tileSet_->addTile(tileIndx);
            resps.append(tileIndx);
        }
    }

    tileSet_->onNewRequest(resps);

    //qDebug() << "getActiveRepliesSize()" << tileDownloader_->getActiveRepliesSize();
    //qDebug() << "getDownloadQueueSize()" << tileDownloader_->getDownloadQueueSize();

    // async
    tileDownloader_->downloadTiles(resps);
}

} // namespace map
