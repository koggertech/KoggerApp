#pragma once

#include <QObject>
#include <QVector>
#include <unordered_map>

#include "map_defs.h"
#include "plotcash.h"
#include "tile_provider.h"
#include "tile_db.h"
#include "tile_downloader.h"
#include "plotcash.h"


namespace map {


class TileSet : public QObject
{
    Q_OBJECT
public:
    TileSet(std::weak_ptr<TileProvider> provider, std::weak_ptr<TileDB> db, std::weak_ptr<TileDownloader> downloader, size_t maxCapacity = 1000, size_t minCapacity = 500);

    void onNewRequest(const QSet<TileIndex>& request, ZoomState zoomState, LLARef viewLlaRef, bool isPerspective, double minLat, double maxLat, double minLon, double maxLon);
    void setTextureIdByTileIndx(const map::TileIndex& tileIndx, GLuint textureId);

signals:
    // TileDB
    void dbLoadTiles(const QSet<TileIndex>& tileIndices);
    void dbStopAndClearTasks();
    void dbStopLoadingTile(const TileIndex& tileIndx);
    void dbSaveTile(const TileIndex& tileIndx, const QImage& image);
    // MapView
    void mvAppendTile(const Tile& tile);
    void mvDeleteTile(const Tile& tile);
    void mvUpdateTileVertices(const Tile& tile);
    void mvClearAppendTasks();

public slots:
    // TileDB
    void onTileLoaded(const TileIndex& tileIndx, const QImage& image, const TileInfo& info);
    void onTileLoadFailed(const TileIndex& tileIndx, const QString& errorString);
    void onTileLoadStopped(const TileIndex& tileIndx);
    // TileDownloader
    void onTileDownloaded(const TileIndex& tileIndx, const QImage& image, const TileInfo& info);
    void onTileDownloadFailed(const TileIndex& tileIndx, const QString& errorString);
    void onTileDownloadStopped(const TileIndex& tileIndx);
    // MapView
    void onNotUsed(const TileIndex& tileIndx);

private:
    /*methods*/
    bool addTiles(const QSet<TileIndex>& request);
    bool addTile(const TileIndex& tileIndx);
    void tryShrinkSetSize();
    void removeOverlappingTilesFromRender(const Tile& newTile);
    bool tilesOverlap(const TileIndex& index1, const TileIndex& index2, int zoomStepEdge = -1) const;
    void processIn(const TileIndex& tileIndex);
    void processOut(const TileIndex& tileIndex);
    void removeFarTilesFromRender(const QSet<TileIndex>&  request);
    void updateTileVerticesInRender(const QSet<TileIndex>& request);
    void drawNumberOnImage(QImage& image, const TileIndex& tileIndx, const QColor& color = Qt::yellow) const;

    void removeFarDBRequests(const QSet<TileIndex>& request);
    void removeFarDownloaderRequests(const QSet<TileIndex>& request);

    /*data*/
    size_t maxCapacity_;
    size_t minCapacity_;
    std::list<Tile> tileList_; // лист тайлов
    std::unordered_map<TileIndex, std::list<Tile>::iterator> tileMap_; // хэш итераторов (least recently used)
    std::weak_ptr<TileProvider> tileProvider_;
    std::weak_ptr<TileDB> tileDB_;
    std::weak_ptr<TileDownloader> tileDownloader_;
    bool isPerspective_;
    LLARef viewLlaRef_;
    ZoomState zoomState_;
    double minLat_;
    double maxLat_;
    double minLon_;
    double maxLon_;
    QSet<TileIndex> request_;
    int32_t currZoom_;
    int32_t diffLevels_;
    QSet<TileIndex> dbReq_;
    QSet<TileIndex> dwReq_;

    const int propagationLevel_ = 2;
};


} // namespace map

