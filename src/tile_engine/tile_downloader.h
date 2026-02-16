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

#include "map_defs.h"
#include "tile_provider.h"


namespace map {


class TileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit TileDownloader(std::weak_ptr<TileProvider> provider, int maxConcurrentDownloads = 5);
    ~TileDownloader();

    bool downloadTile(const TileIndex& tile);
    void stopAndClearRequests();
    void deleteRequest(const TileIndex& tileIndx);
    void setProvider(std::weak_ptr<TileProvider> provider);
    void setNetworkAvailable(bool available);
    bool isNetworkAvailable() const;

signals:
    void tileDownloaded(const map::TileIndex& tileIndx, const QImage& image);
    void downloadFailed(const map::TileIndex& tileIndx, const QString& errorString);
    void downloadStopped(const map::TileIndex& tileIndx);

private slots:
    void onTileDownloaded(QNetworkReply *reply);

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
};


} // namespace map
