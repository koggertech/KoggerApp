#include "tile_downloader.h"

#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>

#ifdef Q_OS_WINDOWS
#include <QTcpSocket>
#include <QNetworkProxy>
#endif



namespace map {


TileDownloader::TileDownloader(std::weak_ptr<TileProvider> provider, int maxConcurrentDownloads) :
    QObject(nullptr),
    networkManager_(new QNetworkAccessManager(this)),
    tileProvider_(provider),
    activeDownloads_(0),
    maxConcurrentDownloads_(maxConcurrentDownloads),
    networkAvailable_(false),
    hostLookupId_(-1)
{
    qRegisterMetaType<TileIndex>("TileIndex");
    qRegisterMetaType<TileInfo>("TileInfo");

    QObject::connect(networkManager_, &QNetworkAccessManager::finished, this, &TileDownloader::onTileDownloaded, Qt::AutoConnection);

    checkNetworkAvailabilityAsync();
    networkCheckTimer_ = new QTimer();
    connect(networkCheckTimer_, &QTimer::timeout, this, &TileDownloader::checkNetworkAvailabilityAsync);
    networkCheckTimer_->start(10000);
}

TileDownloader::~TileDownloader()
{
    stopAndClearRequests();
}

void TileDownloader::downloadTiles(const QList<TileIndex>& tileIndices)
{
    if (!networkAvailable_) {
        emit allDownloadsFinished();
        return;
    }

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

    for (auto* reply : repliesToStop) {
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
            if (auto sharedProvider = tileProvider_.lock(); sharedProvider) {
                TileInfo info = sharedProvider->indexToTileInfo(index);
                emit tileDownloaded(index, image, info);
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

    if (downloadQueue_.isEmpty() && activeDownloads_ == 0) {
        emit allDownloadsFinished();
    }
}

void TileDownloader::checkNetworkAvailabilityAsync()
{
#ifdef Q_OS_ANDROID
    networkAvailable_ = true; // TODO
    // QTcpSocket socket; 
    // socket.connectToHost("8.8.8.8", 53);
    // if (socket.waitForConnected(2000)) {
    //     //qDebug() << "internet available";
    //     networkAvailable_ = true;
    //     return;
    // }
    // qDebug() << "internet UNavailable";
    // networkAvailable_= false;
#else
    if (hostLookupId_ == -1) {
        hostLookupId_ = QHostInfo::lookupHost("www.google.com", this, &TileDownloader::onHostLookupFinished);
    }
#endif
}

#ifndef Q_OS_ANDROID
void TileDownloader::onHostLookupFinished(QHostInfo hostInfo)
{
#ifdef Q_OS_ANDROID
    Q_UNUSED(hostInfo);
#else
    hostLookupId_ = -1;
    auto adresses = hostInfo.addresses();
    if (hostInfo.error() == QHostInfo::NoError && adresses.size()) {
        auto socket = new QTcpSocket();
        socket->setProxy(QNetworkProxy::DefaultProxy);
        socket->connectToHost(adresses.first(), 80);
        connect(socket, &QTcpSocket::connected,
                this, [this, socket]() {
                          networkAvailable_ = true;
                          socket->deleteLater();
                      }, Qt::AutoConnection);

        connect(socket, &QAbstractSocket::errorOccurred,
                this, [this, socket](QAbstractSocket::SocketError error) {
                          Q_UNUSED(error);
                          networkAvailable_ = false;
                          socket->deleteLater();
                      }, Qt::AutoConnection);
    }
    else {
        networkAvailable_ = false;
    }
#endif
}
#endif


} // namespace map
