#include "tile_set.h"


namespace map {

TileSet::TileSet()
{

}

std::unordered_map<TileIndex, Tile> TileSet::getTiles()
{
    QWriteLocker locker(&rwLocker_);

    auto retVal = std::move(tiles_);

    return retVal;


    return std::move(tiles_);
}

void TileSet::addTile(TileIndex tileIndx)
{
    auto it = tiles_.find(tileIndx);
    if (it == tiles_.end()) {
        Tile newTile;
        //newTile.setIndex(tileIndx);

        tiles_.emplace(tileIndx, std::move(newTile));

        //qDebug() << "added:" << tileIndx;
        // download
    }
    else {
        //qDebug() << "exists:" << tileIndx;
    }
}


} // namespace map
