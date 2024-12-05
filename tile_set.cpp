#include "tile_set.h"


namespace map {

TileSet::TileSet(std::weak_ptr<TileProvider> provider, std::weak_ptr<TileDB> db, std::weak_ptr<TileDownloader> downloader, size_t maxCapacity, size_t minCapacity) :
    QObject(nullptr),
    maxCapacity_(maxCapacity),
    minCapacity_(minCapacity),
    tileProvider_(provider),
    tileDB_(db),
    tileDownloader_(downloader),
    isPerspective_(false)
{
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
            return;
        }

        QImage temp = image;
        QTransform trans;
        trans.rotate(90);
        temp = temp.transformed(trans);

        tile.setImage(temp);

        auto updatedInfo = tileProvider_.lock()->indexToTileInfo(tile.getIndex(), getTilePosition(minLon_, maxLon_, info));
        tile.setTileInfo(updatedInfo);
        //tile.setTileInfo(info);

        tile.updateVertices(viewLlaRef_, isPerspective_);
        tile.setState(Tile::State::kReady);

        if (!tile.getInUse()) {
            tile.setInUse(true);
            emit appendSignal(tile);
        }
        //removeOverlappingTiles(tile);
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
            return;
        }

        // image
        QImage temp = image;
        QTransform trans;
        trans.rotate(90);
        temp = temp.transformed(trans);

        tile.setImage(temp);

        auto updatedInfo = tileProvider_.lock()->indexToTileInfo(tile.getIndex(), getTilePosition(minLon_, maxLon_, info));
        tile.setTileInfo(updatedInfo);
        //tile.setTileInfo(info);

        tile.updateVertices(viewLlaRef_, isPerspective_);
        tile.setState(Tile::State::kReady);

        if (!tile.getInUse()) {
            tile.setInUse(true);
            emit appendSignal(tile);
        }

        emit requestSaveTile(tileIndx, image);
        //removeOverlappingTiles(tile);
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

void TileSet::onUpdatedTextureId(const TileIndex &tileIndx, GLuint textureId)
{
    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        it->second->setTextureId(textureId);
    }
}

void TileSet::removeOverlappingTiles(const Tile &newTile)
{
    for (auto it = tileList_.begin(); it != tileList_.end(); )
    {
        Tile &existingTile = *it;

        if (&existingTile == &newTile) {
            ++it;
            continue;
        }

        if (existingTile.getPendingRemoval() && existingTile.getInUse()) {
            if (tilesOverlap(existingTile.getIndex(), newTile.getIndex())) {
                existingTile.setInUse(false);

                emit deleteSignal(existingTile);

                tileMap_.erase(existingTile.getIndex());
                it = tileList_.erase(it);
            }
            else {
                ++it;
            }
        }
        else {
            ++it;
        }
    }
}

bool TileSet::tilesOverlap(const TileIndex &index1, const TileIndex &index2) const
{
    /*   int minZoom = std::min(index1.z_, index2.z_);

    TileIndex idx1 = index1;
    TileIndex idx2 = index2;

    while (idx1.z_ > minZoom) {
        idx1 = idx1.getParent();
    }
    while (idx2.z_ > minZoom) {
        idx2 = idx2.getParent();
    }

    bool retVal = idx1.x_ == idx2.x_ && idx1.y_ == idx2.y_ && idx1.z_ == idx2.z_;
    //qDebug() << "tilesOverlap" << retVal;
    return retVal;*/

    int minZoom = std::min(index1.z_, index2.z_);
    int maxZoom = std::max(index1.z_, index2.z_);

    int zoomDifference = maxZoom - minZoom;
    int scale = 1 << zoomDifference; // 2^zoomDifference

    const TileIndex* lowerZoomTile;
    const TileIndex* higherZoomTile;

    if (index1.z_ < index2.z_) {
        lowerZoomTile = &index1;
        higherZoomTile = &index2;
    } else {
        lowerZoomTile = &index2;
        higherZoomTile = &index1;
    }

    int lowerXMin = lowerZoomTile->x_ * scale;
    int lowerXMax = lowerXMin + scale - 1;
    int lowerYMin = lowerZoomTile->y_ * scale;
    int lowerYMax = lowerYMin + scale - 1;

    int higherX = higherZoomTile->x_;
    int higherY = higherZoomTile->y_;

    bool xOverlap = (higherX >= lowerXMin) && (higherX <= lowerXMax);
    bool yOverlap = (higherY >= lowerYMin) && (higherY <= lowerYMax);

    return xOverlap && yOverlap;
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
        retVal = true;
    }

    checkTileSetSize();

    return retVal;
}

void TileSet::checkTileSetSize()
{
    if (tileMap_.size() > maxCapacity_) {
        while (tileMap_.size() > minCapacity_) { // remove from back (map and list)
            auto lastIt = std::prev(tileList_.end());
            const TileIndex& indexToRemove = lastIt->getIndex();

            emit deleteSignal(*lastIt);

            tileMap_.erase(indexToRemove);
            tileList_.erase(lastIt);
        }
    }
}

void TileSet::setIsPerspective(bool state)
{
    isPerspective_ = state;
}

void TileSet::setViewLla(LLARef viewLlaRef)
{
    viewLlaRef_ = viewLlaRef;
}

void TileSet::onNewRequest(const QList<TileIndex> &request)
{
    // остановить работу db, downloader
    emit requestStopAndClear();
    if (auto sharedDownloader = tileDownloader_.lock(); sharedDownloader) {
        sharedDownloader->stopAndClearRequests();
    }

    // очистить очередь иинициализации текстур
    emit clearAppendTasks();

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // добавление тайлов в tileSet
    for (auto& itm : request) {
        addTile(itm);
    }

    // удалить текстуры тех кто был активен и стал неактивен
    // добавить текстуры новых
    // тех кто был оставить

    // инициализируем в OpenGL на onLoaded/onDownloaded
    // а удаляем тут

    // удалить неиспользуемые
    for (auto& [index, tile] : tileMap_) {
        if (!request.contains(index)) {
            if (tile->getInUse()) { // был в использовании но сейчас нет
                tile->setInUse(false);
                emit deleteSignal(*tile);
            }
        }
    }

    // формируем запрос на загрузку/закачку изображение, все тайлы были созданы до
    QList<TileIndex> filtReq;
    for (const auto& itm : request) {
        if (auto tile = tileMap_.find(itm); tile != tileMap_.end()) {
            if (tile->second->getImageIsNull()) { // для тайлов с 'null' image ДОБАВЛЯЕМ в запрос, в OpenGL позже проинитится
                filtReq.append(itm);
            }
            else {
                // обновить вершины
                auto updatedInfo = tileProvider_.lock()->indexToTileInfo(tile->second->getIndex(), getTilePosition(minLon_, maxLon_, tile->second->getTileInfo()));
                tile->second->setTileInfo(updatedInfo);
                tile->second->updateVertices(viewLlaRef_, isPerspective_);
                emit updVertSignal(*(tile->second));

                // отдать на инит текстуры
                if (!tile->second->getInUse()) {
                    tile->second->setInUse(true);
                    emit appendSignal(*(tile->second));
                }
                else if (!tile->second->getTextureId()) {
                    emit appendSignal(*(tile->second));
                }
            }
        }
    }

    // запрос в DB
    emit requestLoadTiles(filtReq);
}

void TileSet::setEyeView(double minLat, double maxLat, double minLon, double maxLon)
{
    minLat_ = minLat;
    maxLat_ = maxLat;
    minLon_ = minLon;
    maxLon_ = maxLon;
}


} // namespace map
