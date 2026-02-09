#pragma once

#include <QObject>
#include <QVector>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>

#include "map_defs.h"


namespace map {

class TileDB : public QObject
{
    Q_OBJECT

public:
    explicit TileDB(int32_t providerId);
    ~TileDB();

public slots:
    void init();
    void loadTiles(const QSet<map::TileIndex>& tileIndices);
    void saveTile(const map::TileIndex& tileIndx, const QImage& image);
    void stopLoading(const map::TileIndex& tileIndx);
    void stopAndClearRequests();
    void setProviderId(int32_t providerId);

signals:
    void tileLoaded(const map::TileIndex& tileIndx, const QImage& image);
    void tileLoadFailed(const map::TileIndex& tileIndx, const QString& errorString);
    void tileLoadStopped(const map::TileIndex& tileIndx);
    void tileSaved(const map::TileIndex& tileIndx);

private slots:
    void processNextTile();

private:
    void closeDb();
    QString dbNameForProvider(int32_t providerId) const;

    int32_t providerId_;
    QSqlDatabase db_;
    QSet<TileIndex> pendingLoadRequests_;
    bool stopRequested_;
};

} // namespace map

