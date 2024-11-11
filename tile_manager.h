#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QDateTime>
#include <memory>

#include "tile_set.h"
#include "tile_provider.h"
#include "tile_downloader.h"
#include "tile_db.h"


namespace map {


class TileManager : public QObject
{
    Q_OBJECT
public:
    explicit TileManager(QObject *parent = nullptr);
    virtual ~TileManager();

    std::shared_ptr<TileSet> getTileSetPtr() const;
public slots:
    void getRectRequest(QVector<QVector3D> request);

private:
    std::shared_ptr<TileProvider> tileProvider_;
    std::shared_ptr<TileDownloader> tileDownloader_;
    std::shared_ptr<TileDB> tileDB_;
    std::shared_ptr<TileSet> tileSet_;

    int lastZoomLevel_ = -1;
};


} // namespace map

