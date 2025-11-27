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
    void loadTiles(const QSet<map::TileIndex>& tileIndices);
    void saveTile(const map::TileIndex& tileIndx, const QImage& image);
    void stopLoading(const map::TileIndex& tileIndx);
    void stopAndClearRequests();

signals:
    void tileLoaded(const map::TileIndex& tileIndx, const QImage& image);
    void tileLoadFailed(const map::TileIndex& tileIndx, const QString& errorString);
    void tileLoadStopped(const map::TileIndex& tileIndx);
    void tileSaved(const map::TileIndex& tileIndx);

private slots:
    void processNextTile();

private:
    std::weak_ptr<TileProvider> tileProvider_;
    QSqlDatabase db_;
    QSet<TileIndex> pendingLoadRequests_;
    bool stopRequested_;
};

} // namespace map

