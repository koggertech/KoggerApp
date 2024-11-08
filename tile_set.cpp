#include "tile_set.h"


namespace map {

TileSet::TileSet(std::weak_ptr<TileProvider> provider) :
    QObject(nullptr),
    tileProvider_(std::move(provider))
{

}

std::unordered_map<TileIndex, Tile> TileSet::getTiles()
{
    QWriteLocker locker(&rwLocker_);

    auto retVal = std::move(tiles_);

    return retVal;
}

void TileSet::setDatesetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

bool TileSet::isTileContains(const TileIndex &tileIndex) const
{
    if (auto it = tiles_.find(tileIndex); it != tiles_.end()) {
        return true;
    }
    return false;
}

std::unordered_map<TileIndex, Tile> &TileSet::getTilesRef()
{
    return tiles_;
}

void TileSet::onTileDownloaded(const TileIndex &tileIndx, const QImage &image, const TileInfo &info)
{
    if (auto it = tiles_.find(tileIndx); it != tiles_.end()) {

        // image

        QImage temp = image;
        QTransform trans;
        trans.rotate(90);
        temp = temp.transformed(trans);

        it->second.setImage(temp);
        it->second.setTileInfo(info);

        if (datasetPtr_) {
            it->second.updateVertices(datasetPtr_->getRef());
        }

        it->second.setState(Tile::State::kReady);
        emit dataUpdated();
    }
}

void TileSet::onTileDownloadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    qWarning() << "Failed to download tile from:" << tileProvider_.lock()->createURL(tileIndx) << "Error:" << errorString;
}

void TileSet::onTileDownloadStopped(const TileIndex &tileIndx)
{
    qDebug() << "Tile download stopped from:" << tileProvider_.lock()->createURL(tileIndx);
}

void TileSet::addTile(const TileIndex& tileIndx)
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

void TileSet::onNewRequest(const QList<TileIndex> &request)
{
    // удалить текстуры тех кто был активен и стал неактивен
    // добавить текстуры новых
    // тех кто был оставить

    //qDebug() <<  "tiles_.size() << resps.size() " << tiles_.size() << request.size();

    for (auto& [index, tile] : tiles_) {
        bool been = false;

        for (const auto& indexSec : request) {
            if (indexSec == index) {
                if (!tile.getTextureId()) {
                    tile.setNeedToInit(true);
                }
                been = true;
                break;
            }
        }

        if (!been) {
            if (tile.getTextureId()) {
                tile.setNeedToDeinit(true);
            }
        }

        tile.setInUse(been);
    }

    emit dataUpdated();
}

void TileSet::deleteAllTextures()
{

}

} // namespace map
