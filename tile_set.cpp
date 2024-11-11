#include "tile_set.h"


namespace map {

TileSet::TileSet(std::weak_ptr<TileProvider> provider, std::weak_ptr<TileDB> db, std::weak_ptr<TileDownloader> downloader) :
    QObject(nullptr),
    tileProvider_(provider),
    tileDB_(db),
    tileDownloader_(downloader)
{

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

void TileSet::onTileLoaded(const TileIndex &tileIndx, const QImage &image, const TileInfo &info)
{
    if (image.isNull()) {
        qWarning() << "TileSet::onTileLoaded: loaded 'null' image";
        return;
    }

    if (auto it = tiles_.find(tileIndx); it != tiles_.end()) {
        // write image
    }
}

void TileSet::onTileLoadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    //qWarning() << "Failed to load tile: " << tileIndx << ", error:" << errorString;

    // try to load
}

void TileSet::onTileLoadStopped(const TileIndex &tileIndx)
{
    //qDebug() << "Tile load stopped:" << tileIndx;

    // try to load
}

void TileSet::onTileDownloaded(const TileIndex &tileIndx, const QImage &image, const TileInfo &info)
{
    if (image.isNull()) {
        qWarning() << "TileSet::onTileDownloaded: downloaded 'null' image";
        return;
    }

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
        else {
            qWarning() << "TileSet::onTileDownloaded: error: datasetPtr_ is nullptr, tile vertices is not updated";
        }

        it->second.setState(Tile::State::kReady);

        if (!it->second.getInUse()) {
            emit appendSignal(it->second); // инициализируем в OpenGL что пришло
            it->second.setInUse(true);
        }
    }
}

void TileSet::onTileDownloadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    //qWarning() << "Failed to download tile from:" << tileProvider_.lock()->createURL(tileIndx) << "Error:" << errorString;
}

void TileSet::onTileDownloadStopped(const TileIndex &tileIndx)
{
    //qDebug() << "Tile download stopped from:" << tileProvider_.lock()->createURL(tileIndx);
}

bool TileSet::addTile(const TileIndex& tileIndx)
{
    auto it = tiles_.find(tileIndx);
    if (it == tiles_.end()) {
        Tile newTile(tileIndx);
        tiles_.emplace(tileIndx, std::move(newTile));
        return true;
    }

    return false;
}

void TileSet::onNewRequest(const QList<TileIndex> &request)
{    
    // stop work db
    tileDB_.lock()->stopAndClearRequests(); // ?
    // stop work downloader
    tileDownloader_.lock()->stopAndClearRequests(); // ?


    // добавление тайлов
    for (auto& itm : request) {
        addTile(itm);
    }


    // удалить текстуры тех кто был активен и стал неактивен
    // добавить текстуры новых
    // тех кто был оставить

    // инициализируем в OpenGL на onLoaded/onDownloaded
    // а удаляем тут

    // поиск в tileSet (RAM) на удаление
    for (auto& [index, tile] : tiles_) {        
        bool been = false;

        for (auto& indexSec : request) {
            if (indexSec == index) {
                been = true;
                break;
            }
        }

        if (been) {
            continue;
        }

        if (tile.getInUse()) { // был в использовании но сейчас нет
            tile.setInUse(false);
            emit deleteSignal(tile);
        }
    }

    // формируем запрос на загрузку/закачку изображения
    // все тайлы были созданы до
    QList<TileIndex> filtReq;
    for (auto& itm: request) {
        if (auto tile = tiles_.find(itm); tile != tiles_.end()) {
            if (tile->second.getImageIsNull()) { // для тайлов с 'null' image ДОБАВЛЯЕМ в запрос, в OpenGL позже проинитится
                filtReq.append(itm);
            }
            else {
                if (!tile->second.getInUse()) { // если у тайла есть изобрадение, сразу инитим в OpenGL и НЕ добавляем в запрос
                    tile->second.setInUse(true);
                    emit appendSignal(tile->second);
                }
            }
        }
    }


    // в db

    // в downloader
    tileDownloader_.lock()->downloadTiles(filtReq);
}


} // namespace map
