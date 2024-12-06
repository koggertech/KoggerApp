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

bool TileSet::addTiles(const QList<TileIndex> &request)
{
    bool retVal{ false };
    for (auto& itm : request) {
        addTile(itm);
        retVal = true;
    }
    return retVal;
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

        removeOverlappingTilesFromRender(tile);
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

        removeOverlappingTilesFromRender(tile);

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

void TileSet::onUpdatedTextureId(const TileIndex &tileIndx, GLuint textureId)
{
    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        it->second->setTextureId(textureId);
    }
}

void TileSet::removeOverlappingTilesFromRender(const Tile &newTile)
{
    switch (lastZoomState_) {
    case map::ZoomState::kOut: {
        processOut(newTile.getIndex());
        break;
    }
    case map::ZoomState::kIn: {
        processIn(newTile.getIndex());
        break;
    }
    case map::ZoomState::kUnchanged: {
        processOut(newTile.getIndex());
        processIn(newTile.getIndex());
        break;
    }
    default:
        break;
    }
}

bool TileSet::tilesOverlap(const TileIndex &index1, const TileIndex &index2) const
{
    int minZoom = std::min(index1.z_, index2.z_);

    TileIndex idx1 = index1;
    TileIndex idx2 = index2;

    while (idx1.z_ > minZoom) {
        idx1 = idx1.getParent();
    }
    while (idx2.z_ > minZoom) {
        idx2 = idx2.getParent();
    }

    bool retVal = idx1.x_ == idx2.x_ && idx1.y_ == idx2.y_ && idx1.z_ == idx2.z_;

    return retVal;
}

void TileSet::processIn(const TileIndex &tileIndex)
{
    auto parent = tileIndex.getParent();

    bool allChildrensAreInited{ true };
    auto childs = parent.getChildren();

    for (auto& itm : childs) {
        if (auto it = tileMap_.find(itm); it != tileMap_.end()) {
            if (it->second->getImageIsNull()) {
                allChildrensAreInited = false;
                break;
            }
        }
        else {
            allChildrensAreInited = false;
            break;
        }
    }

    if (allChildrensAreInited) {
        if (auto it = tileMap_.find(parent); it != tileMap_.end()) {
            //qDebug() << "deleting PARENT" << parent;
            it->second->setInUse(false);
            emit deleteSignal(*(it->second));
        }
    }
}

void TileSet::processOut(const TileIndex &tileIndex)
{
    auto childs = tileIndex.getChildren();
    for (auto& itm : childs) {
        if (auto it = tileMap_.find(itm); it != tileMap_.end()) {
            //qDebug() << "deleting CHILDREN" << itm;
            it->second->setInUse(false);
            emit deleteSignal(*(it->second));
        }
    }
}

void TileSet::removeFarTiles(const QList<TileIndex>& request)
{
    for (auto& itmI : tileList_) {
        bool overLap = false;
        for (auto& itmJ : request) {
            if (tilesOverlap(itmJ,  itmI.getIndex())) {
                overLap = true;
                break;
            }
        }

        if (!overLap) {
            itmI.setInUse(false);
            emit deleteSignal(itmI);
        }
    }
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

void TileSet::onNewRequest(const QList<TileIndex> &request, ZoomState zoomState)
{
    if (request.isEmpty()) {
        return;
    }

    lastZoomState_ = zoomState;

    // остановить работу db, downloader
    emit requestStopAndClear();
    if (auto sharedDownloader = tileDownloader_.lock(); sharedDownloader) {
        sharedDownloader->stopAndClearRequests();
    }

    // очистить очередь иинициализации текстур
    emit clearAppendTasks();

    QCoreApplication::processEvents(QEventLoop::AllEvents);

    // добавление тайлов в tileSet
    addTiles(request);

    // обрезать размер
    checkTileSetSize();

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
                    removeOverlappingTilesFromRender(*(tile->second));
                }
                else if (!tile->second->getTextureId()) {
                    emit appendSignal(*(tile->second));
                    removeOverlappingTilesFromRender(*(tile->second));
                }
            }
        }
    }

    removeFarTiles(request);

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
