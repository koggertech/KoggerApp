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

bool TileSet::addTiles(const QSet<TileIndex> &request)
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

        if (tile.getInUse() || !tile.getImageIsNull()) {
            return;
        }

        QImage tmpImage = image;
        //drawNumberOnImage(temp, tileIndx);
        QTransform trans;
        trans.rotate(90);
        tmpImage = tmpImage.transformed(trans);

        tile.setImage(tmpImage);
        tile.setOriginTileInfo(info);
        tile.setState(Tile::State::kReady);

        if (request_.contains(tileIndx)) {
            auto updatedInfo = tileProvider_.lock()->indexToTileInfo(tile.getIndex(), getTilePosition(minLon_, maxLon_, info));
            tile.setModifiedTileInfo(updatedInfo);
            tile.updateVertices(viewLlaRef_, isPerspective_);

            tile.setInUse(true);
            emit mvAppendTile(tile);

            removeOverlappingTilesFromRender(tile);
        }
    }
}

void TileSet::onTileLoadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    dbReq_.remove(tileIndx);

    Q_UNUSED(errorString);

    // try to download
    if (auto sharedDownloader = tileDownloader_.lock(); sharedDownloader) {
        dwReq_.insert(tileIndx);
        sharedDownloader->downloadTile(tileIndx);
    }
}

void TileSet::onTileLoadStopped(const TileIndex &tileIndx)
{
    dbReq_.remove(tileIndx);
}

void TileSet::onTileDownloaded(const TileIndex &tileIndx, const QImage &image, const TileInfo &info)
{
    //qDebug() << "Downloaded tile:" << tileIndx;

    dwReq_.remove(tileIndx);

    if (image.isNull()) {
        qWarning() << "TileSet::onTileDownloaded: downloaded 'null' image";
        return;
    }

    emit dbSaveTile(tileIndx, image);

    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second);
        Tile& tile = *(it->second);

        if (tile.getInUse() || !tile.getImageIsNull()) {
            return;
        }

        // image
        QImage temp = image;
        //drawNumberOnImage(temp, tileIndx);
        QTransform trans;
        trans.rotate(90);
        temp = temp.transformed(trans);

        tile.setImage(temp);
        tile.setOriginTileInfo(info);
        tile.setState(Tile::State::kReady);

        if (request_.contains(tileIndx)) {;
            auto updatedInfo = tileProvider_.lock()->indexToTileInfo(tile.getIndex(), getTilePosition(minLon_, maxLon_, info));
            tile.setModifiedTileInfo(updatedInfo);
            tile.updateVertices(viewLlaRef_, isPerspective_);

            tile.setInUse(true);
            emit mvAppendTile(tile);

            removeOverlappingTilesFromRender(tile);
        }
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

void TileSet::removeOverlappingTilesFromRender(const Tile &newTile)
{
    switch (zoomState_) {
    case map::ZoomState::kOut: {
        processOut(newTile.getIndex());
        break;
    }
    case map::ZoomState::kIn: {
        processIn(newTile.getIndex());
        break;
    }
    case map::ZoomState::kUnchanged: {
        processIn(newTile.getIndex());
        processOut(newTile.getIndex());
        break;
    }
    default:
        break;
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
/*
    auto absDiffLevels = std::abs(diffLevels_);
    auto parent = tileIndex;


    for (int i = 0; i < absDiffLevels; ++i) {

        parent = parent.getParent();
        qDebug() << i << parent;

        bool allChildsAreInited{ true };

        if (!i) {
            auto childs = parent.getChildren();

            for (auto& itm : childs) {
                if (request_.contains(itm)) {
                    if (auto it = tileMap_.find(itm); it != tileMap_.end()) {
                        if (it->second->getImageIsNull()) {
                            allChildsAreInited = false;
                            break;
                        }
                    }
                }
            }
        }

        if (allChildsAreInited || i) {
            if (auto it = tileMap_.find(parent); it != tileMap_.end()) {
                //if (it->second->getInUse()) {
                    it->second->setInUse(false);
                    emit mvDeleteTile(*(it->second));
                //}
            }
        }
    }
*/


    // first level
    bool allChildsAreInited{ true };

    auto parent = tileIndex.getParent();
    auto childs = parent.getChildren();

    for (auto& itm : childs) {
        if (request_.contains(itm)) {
            if (auto it = tileMap_.find(itm); it != tileMap_.end()) {
                if (it->second->getImageIsNull()) {
                    allChildsAreInited = false;
                    break;
                }
            }
        }
    }

    if (allChildsAreInited) {
        if (auto it = tileMap_.find(parent); it != tileMap_.end()) {
            it->second->setInUse(false);
            emit mvDeleteTile(*(it->second));
        }

        //auto absDiffLevels = std::abs(diffLevels_);
        //if (absDiffLevels > 1) {
        //    TileIndex currTileIndx = parent;
        //    for (int i = 0; i < (absDiffLevels - 1); ++i) {
        //
        //    }
        //
        //
        //
        //}


    }



}

void TileSet::processOut(const TileIndex &tileIndex)
{
    auto childs = tileIndex.getChildren();

    for (auto& itm : childs) {
        if (auto it = tileMap_.find(itm); it != tileMap_.end()) {
            it->second->setInUse(false);
            emit mvDeleteTile(*(it->second));
        }

        // sub layer
        auto subChilds = itm.getChildren();
        for (auto& itmm : subChilds) {
            if (auto itt = tileMap_.find(itmm); itt != tileMap_.end()) {
                itt->second->setInUse(false);
                emit mvDeleteTile(*(itt->second));
            }
        }
    }
}

void TileSet::removeFarTilesFromRender(const QSet<TileIndex>& request)
{
    for (auto& itmI : tileList_) {
        if (!itmI.getInUse()) {
            continue;
        }

        int32_t diff = std::abs(itmI.getIndex().z_ - currZoom_);

        if (diff > 15) {
            itmI.setInUse(false);
            emit mvDeleteTile(itmI);
        }
        else { // overlapping checking
            bool overLap = false;
            for (auto& itmJ : request) {
                if (tilesOverlap(itmJ,  itmI.getIndex(), propagationLevel_)) {
                    overLap = true;
                    break;
                }
            }

            if (!overLap) {
                itmI.setInUse(false);
                //qDebug() << "removed far" << itmI.getIndex();
                emit mvDeleteTile(itmI);
            }
        }
    }

}

void TileSet::removeFarDBRequests(const QSet<TileIndex> &request)
{
    auto dbReqCopy = dbReq_;

    for (auto it = dbReqCopy.begin(); it != dbReqCopy.end(); ++it) {
        bool overLap = false;

        for (const auto& itmJ : request) {
            if (tilesOverlap(*it, itmJ, propagationLevel_)) {
                overLap = true;
                break;
            }
        }

        if (!overLap) {
            emit dbStopLoadingTile(*it);
        }
    }
}

void TileSet::removeFarDownloaderRequests(const QSet<TileIndex>& request)
{
    auto dwReqCopy = dwReq_;

    for (auto it = dwReqCopy.begin(); it != dwReqCopy.end(); ++it) {
        bool overLap = false;

        for (const auto& itmJ : request) {
            if (tilesOverlap(*it, itmJ, propagationLevel_)) {
                overLap = true;
                break;
            }
        }

        if (!overLap) {
            tileDownloader_.lock()->deleteRequest(*it);
        }
    }
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

bool TileSet::addTile(const TileIndex& tileIndx)
{
    bool retVal{ false };

    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second); // move to front
        it->second->setRequestLastTime(QDateTime::currentDateTimeUtc());
        retVal = false;
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



                //if (mapView_) {
                //    mapView_->onTileAppend(*lastIt);
                //}
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
    if (request.isEmpty()) {
        return;
    }

    viewLlaRef_     = viewLlaRef;
    isPerspective_  = isPerspective;

    minLat_         = minLat;
    maxLat_         = maxLat;
    minLon_         = minLon;
    maxLon_         = maxLon;

    request_        = request;
    zoomState_      = zoomState;
    diffLevels_     = request.begin()->z_ - currZoom_;
    currZoom_       = request.begin()->z_;


    //qDebug() << "ON NEW REQ" << currZoom_ << diffLevels_;

    // удалить тайлы из работы TileDB, TileDownloader
    removeFarDBRequests(request);
    removeFarDownloaderRequests(request);

    // очистить очередь иинициализации текстур
    emit mvClearAppendTasks();

    // добавление тайлов в tileSet
    addTiles(request);

    //// удалить неиспользуемые
    //for (auto& [index, tile] : tileMap_) {
    //        if (tile->getInUse()) { // был в использовании но сейчас нет
    //            tile->setInUse(false);
    //            //tile->setPendingRemoval(true);
    //            emit deleteSignal(*tile);
    //        }
    //}

    // формируем запрос на загрузку/закачку недостающих изображений, все тайлы были созданы до
    QSet<TileIndex> filtReqSec;
    for (const auto& itm : request) {

        if (auto it = tileMap_.find(itm); it != tileMap_.end()) {

            Tile& tile = *(it->second);

            if (tile.getImageIsNull()) { // для тайлов с 'null' image ДОБАВЛЯЕМ в запрос, в OpenGL позже проинитится
                filtReqSec.insert(itm);
            }
            else {

                // обновить вершины у ЗАПРОШЕННЫХ тайлов
                //qDebug() << "HOT cache tile" << tile->first;
                auto updatedInfo = tileProvider_.lock()->indexToTileInfo(tile.getIndex(), getTilePosition(minLon_, maxLon_, tile.getOriginTileInfo()));
                tile.setModifiedTileInfo(updatedInfo);
                tile.updateVertices(viewLlaRef_, isPerspective_);

                //() << "upd:" << tile->second->getVerticesRef();



                //if (mapView_) {
                //    mapView_->onTileVerticesUpdated(tile->first, tile->second->getVerticesRef());
                //}
                emit mvUpdateTileVertices(tile);



                // если был не в использовании или текстура не проиничена
                if (!tile.getInUse() || !tile.getTextureId()) {
                    tile.setInUse(true);
                    emit mvAppendTile(tile);
                }

                removeOverlappingTilesFromRender(tile);
            }
        }
    }

    dbReq_.unite(filtReqSec);

    // обрезать размер буффера
    removeFarTilesFromRender(request);
    updateTileVerticesInRender(request);

    tryShrinkSetSize();

    //QVector<TileIndex> vec;
    //for (auto& [tileIndx, tile] : tileMap_) {
    //    if (tile->getInUse() && currZoom_ != tileIndx.z_) {
    //        vec.append(tileIndx);
    //    }
    //}
    //qDebug() << "tile->getInUse() && currZoom_ != tileIndx.z_ VECTOR:";
    //qDebug() << vec;

    //qDebug() << dbReq_;
    //qDebug() << "CDBS aft:" << currDBRequests_.size();
    //qDebug() << "filtReqSec";
    //qDebug() << filtReqSec;


    // запрос в DB
    if (!filtReqSec.empty()) {
        //qDebug() << "requested" << filtReqSec;
        //qDebug() << filtReqSec;
        emit dbLoadTiles(filtReqSec);
    }
}

void TileSet::setTextureIdByTileIndx(const TileIndex &tileIndx, GLuint textureId)
{
    if (auto it = tileMap_.find(tileIndx); it != tileMap_.end()) {
        it->second->setTextureId(textureId);
    }
}


} // namespace map
