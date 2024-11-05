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

public slots:
    void getRectRequest(QVector<QVector3D> rect);

private:
    std::unique_ptr<TileSet> tileSet_;
    std::shared_ptr<TileProvider> tileProvider_;
    std::unique_ptr<TileDownloader> tileDownloader_;
    std::unique_ptr<TileDB> tileDB_;
};


} // namespace map

