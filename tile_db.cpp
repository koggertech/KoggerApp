#include "tile_db.h"


namespace map {

TileDB::TileDB(std::weak_ptr<TileProvider> tileProvider) :
    QObject(nullptr),
    tileProvider_(tileProvider)
{

}

void TileDB::stopAndClearRequests()
{

}

} // namespace map
