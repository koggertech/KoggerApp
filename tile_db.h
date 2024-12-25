#pragma once

#include <QObject>
#include <QVector>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>

#include "map_defs.h"
#include "tile_provider.h"


namespace map {

class TileDB : public QObject
{
    Q_OBJECT

public:
    explicit TileDB(std::weak_ptr<TileProvider> tileProvider);
    ~TileDB();

public slots:
    void init();
    void loadTiles(const QSet<TileIndex>& tileIndices);
    void saveTile(const TileIndex& tileIndx, const QImage& image);
    void stopLoading(const TileIndex& tileIndx);
    void stopAndClearRequests();

signals:
    void tileLoaded(const TileIndex& tileIndx, const QImage& image);
    void tileLoadFailed(const TileIndex& tileIndx, const QString& errorString);
    void tileLoadStopped(const TileIndex& tileIndx);
    void tileSaved(const TileIndex& tileIndx);

private slots:
    void processNextTile();

private:
    std::weak_ptr<TileProvider> tileProvider_;
    QSqlDatabase db_;
    QSet<TileIndex> pendingLoadRequests_;
    bool stopRequested_;
};

} // namespace map

