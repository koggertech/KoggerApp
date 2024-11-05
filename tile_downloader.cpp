#include "tile_downloader.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>


namespace map {

TileDownloader::TileDownloader(std::shared_ptr<TileProvider> provider, int maxConcurrentDownloads, QObject *parent)
    : QObject(parent),
      networkManager_(new QNetworkAccessManager(this)),
      activeDownloads_(0),
      maxConcurrentDownloads_(maxConcurrentDownloads),
      tileProvider_(provider)
{
    QObject::connect(networkManager_, &QNetworkAccessManager::finished, this, &TileDownloader::onTileDownloaded);
}

TileDownloader::~TileDownloader()
{

}

void TileDownloader::downloadTiles(const QList<TileIndex>& tileIndices)
{
    if (tileIndices.isEmpty()) {
        qDebug() << "No TileIndices to download.";
        emit allDownloadsFinished();
        return;
    }

    //downloadQueue_.clear();

    for (const TileIndex& index : tileIndices) {
        if (!downloadQueue_.contains(index)) {
            downloadQueue_.enqueue(index);
        }
    }

    qDebug() << downloadQueue_.size() << "tiles for downloading.";

    while (activeDownloads_ < maxConcurrentDownloads_ && !downloadQueue_.isEmpty()) {
        startNextDownload();
    }

    if (downloadQueue_.isEmpty() && activeDownloads_ == 0) {
        qDebug() << "All downloads are already finished.";
        emit allDownloadsFinished();
    }
}

void TileDownloader::startNextDownload()
{
    if (downloadQueue_.isEmpty()) {
        return;
    }

    auto index = downloadQueue_.dequeue();

    QUrl url = tileProvider_->createURL(index);

    if (!url.isValid()) {
        qWarning() << "Constructed invalid URL for TileIndex:" << index.x_ << index.y_ << index.z_;
        emit downloadFailed(index, "Invalid URL constructed");
        return;
    }

    QNetworkRequest request(url);
    QNetworkReply* reply = networkManager_->get(request);

    reply->setProperty("tileIndex", QVariant::fromValue(index));

    activeDownloads_++;
    qDebug() << "Started downloading tile from:" << url.toString();
}

void TileDownloader::onTileDownloaded(QNetworkReply *reply)
{
    if (!reply) {
        qWarning() << "Received a finished signal from a non-QNetworkReply sender.";
        return;
    }

    TileIndex index = reply->property("tileIndex").value<TileIndex>();

    if (reply->error() != QNetworkReply::NoError) {
        emit downloadFailed(index, reply->errorString());
        qWarning() << "Failed to download tile from:" << tileProvider_->createURL(index) << "Error:" << reply->errorString();
    }
    else {
        QByteArray imageData = reply->readAll();
        QImage image;

        if (image.loadFromData(imageData)) {
            TileInfo info = tileProvider_->indexToTileInfo(index);
            emit tileDownloaded(index, image, info);
        }
        else {
            emit downloadFailed(index, "Failed to load image from data");
        }
    }

    reply->deleteLater();
    activeDownloads_--;

    startNextDownload();

    if (downloadQueue_.isEmpty() && activeDownloads_ == 0) {
        qDebug() << "All downloads finished.";
        emit allDownloadsFinished();
    }
}

} // namespace map
