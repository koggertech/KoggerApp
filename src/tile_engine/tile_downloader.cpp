#include "tile_downloader.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPainter>

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
        applyProviderHeaders(request, sharedProvider->getProviderId());
    }
    QNetworkReply* reply = networkManager_->get(request);
    reply->setProperty("tileIndex", QVariant::fromValue(index));

    activeReplies_.insert(reply);
    activeDownloads_++;
}

void TileDownloader::applyProviderHeaders(QNetworkRequest& request, int32_t providerId) const
{
    if (providerId == kOsmProviderId) {
        request.setHeader(QNetworkRequest::UserAgentHeader, "KoggerApp/1.0 (contact: support@kogger.tech)");
    } else if (providerId == kBaiduSatProviderId ||
               providerId == kBaiduSchemaProviderId ||
               providerId == kBaiduHybridProviderId) {
        request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 KoggerApp/1.0");
        request.setRawHeader("Referer", "https://map.baidu.com/");
    }
}

void TileDownloader::onTileDownloaded(QNetworkReply *reply)
{
    if (!reply || !activeReplies_.contains(reply)) {
        qWarning() << "Received a signal from an unknown or inactive reply"; // TODO
        return;
    }

    TileIndex index = reply->property("tileIndex").value<TileIndex>();
    const bool isOverlayStage = reply->property("isOverlay").toBool();

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            emit downloadStopped(index);
        }
        else if (isOverlayStage) {
            // Overlay fetch failed — fall back to the base satellite image we
            // already have, so the tile still renders without labels.
            QImage baseImage = reply->property("baseImage").value<QImage>();
            if (!baseImage.isNull()) {
                emit tileDownloaded(index, baseImage);
            } else {
                emit downloadFailed(index, reply->errorString());
            }
        }
        else {
            emit downloadFailed(index, reply->errorString());
        }
    }
    else {
        QByteArray imageData = reply->readAll();
        QImage image;
        const bool decoded = image.loadFromData(imageData);

        if (isOverlayStage) {
            // Stage 2: composite overlay onto the previously downloaded base.
            QImage baseImage = reply->property("baseImage").value<QImage>();
            if (!baseImage.isNull()) {
                if (decoded && !image.isNull()) {
                    QImage merged = baseImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
                    QPainter painter(&merged);
                    painter.drawImage(merged.rect(), image, image.rect());
                    painter.end();
                    emit tileDownloaded(index, merged);
                } else {
                    // Couldn't decode overlay — graceful fallback to base only.
                    emit tileDownloaded(index, baseImage);
                }
            } else if (decoded) {
                emit tileDownloaded(index, image);
            } else {
                emit downloadFailed(index, "Failed to load overlay image");
            }
        }
        else if (decoded) {
            // Stage 1 succeeded. If the provider wants an overlay (Baidu hybrid),
            // launch a second request and composite when it finishes; otherwise
            // emit the tile right away.
            QString overlayUrl;
            if (auto sharedProvider = tileProvider_.lock(); sharedProvider) {
                overlayUrl = sharedProvider->createOverlayURL(index);
            }

            if (overlayUrl.isEmpty()) {
                emit tileDownloaded(index, image);
            } else {
                QUrl url(overlayUrl);
                if (url.isValid()) {
                    QNetworkRequest oreq(url);
                    if (auto sharedProvider = tileProvider_.lock(); sharedProvider) {
                        applyProviderHeaders(oreq, sharedProvider->getProviderId());
                    }
                    QNetworkReply* oreply = networkManager_->get(oreq);
                    oreply->setProperty("tileIndex", QVariant::fromValue(index));
                    oreply->setProperty("isOverlay", true);
                    oreply->setProperty("baseImage", QVariant::fromValue(image));
                    activeReplies_.insert(oreply);
                    // Note: don't decrement activeDownloads_ here — the overlay
                    // reply continues to occupy this slot until it completes.
                    activeReplies_.remove(reply);
                    reply->deleteLater();
                    return;
                } else {
                    // Bad overlay URL — return base image as-is.
                    emit tileDownloaded(index, image);
                }
            }
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
