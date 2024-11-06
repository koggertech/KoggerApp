#pragma once

#include <QObject>
#include <QImage>
#include <QList>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QPair>

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

signals:
    void tileDownloaded(const TileIndex& tileIndx, const QImage& image, const TileInfo& info);
    void downloadFailed(const TileIndex& tileIndx, const QString& errorString);
    void allDownloadsFinished();

private slots:
    void onTileDownloaded(QNetworkReply *reply);

private:
    void startNextDownload();

    /*data*/
    QNetworkAccessManager* networkManager_;
    QQueue<TileIndex> downloadQueue_;
    int activeDownloads_;
    int maxConcurrentDownloads_;
    std::weak_ptr<TileProvider> tileProvider_;
};


} // namespace map
