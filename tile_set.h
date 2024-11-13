#pragma once

#include <QObject>
#include <QVector>
#include <unordered_map>

#include "map_defs.h"
#include "plotcash.h"
#include "tile_provider.h"
#include "tile_db.h"
#include "tile_downloader.h"


namespace map {


class TileSet : public QObject
{
    Q_OBJECT
public:
    TileSet(std::weak_ptr<TileProvider> provider, std::weak_ptr<TileDB> db, std::weak_ptr<TileDownloader> downloader, size_t maxCapacity = 1000);

    void setDatesetPtr(Dataset* datasetPtr);
    bool isTileContains(const TileIndex& tileIndex) const;
    bool addTile(const TileIndex& tileIndx);
    void onNewRequest(const QList<TileIndex>& request);

signals:
    // OpenGL
    void appendSignal(const Tile& tile);
    void deleteSignal(const Tile& tile);
    // db
    void requestLoadTiles(const QList<TileIndex>& tileIndices);
    void requestStopAndClear();
    void requestSaveTile(const TileIndex& tileIndx, const QImage& image);

public slots:
    // db
    void onTileLoaded(const TileIndex& tileIndx, const QImage& image, const TileInfo& info);
    void onTileLoadFailed(const TileIndex& tileIndx, const QString& errorString);
    void onTileLoadStopped(const TileIndex& tileIndx);
    // downloader
    void onTileDownloaded(const TileIndex& tileIndx, const QImage& image, const TileInfo& info);
    void onTileDownloadFailed(const TileIndex& tileIndx, const QString& errorString);
    void onTileDownloadStopped(const TileIndex& tileIndx);

private:
    size_t maxCapacity_;
    std::list<Tile> tileList_; // лист тайлов
    std::unordered_map<TileIndex, std::list<Tile>::iterator> tileMap_; // хэш итераторов (least recently used)
    Dataset* datasetPtr_;
    std::weak_ptr<TileProvider> tileProvider_;
    std::weak_ptr<TileDB> tileDB_;
    std::weak_ptr<TileDownloader> tileDownloader_;
};


} // namespace map

