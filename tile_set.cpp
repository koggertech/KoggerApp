#include "tile_set.h"


namespace map {

TileSet::TileSet(std::weak_ptr<TileProvider> provider, std::weak_ptr<TileDB> db, std::weak_ptr<TileDownloader> downloader, size_t maxCapacity) :
    QObject(nullptr),
    maxCapacity_(maxCapacity),
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
    if (auto it = tileMap_.find(tileIndex); it != tileMap_.end()) {
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

    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second);
        Tile& tile = *(it->second);

        if (tile.getInUse() || !tile.getImageIsNull()) {
            //qDebug() << "TileSet::onTileLoaded: tile.getInUse():" << tile.getInUse() << ", !tile.getImageIsNull()" << !tile.getImageIsNull(); // TODO
            return;
        }

        QImage temp = image;
        QTransform trans;
        trans.rotate(90);
        temp = temp.transformed(trans);

        tile.setImage(temp);
        tile.setTileInfo(info);

        if (datasetPtr_) {
            tile.updateVertices(datasetPtr_->getRef());
        }
        else {
            qWarning() << "TileSet::onTileLoaded: datasetPtr_ equals nullptr, tile vertices not updated";
        }

        tile.setState(Tile::State::kReady);

        if (!tile.getInUse()) {
            tile.setInUse(true);
            emit appendSignal(tile);
        }
    }
}

void TileSet::onTileLoadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    Q_UNUSED(errorString);

    // try to load
    QList<TileIndex> tilesToDownload;
    tilesToDownload.append(tileIndx);
    if (auto sharedDownloader = tileDownloader_.lock(); sharedDownloader) {
        sharedDownloader->downloadTiles(tilesToDownload);
    }
}

void TileSet::onTileLoadStopped(const TileIndex &tileIndx)
{
    qDebug() << "TileSet::onTileLoadStopped:" << tileIndx;
}

void TileSet::onTileDownloaded(const TileIndex &tileIndx, const QImage &image, const TileInfo &info)
{
    if (image.isNull()) {
        qWarning() << "TileSet::onTileDownloaded: downloaded 'null' image";
        return;
    }

    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second);
        Tile& tile = *(it->second);

        // skip
        if (tile.getInUse() || !tile.getImageIsNull()) {
            //qDebug() << "TileSet::onTileDownloaded: " << tileIndx<< "tile.getInUse():" << tile.getInUse() << ", !tile.getImageIsNull()" << !tile.getImageIsNull();
            return;
        }

        // image
        QImage temp = image;
        QTransform trans;
        trans.rotate(90);
        temp = temp.transformed(trans);

        tile.setImage(temp);
        tile.setTileInfo(info);

        if (datasetPtr_) {
            tile.updateVertices(datasetPtr_->getRef());
        }
        else {
            qWarning() << "TileSet::onTileDownloaded: datasetPtr_ equals nullptr, tile vertices not updated";
        }

        tile.setState(Tile::State::kReady);

        if (!tile.getInUse()) {
            tile.setInUse(true);
            emit appendSignal(tile);
        }

        emit requestSaveTile(tileIndx, image);
    }
}

void TileSet::onTileDownloadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    Q_UNUSED(tileIndx);
    Q_UNUSED(errorString);

    //qWarning() << "Failed to download tile from:" << tileProvider_.lock()->createURL(tileIndx) << "Error:" << errorString;
}

void TileSet::onTileDownloadStopped(const TileIndex &tileIndx)
{
    Q_UNUSED(tileIndx);

    //qDebug() << "Tile download stopped from:" << tileProvider_.lock()->createURL(tileIndx);
}

bool TileSet::addTile(const TileIndex& tileIndx)
{
    bool retVal{ false };
    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second); // move to front
        it->second->setRequestLastTime(QDateTime::currentDateTimeUtc());
        retVal =  false;
    }
    else {
        Tile newTile(tileIndx);
        newTile.setRequestLastTime(QDateTime::currentDateTimeUtc());
        tileList_.push_front(std::move(newTile));
        tileMap_[tileIndx] = tileList_.begin();

        if (tileMap_.size() > maxCapacity_) {
            while (tileMap_.size() > maxCapacity_) { // remove from back (map and list)
                auto lastIt = std::prev(tileList_.end());
                const TileIndex& indexToRemove = lastIt->getIndex();
                emit deleteSignal(*lastIt);
                tileMap_.erase(indexToRemove);
                tileList_.erase(lastIt);
            }
        }

        retVal = true;
    }

    return retVal;
}

void TileSet::onNewRequest(const QList<TileIndex> &request)
{
    // остановить работу db, downloader
    emit requestStopAndClear();
    if (auto sharedDownloader = tileDownloader_.lock(); sharedDownloader) {
        sharedDownloader->stopAndClearRequests();
    }

    // добавление тайлов в tileSet
    for (auto& itm : request) {
        addTile(itm);
    }

    // удалить текстуры тех кто был активен и стал неактивен
    // добавить текстуры новых
    // тех кто был оставить

    // инициализируем в OpenGL на onLoaded/onDownloaded
    // а удаляем тут

    // поиск в tileSet (RAM) на удаление в OpenGL
    for (auto& [index, tile] : tileMap_) {
        if (!request.contains(index)) {
            if (tile->getInUse()) { // был в использовании но сейчас нет
                tile->setInUse(false);
                emit deleteSignal(*tile);
            }
        }
    }

    // формируем запрос на загрузку/закачку изображения
    // все тайлы были созданы до
    QList<TileIndex> filtReq;
    for (const auto& itm : request) {
        if (auto tile = tileMap_.find(itm); tile != tileMap_.end()) {
            if (tile->second->getImageIsNull()) { // для тайлов с 'null' image ДОБАВЛЯЕМ в запрос, в OpenGL позже проинитится
                filtReq.append(itm);
            }
            else { // если у тайла есть изобрадение, сразу инитим в OpenGL и НЕ добавляем в запрос
                if (!tile->second->getInUse()) {
                    tile->second->setInUse(true);
                    emit appendSignal(*(tile->second));
                }
            }
        }
    }

    // запрос в DB
    emit requestLoadTiles(filtReq);
}


} // namespace map
