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
        Tile& tile = it->second;

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
    tileDownloader_.lock()->downloadTiles(tilesToDownload);
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

    if (auto it = tiles_.find(tileIndx); it != tiles_.end()) {
        Tile& tile = it->second;

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
    if (auto it = tiles_.find(tileIndx); it == tiles_.end()) {
        Tile newTile(tileIndx);
        tiles_.emplace(tileIndx, std::move(newTile));
        return true;
    }

    return false;
}

void TileSet::onNewRequest(const QList<TileIndex> &request)
{    
    // остановить работу db, downloader
    emit requestStopAndClear();
    tileDownloader_.lock()->stopAndClearRequests();

    // добавление тайлов
    for (auto& itm : request) {
        addTile(itm);
    }

    // удалить текстуры тех кто был активен и стал неактивен
    // добавить текстуры новых
    // тех кто был оставить

    // инициализируем в OpenGL на onLoaded/onDownloaded
    // а удаляем тут

    // поиск в tileSet (RAM) на удаление в OpenGL
    for (auto& [index, tile] : tiles_) {
        if (!request.contains(index)) {
            if (tile.getInUse()) { // был в использовании но сейчас нет
                tile.setInUse(false);
                emit deleteSignal(tile);
            }
        }
    }

    // формируем запрос на загрузку/закачку изображения
    // все тайлы были созданы до
    QList<TileIndex> filtReq;
    for (const auto& itm : request) {
        if (auto tile = tiles_.find(itm); tile != tiles_.end()) {
            if (tile->second.getImageIsNull()) { // для тайлов с 'null' image ДОБАВЛЯЕМ в запрос, в OpenGL позже проинитится
                filtReq.append(itm);
            }
            else { // если у тайла есть изобрадение, сразу инитим в OpenGL и НЕ добавляем в запрос
                if (!tile->second.getInUse()) {
                    tile->second.setInUse(true);
                    emit appendSignal(tile->second);
                }
            }
        }
    }

    // запрос в DB
    emit requestLoadTiles(filtReq);
}


} // namespace map
