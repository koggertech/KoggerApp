#include "tile_downloader.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "tile_provider_ids.h"

namespace map {


TileDownloader::TileDownloader(std::weak_ptr<TileProvider> provider, int maxConcurrentDownloads) :
    QObject(nullptr),
    networkManager_(new QNetworkAccessManager(this)),
    tileProvider_(provider),
    activeDownloads_(0),
    maxConcurrentDownloads_(maxConcurrentDownloads),
    networkAvailable_(false)
{
    qRegisterMetaType<TileIndex>("TileIndex");
    qRegisterMetaType<TileInfo>("TileInfo");

    QObject::connect(networkManager_, &QNetworkAccessManager::finished, this, &TileDownloader::onTileDownloaded, Qt::AutoConnection);
}

TileDownloader::~TileDownloader()
{
    stopAndClearRequests();
}

bool TileDownloader::downloadTile(const TileIndex& tileIndx)
{    
    if (downloadQueue_.contains(tileIndx)) {
        return true;
    }

    for (auto it = activeReplies_.begin(); it != activeReplies_.end(); ++it) {
        QNetworkReply* reply = *it;
        TileIndex activeIndex = reply->property("tileIndex").value<TileIndex>();
        if (activeIndex == tileIndx) {
            return true;
        }
    }

    if (!networkAvailable_) {
        return false;
    }

    downloadQueue_.enqueue(tileIndx);

    while (activeDownloads_ < maxConcurrentDownloads_ && !downloadQueue_.isEmpty()) {
        startNextDownload();
    }

    return true;
}

void TileDownloader::stopAndClearRequests()
{
    QSet<TileIndex> stoppedDownloads;

    for (auto it = downloadQueue_.cbegin(); it != downloadQueue_.cend(); ++it) {
        stoppedDownloads.insert(*it);
    }
    downloadQueue_.clear();

    QList<QNetworkReply*> repliesToStop = activeReplies_.values();

    for (auto it = repliesToStop.cbegin(); it != repliesToStop.cend(); ++it) {
        auto* reply = *it;
        TileIndex index = reply->property("tileIndex").value<TileIndex>();
        stoppedDownloads.insert(index);
        reply->abort();
    }

    for (auto& itm : stoppedDownloads) {
        emit downloadStopped(itm);
    }
}

void TileDownloader::deleteRequest(const TileIndex& tileIndx)
{
    bool beenDeleted = false;

    int removedFromQueue = downloadQueue_.removeAll(tileIndx);
    if (removedFromQueue > 0) {
        beenDeleted = true;
    }

    QList<QNetworkReply*> repliesToStop = activeReplies_.values();
    for (auto it = repliesToStop.cbegin(); it != repliesToStop.cend(); ++it) {
        auto* reply = *it;
        TileIndex index = reply->property("tileIndex").value<TileIndex>();
        if (index == tileIndx) {
            beenDeleted = true;
            reply->abort();
            break; // cause origin - set
        }
    }

    if (beenDeleted) {
        emit downloadStopped(tileIndx);
    }
}

void TileDownloader::setProvider(std::weak_ptr<TileProvider> provider)
{
    tileProvider_ = provider;
}

void TileDownloader::setNetworkAvailable(bool available)
{
    if (networkAvailable_ == available) {
        return;
    }

    networkAvailable_ = available;

    if (!networkAvailable_) {
        stopAndClearRequests();
        return;
    }

    while (activeDownloads_ < maxConcurrentDownloads_ && !downloadQueue_.isEmpty()) {
        startNextDownload();
    }
}

bool TileDownloader::isNetworkAvailable() const
{
    return networkAvailable_;
}

void TileDownloader::startNextDownload()
{
    if (downloadQueue_.isEmpty() || !networkAvailable_) {
        return;
    }

    auto index = downloadQueue_.dequeue();

    QUrl url;
    if (auto sharedProvider = tileProvider_.lock(); sharedProvider) {
        url = sharedProvider->createURL(index);
    }

    if (!url.isValid()) {
        qWarning() << "Constructed invalid URL for TileIndex:" << index.x_ << index.y_ << index.z_;
        emit downloadFailed(index, "Invalid URL constructed");
        return;
    }

    QNetworkRequest request(url);
    if (auto sharedProvider = tileProvider_.lock(); sharedProvider) {
        if (sharedProvider->getProviderId() == kOsmProviderId) {
            request.setHeader(QNetworkRequest::UserAgentHeader, "KoggerApp/1.0 (contact: support@kogger.tech)");
        }
    }
    QNetworkReply* reply = networkManager_->get(request);
    reply->setProperty("tileIndex", QVariant::fromValue(index));

    activeReplies_.insert(reply);
    activeDownloads_++;
}

void TileDownloader::onTileDownloaded(QNetworkReply *reply)
{
    if (!reply || !activeReplies_.contains(reply)) {
        qWarning() << "Received a signal from an unknown or inactive reply"; // TODO
        return;
    }

    TileIndex index = reply->property("tileIndex").value<TileIndex>();

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            emit downloadStopped(index);
        }
        else {
            emit downloadFailed(index, reply->errorString());
        }
    }
    else {
        QByteArray imageData = reply->readAll();
        QImage image;

        if (image.loadFromData(imageData)) {
                emit tileDownloaded(index, image);
        }
        else {
            emit downloadFailed(index, "Failed to load image from data");
        }
    }

    activeReplies_.remove(reply);
    reply->deleteLater();
    activeDownloads_--;

    startNextDownload();
}

} // namespace map
