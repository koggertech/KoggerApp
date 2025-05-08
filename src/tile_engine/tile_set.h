#pragma once

#include <QObject>
#include <QVector>
#include <unordered_map>

#include "map_defs.h"
#include "dataset.h"
#include "tile_provider.h"
#include "tile_db.h"
#include "tile_downloader.h"


namespace map {


class TileSet : public QObject
{
    Q_OBJECT
public:
    TileSet(std::weak_ptr<TileProvider> provider, std::weak_ptr<TileDB> db, std::weak_ptr<TileDownloader> downloader, size_t maxCapacity = 1000, size_t minCapacity = 500);

    void onNewRequest(const QSet<TileIndex>& request, ZoomState zoomState, LLARef viewLlaRef, bool isPerspective, double minLon, double maxLon, bool moveUp);
    void onNewLlaRef(LLARef viewLlaRef);
    void setTextureIdByTileIndx(const map::TileIndex& tileIndx, GLuint textureId);

signals:
    // TileDB
    void dbLoadTiles(const QSet<TileIndex>& tileIndices);
    void dbStopAndClearTasks();
    void dbStopLoadingTile(const TileIndex& tileIndx);
    void dbSaveTile(const TileIndex& tileIndx, const QImage& image);
    // MapView
    void mvAppendTile(const Tile& tile);
    void mvDeleteTile(const TileIndex& tileIndx);
    void mvUpdateTileImage(const TileIndex& tileIndx, const QImage& image);
    void mvUpdateTileVertices(const TileIndex& tile, const QVector<QVector3D>& vertices);
    void mvClearAppendTasks();

public slots:
    // TileDB
    void onTileLoaded(const TileIndex& tileIndx, const QImage& image);
    void onTileLoadFailed(const TileIndex& tileIndx, const QString& errorString);
    void onTileLoadStopped(const TileIndex& tileIndx);
    void onTileSaved(const TileIndex& tileIndx);
    // TileDownloader
    void onTileDownloaded(const TileIndex& tileIndx, const QImage& image);
    void onTileDownloadFailed(const TileIndex& tileIndx, const QString& errorString);
    void onTileDownloadStopped(const TileIndex& tileIndx);
    // MapView
    void onDeletedFromAppend(const TileIndex& tileIndx);

private:
    /*methods*/
    bool addTiles(const QSet<TileIndex>& request);
    bool addTile(const TileIndex& tileIndx);
    void tryReduceSetSize();
    bool processIn(Tile* tile) const;
    bool processOut(Tile* tile) const;
    void removeFarTilesFromRender(const QSet<TileIndex>& request);
    void drawNumberOnImage(QImage& image, const TileIndex& tileIndx, const QColor& color = Qt::yellow) const;
    bool tryRenderTile(Tile& tile, bool force = false);
    Tile* getTileByIndx(const TileIndex& tileIndx) const;
    void removeFarDBRequests(const QSet<TileIndex>& request);
    void removeFarDWRequests(const QSet<TileIndex>& request);
    bool tryCopyImage(Tile* tile) const;
    bool updateVertices(Tile* tile) const;

    /*data*/
    size_t maxCapacity_;
    size_t minCapacity_;
    std::list<Tile> tileList_; // LRU
    std::unordered_map<TileIndex, std::list<Tile>::iterator> tileHash_;
    std::weak_ptr<TileProvider> tileProvider_;
    std::weak_ptr<TileDB> tileDB_;
    std::weak_ptr<TileDownloader> tileDownloader_;
    bool isPerspective_;
    LLARef viewLlaRef_;
    ZoomState zoomState_;
    double minLon_;
    double maxLon_;
    QSet<TileIndex> request_;
    int32_t currZoom_;
    int32_t diffLevels_;
    QSet<TileIndex> dbReq_;
    QSet<TileIndex> dwReq_;
    QSet<TileIndex> dbSvd_;
    bool moveUp_;
    QSize defaultSize_;
    QImage::Format defaultImageFormat_;

    const int propagationLevel_ = 2;
};


} // namespace map

