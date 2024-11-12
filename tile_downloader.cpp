#include "tile_downloader.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>


namespace map {


TileDownloader::TileDownloader(std::weak_ptr<TileProvider> provider, int maxConcurrentDownloads) :
    QObject(nullptr),
    networkManager_(new QNetworkAccessManager(this)),
    tileProvider_(provider),
    activeDownloads_(0),
    maxConcurrentDownloads_(maxConcurrentDownloads)
{
    QObject::connect(networkManager_, &QNetworkAccessManager::finished, this, &TileDownloader::onTileDownloaded, Qt::AutoConnection);
}

TileDownloader::~TileDownloader()
{
    stopAndClearRequests();
}

void TileDownloader::downloadTiles(const QList<TileIndex>& tileIndices)
{
    if (tileIndices.isEmpty()) {
        emit allDownloadsFinished();
        return;
    }

    for (const TileIndex& index : tileIndices) {
        if (!downloadQueue_.contains(index)) {
            downloadQueue_.enqueue(index);
        }
    }

    while (activeDownloads_ < maxConcurrentDownloads_ && !downloadQueue_.isEmpty()) {
        startNextDownload();
    }

    if (downloadQueue_.isEmpty() && activeDownloads_ == 0) {
        emit allDownloadsFinished();
    }
}

void TileDownloader::stopAndClearRequests()
{
    QList<QNetworkReply*> repliesToStop = activeReplies_.values();

    for (QNetworkReply* reply : repliesToStop) {
        TileIndex index = reply->property("tileIndex").value<TileIndex>();
        emit downloadStopped(index);
        reply->abort();
    }

    downloadQueue_.clear();
    activeDownloads_ = 0;
    emit allDownloadsFinished();
}

void TileDownloader::startNextDownload()
{
    if (downloadQueue_.isEmpty()) {
        return;
    }

    auto index = downloadQueue_.dequeue();
    QUrl url = tileProvider_.lock()->createURL(index);

    if (!url.isValid()) {
        qWarning() << "Constructed invalid URL for TileIndex:" << index.x_ << index.y_ << index.z_;
        emit downloadFailed(index, "Invalid URL constructed");
        return;
    }

    QNetworkRequest request(url);
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
            TileInfo info = tileProvider_.lock()->indexToTileInfo(index);
            emit tileDownloaded(index, image, info);
        }
        else {
            emit downloadFailed(index, "Failed to load image from data");
        }
    }

    activeReplies_.remove(reply);
    reply->deleteLater();
    activeDownloads_--;

    startNextDownload();

    if (downloadQueue_.isEmpty() && activeDownloads_ == 0) {
        emit allDownloadsFinished();
    }
}


} // namespace map
