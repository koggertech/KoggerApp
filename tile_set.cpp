#include "tile_set.h"


namespace map {

TileSet::TileSet(std::weak_ptr<TileProvider> provider, std::weak_ptr<TileDB> db, std::weak_ptr<TileDownloader> downloader, size_t maxCapacity, size_t minCapacity) :
    QObject(nullptr),
    maxCapacity_(maxCapacity),
    minCapacity_(minCapacity),
    tileProvider_(provider),
    tileDB_(db),
    tileDownloader_(downloader),
    isPerspective_(false),
    zoomState_(ZoomState::kUndefined),
    minLat_(0.0),
    maxLat_(0.0),
    minLon_(0.0),
    maxLon_(0.0),
    currZoom_(-1),
    diffLevels_(std::numeric_limits<int32_t>::max())
{
    qRegisterMetaType<Tile>("Tile");
    qRegisterMetaType<QSet<TileIndex>>("QSet<TileIndex>");
}

bool TileSet::addTiles(const QSet<TileIndex>& request)
{
    bool retVal{ false };
    for (auto& itm : request) {
        if (addTile(itm)) {
            retVal = true;
        }
    }
    return retVal;
}

void TileSet::onTileLoaded(const TileIndex &tileIndx, const QImage &image, const TileInfo &info)
{
    //qDebug() << "Loaded tile:" << tileIndx;

    dbReq_.remove(tileIndx);

    if (image.isNull()) {
        qWarning() << "TileSet::onTileLoaded: loaded 'null' image";
        return;
    }

    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second);
        Tile& tile = *(it->second);

        QImage tmpImage = image;
        //drawNumberOnImage(tmpImage, tileIndx);
        QTransform trans;
        trans.rotate(90);
        tmpImage = tmpImage.transformed(trans);

        tile.setImage(tmpImage);
        tile.setOriginTileInfo(info);
        tile.setState(Tile::State::kReady);

        tryRenderTile(tile);
    }
}

void TileSet::onTileLoadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    dbReq_.remove(tileIndx);

    Q_UNUSED(errorString);

    // try to download
    if (auto sharedDownloader = tileDownloader_.lock(); sharedDownloader) {
        if (!dwReq_.contains(tileIndx) && !dbSvd_.contains(tileIndx)) {
            dwReq_.insert(tileIndx);
            sharedDownloader->downloadTile(tileIndx);
        }
    }
}

void TileSet::onTileLoadStopped(const TileIndex &tileIndx)
{
    dbReq_.remove(tileIndx);
}

void TileSet::onTileSaved(const TileIndex &tileIndx)
{
    dbSvd_.remove(tileIndx);
}

void TileSet::onTileDownloaded(const TileIndex &tileIndx, const QImage &image, const TileInfo &info)
{
    //qDebug() << "Downloaded tile:" << tileIndx;

    dwReq_.remove(tileIndx);

    if (image.isNull()) {
        qWarning() << "TileSet::onTileDownloaded: downloaded 'null' image";
        return;
    }

    dbSvd_.insert(tileIndx);
    emit dbSaveTile(tileIndx, image);

    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second);
        Tile& tile = *(it->second);

        // image
        QImage tmpImage = image;
        //drawNumberOnImage(tmpImage, tileIndx);
        QTransform trans;
        trans.rotate(90);
        tmpImage = tmpImage.transformed(trans);

        tile.setImage(tmpImage);
        tile.setOriginTileInfo(info);
        tile.setState(Tile::State::kReady);

        tryRenderTile(tile);
    }
}

void TileSet::onTileDownloadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    dwReq_.remove(tileIndx);

    Q_UNUSED(tileIndx);
    Q_UNUSED(errorString);

    qWarning() << "Failed to download tile" << tileIndx << "from:" << tileProvider_.lock()->createURL(tileIndx) << "Error:" << errorString;
}

void TileSet::onTileDownloadStopped(const TileIndex &tileIndx)
{
    dwReq_.remove(tileIndx);
}

void TileSet::onNotUsed(const TileIndex &tileIndx)
{
    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        it->second->setInUse(false);
    }
}

bool TileSet::tilesOverlap(const TileIndex &index1, const TileIndex &index2, int zoomStepEdge) const
{
    int minZoom = std::min(index1.z_, index2.z_);

    TileIndex idx1 = index1;
    TileIndex idx2 = index2;

    if (zoomStepEdge != -1) {
        int zDiff = std::abs(idx1.z_ - idx2.z_);
        if (std::abs(zoomStepEdge) < zDiff) {
            return false;
        }
    }

    while (idx1.z_ > minZoom) {
        idx1 = idx1.getParent().first;
    }
    while (idx2.z_ > minZoom) {
        idx2 = idx2.getParent().first;
    }

    bool retVal = idx1.x_ == idx2.x_ && idx1.y_ == idx2.y_ && idx1.z_ == idx2.z_;

    return retVal;
}

void TileSet::processIn(const TileIndex &tileIndx)
{
    // 1 ---> 21
    int depth = std::min(diffLevels_, propagationLevel_);
    TileIndex currentTileIndx = tileIndx;
    bool canRenderTile = true;
    QList<TileIndex> parentsToRemove;
    bool lastChildsBeenAll = false;

    TileIndex nearbyValidParentIndx;

    auto calcNumOverlappingTiles = [this](const std::vector<TileIndex>& childs) -> int {
        int retVal = 0;
        for (const auto& child : childs) {
            if (!request_.contains(child)) {
                continue;
            }
            ++retVal;
        }
        return retVal;
    };

    auto updateVertices = [this](Tile* tile, bool emitUpd = true) -> void {
        auto updatedInfo = tileProvider_.lock()->indexToTileInfo(tile->getIndex(), getTilePosition(minLon_, maxLon_, tile->getOriginTileInfo()));
        tile->setModifiedTileInfo(updatedInfo);
        tile->updateVertices(viewLlaRef_, isPerspective_);

        if (emitUpd) {
            emit mvUpdateTileVertices(*tile);
        }
    };

    for (int currentDepth = 1; currentDepth <= depth; ++currentDepth) {
        bool parentInRender = false;
        auto parentIndx = currentTileIndx.getParent().first;

        if (lastChildsBeenAll) {
            parentsToRemove.append(parentIndx);
        }
        else {
            auto* pTile = getTileByIndx(parentIndx);
            if (pTile && pTile->getInUse()) {
                parentInRender = true;
            }

            if (parentInRender) {
                bool allChildsHaveImage = true;
                auto childs = parentIndx.getChilds(currentDepth).first; // на уровне запроса

                for (const auto& child : childs) {
                    if (!request_.contains(child)) {
                        continue;
                    }
                    auto* cTile = getTileByIndx(child);
                    if (cTile) {
                        if (cTile->getImageIsNull()) {
                            allChildsHaveImage = false;
                            break;
                        }
                    }
                }

                if (allChildsHaveImage) {
                    lastChildsBeenAll = true;
                    parentsToRemove.append(parentIndx);
                }
                else {
                    if (!nearbyValidParentIndx.isValid()) {
                        nearbyValidParentIndx = parentIndx;
                        updateVertices(pTile);
                    }
                    else {
                        // проверить предуыущий следующий
                        auto prevParentChilds = parentIndx.getChilds().first;

                        if (prevParentChilds.empty()) {
                            parentsToRemove.append(nearbyValidParentIndx);
                            nearbyValidParentIndx = parentIndx;
                            updateVertices(pTile);
                        }
                        else {
                            int currNumOvlpTiles = calcNumOverlappingTiles(childs);
                            int prevNumOvlpTiles = calcNumOverlappingTiles(prevParentChilds);

                            if (currNumOvlpTiles > prevNumOvlpTiles) {
                                parentsToRemove.append(nearbyValidParentIndx);
                                nearbyValidParentIndx = parentIndx;
                                updateVertices(pTile);
                            }
                            else {
                                parentsToRemove.append(parentIndx);
                            }
                        }
                    }

                    canRenderTile = false;
                }
            }
            else {
                canRenderTile = true;
            }
        }

        currentTileIndx = parentIndx;
    }

    for (const auto& parent : parentsToRemove) {
        auto* pTile = getTileByIndx(parent);
        if (pTile) {
            pTile->setInUse(false);
            emit mvDeleteTile(*pTile);
        }
    }

    if (canRenderTile) {
        auto parent = tileIndx.getParent().first;
        auto childs = parent.getChilds().first;

        for (const auto& child : childs) {
            if (!request_.contains(child)) {
                continue;
            }
            auto* cTile = getTileByIndx(child);
            if (cTile) {
                if (!cTile->getImageIsNull() && !cTile->getInUse()) {
                    updateVertices(cTile, false);
                    cTile->setInUse(true);
                    emit mvAppendTile(*cTile);
                }
            }
        }
    }
    else {
        //qDebug() << "tile" << tileIndx << "ne otrisuetsa!!!";
    }
}

void TileSet::processOut(const TileIndex &tileIndx)
{    
    // 21 ---> 1
    int depth = std::min(diffLevels_, propagationLevel_);
    auto currTileIndx = tileIndx;

    for (int currentDepth = 1; currentDepth <= depth; ++currentDepth) {

        auto childs = currTileIndx.getChilds(currentDepth).first;

        for (auto& itm : childs) {
            auto* cTile = getTileByIndx(itm);

            if (cTile) {
                if (cTile->getInUse()) {
                    cTile->setInUse(false);
                    emit mvDeleteTile(*(cTile));
                }
            }
        }
    }

    auto* currTile = getTileByIndx(currTileIndx);
    if (currTile) {
        if (!currTile->getInUse()) {
            currTile->setInUse(true);
            emit mvAppendTile(*currTile);
        }
    }

}

void TileSet::processUnchanged(const TileIndex &tileIndx)
{
    int depth = propagationLevel_;

    // OUT
    // delete all possible childs
    for (int currentDepth = 1; currentDepth <= depth; ++currentDepth) {
        auto childs = tileIndx.getChilds(currentDepth).first;
        for (auto& itm : childs) {
            auto* cTile = getTileByIndx(itm);

            if (cTile) {
                if (cTile->getInUse()) {
                    cTile->setInUse(false);
                    emit mvDeleteTile(*(cTile));
                }
            }
        }
    }

    // IN
    // 1 ---> 21
    bool canRenderTile = true;
    QList<TileIndex> parentsToRemove;
    bool lastChildsBeenAll = false;
    auto currTileIndx = tileIndx;

    TileIndex nearbyValidParentIndx;

    auto calcNumOverlappingTiles = [this](const std::vector<TileIndex>& childs) -> int {
        int retVal = 0;
        for (const auto& child : childs) {
            if (!request_.contains(child)) {
                continue;
            }
            ++retVal;
        }
        return retVal;
    };


    auto updateVertices = [this](Tile* tile, bool emitUpd = true) -> void {
        auto updatedInfo = tileProvider_.lock()->indexToTileInfo(tile->getIndex(), getTilePosition(minLon_, maxLon_, tile->getOriginTileInfo()));
        tile->setModifiedTileInfo(updatedInfo);
        tile->updateVertices(viewLlaRef_, isPerspective_);

        if (emitUpd) {
            emit mvUpdateTileVertices(*tile);
        }
    };

    for (int currentDepth = 1; currentDepth <= depth; ++currentDepth) {

        bool parentInRender = false;
        auto parentIndx = currTileIndx.getParent().first;

        if (lastChildsBeenAll) {
            parentsToRemove.append(parentIndx);
        }
        else {
            auto* pTile = getTileByIndx(parentIndx);
            if (pTile && pTile->getInUse()) {
                parentInRender = true;
            }

            if (parentInRender) {
                bool allChildsHaveImage = true;
                auto childs = parentIndx.getChilds(currentDepth).first; // на уровне запроса

                for (const auto& child : childs) {
                    if (!request_.contains(child)) {
                        continue;
                    }
                    auto* cTile = getTileByIndx(child);
                    if (cTile) {
                        if (cTile->getImageIsNull()) {
                            allChildsHaveImage = false;
                            break;
                        }
                    }
                }

                if (allChildsHaveImage) {
                    lastChildsBeenAll = true;
                    parentsToRemove.append(parentIndx);
                }
                else {
                    if (!nearbyValidParentIndx.isValid()) {
                        nearbyValidParentIndx = parentIndx;
                        updateVertices(pTile);
                    }
                    else {
                        // проверить предуыущий следующий
                        auto prevParentChilds = parentIndx.getChilds().first;

                        if (prevParentChilds.empty()) {
                            parentsToRemove.append(nearbyValidParentIndx);
                            nearbyValidParentIndx = parentIndx;
                            updateVertices(pTile);
                        }
                        else {
                            int currNumOvlpTiles = calcNumOverlappingTiles(childs);
                            int prevNumOvlpTiles = calcNumOverlappingTiles(prevParentChilds);

                            if (currNumOvlpTiles > prevNumOvlpTiles) {
                                parentsToRemove.append(nearbyValidParentIndx);
                                nearbyValidParentIndx = parentIndx;
                                updateVertices(pTile);
                            }
                            else {
                                parentsToRemove.append(parentIndx);
                            }
                        }
                    }

                    canRenderTile = false;
                }
            }
            else {
                canRenderTile = true;
            }
        }

        currTileIndx = parentIndx;
    }

    for (const auto& parent : parentsToRemove) {
        auto* pTile = getTileByIndx(parent);
        if (pTile) {
            pTile->setInUse(false);
            emit mvDeleteTile(*pTile);
        }
    }

    if (canRenderTile) {

        auto parent = tileIndx.getParent().first;
        auto childs = parent.getChilds().first;

        for (const auto& child : childs) {
            if (!request_.contains(child)) {
                continue;
            }
            auto* cTile = getTileByIndx(child);
            if (cTile) {
                if (!cTile->getImageIsNull() && !cTile->getInUse()) {
                    updateVertices(cTile, false);
                    cTile->setInUse(true);
                    emit mvAppendTile(*cTile);
                }
            }
        }
    }
    else {
        //qDebug() << "tile" << tileIndx << "ne otrisuetsa 2222!!!";
    }
}
void TileSet::removeFarTilesFromRender(const QSet<TileIndex>& request)
{
    for (auto& lstTile : tileList_) {
        if (!lstTile.getInUse()) {
            continue;
        }

        auto lstTileIndx = lstTile.getIndex();

        int32_t diff = std::abs(lstTileIndx.z_ - currZoom_);

        if (diff > propagationLevel_) {
            lstTile.setInUse(false);
            emit mvDeleteTile(lstTile);
        }
        else {
            bool overLap = false;

            if (request.contains(lstTileIndx)) {
                overLap = true;
            }
            else {
                for (auto& reqTileIndx : request) {
                    if (tilesOverlap(reqTileIndx, lstTileIndx, propagationLevel_)) {
                        overLap = true;
                        break;
                    }
                }
            }

            if (!overLap) {
                lstTile.setInUse(false);
                emit mvDeleteTile(lstTile);
            }
        }
    }
}

void TileSet::removeFarDBRequests(const QSet<TileIndex>& request)
{
    auto dbReqCopy = dbReq_;

    for (auto dbTileIndx : dbReqCopy) {

        bool overLap = false;

        if (request.contains(dbTileIndx)) {
            overLap = true;
        }
        else {
            for (const auto& reqTileIndx : request) {
                if (tilesOverlap(dbTileIndx, reqTileIndx, propagationLevel_)) {
                    overLap = true;
                    break;
                }
            }
        }

        if (!overLap) {
            emit dbStopLoadingTile(dbTileIndx);
        }
    }
}

void TileSet::removeFarDownloaderRequests(const QSet<TileIndex>& request)
{
    auto dwReqCopy = dwReq_;
    for (auto dwTileIndx : dwReqCopy) {

        bool overLap = false;

        if (request.contains(dwTileIndx)) {
            overLap = true;
        }
        else {
            for (const auto& requestTileIndex : request) {
                if (tilesOverlap(dwTileIndx, requestTileIndex, propagationLevel_)) {
                    overLap = true;
                    break;
                }
            }
        }

        if (!overLap) {
            tileDownloader_.lock()->deleteRequest(dwTileIndx);
        }
    }
}

void TileSet::removeOverlappingTiles()
{
    QSet<TileIndex> tilesInRender;
    for (const auto& lTile : tileList_) {
        if (lTile.getInUse()) {
            tilesInRender.insert(lTile.getIndex());
        }
    }

    QList<TileIndex> finalTiles;
    for (const auto& tile : tilesInRender) {
        bool shouldAdd = true;

        for (auto it = finalTiles.begin(); it != finalTiles.end(); ) {


            if (tilesOverlap(tile, *it, -1)) {
                if (tile.z_ < it->z_) {
                    it = finalTiles.erase(it);
                }
                else {
                    shouldAdd = false;
                    break;
                }
            }
            else {
                ++it;
            }
        }

        if (shouldAdd) {
            finalTiles.append(tile);
        }
    }

    QSet<TileIndex> tilesToDelete = tilesInRender;

    for (const auto& tile : finalTiles) {
        tilesToDelete.remove(tile);
    }

    for (auto& itm : tilesToDelete) {

        auto* dTile = getTileByIndx(itm);
        if (dTile) {
            dTile->setInUse(false);
            emit mvDeleteTile(*dTile);
        }
    }
}

QImage TileSet::extractRegion(const QImage &image, int dimension, int x, int y)
{
    if (dimension <= 0 || (dimension & (dimension - 1)) != 0) {
        return QImage();
    }

    int cellWidth = image.width() / dimension;
    int cellHeight = image.height() / dimension;

    if (x < 0 || x >= dimension || y < 0 || y >= dimension) {
        return QImage();
    }

    QRect regionToCopy(x * cellWidth, y * cellHeight, cellWidth, cellHeight);

    if (!image.rect().contains(regionToCopy)) {
        return QImage();
    }

    return image.copy(regionToCopy);
}

void TileSet::updateTileVerticesInRender(const QSet<TileIndex>& request)
{
    for (auto& itm : request) {
        if (auto it = tileMap_.find(itm); it != tileMap_.end()) {
            if (it->second->getInUse()) {
                auto updatedInfo = tileProvider_.lock()->indexToTileInfo(it->second->getIndex(), getTilePosition(minLon_, maxLon_, it->second->getOriginTileInfo()));
                it->second->setModifiedTileInfo(updatedInfo);
                it->second->updateVertices(viewLlaRef_, isPerspective_);
                emit mvUpdateTileVertices(*(it->second));
            }
        }
    }
}

void TileSet::drawNumberOnImage(QImage &image, const TileIndex& tileIndx, const QColor &color) const
{
    if (image.isNull()) {
        return;
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QFont font;
    font.setPointSize(64);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(color);

    auto drawTextWithOutline = [&](const QRect& rect, const QString& text, int outlineWidth) {
        painter.setPen(QPen(Qt::red, outlineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx != 0 || dy != 0) {
                    QRect offsetRect = rect.translated(dx, dy);
                    painter.drawText(offsetRect, Qt::AlignCenter, text);
                }
            }
        }

        painter.setPen(color);
        painter.drawText(rect, Qt::AlignCenter, text);
    };

    QRect rectZ(0, 35, 256, 80);
    drawTextWithOutline(rectZ, QString::number(tileIndx.z_), 3);

    font.setPointSize(32);
    font.setBold(false);
    painter.setFont(font);

    QRect rectX(0, 125, 256, 40);
    drawTextWithOutline(rectX, "x:" + QString::number(tileIndx.x_), 2);

    QRect rectY(0, 175, 256, 40);
    drawTextWithOutline(rectY, "y:" + QString::number(tileIndx.y_), 2);

    QPen borderPen(Qt::red);
    borderPen.setWidth(4);
    painter.setPen(borderPen);
    painter.drawRect(0, 0, image.width() - 1, image.height() - 1);

    painter.end();
}

void TileSet::tryRenderTile(Tile &tile)
{
    if (tile.getImageIsNull()) {
        return;
    }

    auto tileIndx = tile.getIndex();
    if (!request_.contains(tileIndx)) {
        return;
    }

    // обновить вершины
    auto updatedInfo = tileProvider_.lock()->indexToTileInfo(tileIndx, getTilePosition(minLon_, maxLon_, tile.getOriginTileInfo()));
    tile.setModifiedTileInfo(updatedInfo);
    tile.updateVertices(viewLlaRef_, isPerspective_);

    if (tile.getInUse()) {
        emit mvUpdateTileVertices(tile);
        return;
    }

    switch (zoomState_) {
    case map::ZoomState::kIn: {
        processIn(tileIndx);
        break;
    }
    case map::ZoomState::kOut: {
        processOut(tileIndx);
        break;
    }
    case map::ZoomState::kUnchanged: {
        processUnchanged(tileIndx);
        break;
    }
    default: {
        break;
    }
    }
}

Tile* TileSet::getTileByIndx(const TileIndex &tileIndx)
{
    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        return &(*(it->second));
    }
    return nullptr;
}

bool TileSet::addTile(const TileIndex& tileIndx)
{
    bool retVal{ false };

    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second); // move to front
        it->second->setRequestLastTime(QDateTime::currentDateTimeUtc());
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

void TileSet::tryShrinkSetSize()
{
    if (tileMap_.size() > maxCapacity_) {
        qDebug() << "overhead detected, delete old tiles";

        while (tileMap_.size() > minCapacity_) { // remove from back (map and list)
            auto lastIt = std::prev(tileList_.end());
            const TileIndex& indexToRemove = lastIt->getIndex();

            if (lastIt->getInUse()) {
                emit mvDeleteTile(*lastIt);
            }

            tileMap_.erase(indexToRemove);
            tileList_.erase(lastIt);
        }
    }
}

void TileSet::onNewRequest(const QSet<TileIndex>& request, ZoomState zoomState, LLARef viewLlaRef,
    bool isPerspective, double minLat, double maxLat, double minLon, double maxLon)
{
    //if (zoomState == ZoomState::kUnchanged) {
    //    return;
    //}

    if (request.isEmpty()) {
        return;
    }

    viewLlaRef_    = viewLlaRef;
    isPerspective_ = isPerspective;

    minLat_        = minLat;
    maxLat_        = maxLat;
    minLon_        = minLon;
    maxLon_        = maxLon;

    request_       = request;
    zoomState_     = zoomState;
    diffLevels_    = std::abs(request.begin()->z_ - currZoom_);
    currZoom_      = request.begin()->z_;


    qDebug() << "ON NEW REQ" << currZoom_ << diffLevels_;

    // удалить отдаленные тайлы из работы TileDB, TileDownloader
    removeFarDBRequests(request);
    removeFarDownloaderRequests(request);

    // очистить очередь иинициализации текстур (добавится далее)
    emit mvClearAppendTasks();

    // добавление тайлов в tileSet
    addTiles(request);

    // в рендере удалить дальние и обновить вершины у всех оставшихся
    removeFarTilesFromRender(request);
    updateTileVerticesInRender(request);

    // формируем запрос на загрузку/закачку недостающих изображений
    QSet<TileIndex> filtDbReq;
    for (const auto& reqTileIndx : request) {
        if (auto it = tileMap_.find(reqTileIndx); it != tileMap_.end()) {
            Tile& tile = *(it->second);

            if (tile.getImageIsNull()) {
                if (!dbReq_.contains(reqTileIndx) && !dwReq_.contains(reqTileIndx) && !dbSvd_.contains(reqTileIndx)) {
                    filtDbReq.insert(reqTileIndx);
                }
            }
            else {
                //qDebug() << "Hot cache:" << tile.getIndex();
                tryRenderTile(tile);
            }
        }
    }
    // чекнуть размер буффера
    tryShrinkSetSize();
    // удалить возможные перекрывающиеся тайлы
    removeOverlappingTiles();

/*
    QVector<TileIndex> vec;
    for (auto& [tileIndx, tile] : tileMap_) {
        if (tile->getInUse() && currZoom_ != tileIndx.z_) {
            vec.append(tileIndx);
        }
    }
    if (!vec.empty()) {
        qDebug() << "tile->getInUse() && currZoom_ != tileIndx.z_, vec size:" << vec.size();
        qDebug() << vec;
    }
*/

    // запрос в TileDB
    if (!filtDbReq.empty()) {
        dbReq_.unite(filtDbReq);
        emit dbLoadTiles(filtDbReq);
    }
}

void TileSet::setTextureIdByTileIndx(const TileIndex &tileIndx, GLuint textureId)
{
    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        it->second->setTextureId(textureId);
    }
}


} // namespace map
