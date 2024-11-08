#pragma once

#include <QObject>
#include <QVector>
#include <QReadWriteLock>
#include <unordered_map>

#include "map_defs.h"
#include "plotcash.h"
#include "tile_provider.h"


namespace map {

class TileSet : public QObject
{
    Q_OBJECT

public:
    explicit TileSet(std::weak_ptr<TileProvider> provider);

    std::unordered_map<TileIndex, Tile> getTiles();
    void setDatesetPtr(Dataset* datasetPtr);
    bool isTileContains(const TileIndex& tileIndex) const;
    std::unordered_map<TileIndex, Tile>& getTilesRef();
    void addTile(const TileIndex& tileIndx);

    void onNewRequest(const QList<TileIndex>& request);
    void deleteAllTextures();

signals:
    void dataUpdated();

public slots:
    void onTileDownloaded(const TileIndex& tileIndx, const QImage& image, const TileInfo& info);
    void onTileDownloadFailed(const TileIndex& tileIndx, const QString& errorString);
    void onTileDownloadStopped(const TileIndex& tileIndx);

protected:

private:
    std::unordered_map<TileIndex, Tile> tiles_;
    QReadWriteLock rwLocker_;

    Dataset* datasetPtr_;
    std::weak_ptr<TileProvider> tileProvider_;
};


} // namespace map

