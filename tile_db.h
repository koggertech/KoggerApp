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

    void loadTiles(const QList<TileIndex>& tileIndices);
    void saveTile(const TileIndex& tileIndx, const QImage& image);
    void stopAndClearRequests();

public slots:
    void init();

signals:
    void tileLoaded(const TileIndex& tileIndx, const QImage& image, const TileInfo& info);
    void tileLoadFailed(const TileIndex& tileIndx, const QString& errorString);

private slots:
    void processNextTile();

private:
    std::weak_ptr<TileProvider> tileProvider_;
    QSqlDatabase db_;
    QList<TileIndex> pendingLoadRequests_;
    bool stopRequested_;
};

} // namespace map

