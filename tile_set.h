#pragma once

#include <QVector>
#include <QReadWriteLock>

#include "map_defs.h"
#include "plotcash.h"
#include "tile_downloader.h"


namespace map {

class TileSet
{
public:
    TileSet();
    std::unordered_map<TileIndex, Tile> getTiles();


public slots:
    void addTile(TileIndex tileIndx);

protected:

private:
    std::unordered_map<TileIndex, Tile> tiles_;
    QReadWriteLock rwLocker_;
};


} // namespace map

