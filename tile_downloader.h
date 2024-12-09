#pragma once

#include <QObject>
#include <QImage>
#include <QList>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QPair>
#include <QSet>
#ifdef Q_OS_WINDOWS
#include <QHostInfo>
#endif
#include <QTimer>

#include "map_defs.h"
#include "tile_provider.h"


namespace map {


class TileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit TileDownloader(std::weak_ptr<TileProvider> provider, int maxConcurrentDownloads = 5);
    ~TileDownloader();

    void downloadTiles(const QList<TileIndex>& tiles);
    void stopAndClearRequests();

signals:
    void tileDownloaded(const TileIndex& tileIndx, const QImage& image, const TileInfo& info);
    void downloadFailed(const TileIndex& tileIndx, const QString& errorString);
    void downloadStopped(const TileIndex& tileIndx);
    void allDownloadsFinished();

private slots:
    void onTileDownloaded(QNetworkReply *reply);
    void checkNetworkAvailabilityAsync();
#ifdef Q_OS_WINDOWS
    void onHostLookupFinished(QHostInfo hostInfo);
#endif

private:
    void startNextDownload();

    /*data*/
    QNetworkAccessManager* networkManager_;
    std::weak_ptr<TileProvider> tileProvider_;
    QQueue<TileIndex> downloadQueue_;
    QSet<QNetworkReply*> activeReplies_;
    int activeDownloads_;
    int maxConcurrentDownloads_;
    bool networkAvailable_;
    QTimer* networkCheckTimer_;
    int hostLookupId_;
};


} // namespace map
