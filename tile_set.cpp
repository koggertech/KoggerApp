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
    minLon_(0.0),
    maxLon_(0.0),
    currZoom_(-1),
    diffLevels_(std::numeric_limits<int32_t>::max()),
    moveUp_(false),
    defaultSize_(256, 256),
    defaultImageFormat_(QImage::Format_RGB32)
{
    qRegisterMetaType<Tile>("Tile");
    qRegisterMetaType<QSet<TileIndex>>("QSet<TileIndex>");
}

void TileSet::onNewRequest(const QSet<TileIndex>& request, ZoomState zoomState, LLARef viewLlaRef,
                           bool isPerspective, double minLon, double maxLon, bool moveUp)
{
    if (request.isEmpty()) {
        return;
    }

    viewLlaRef_    = viewLlaRef;
    isPerspective_ = isPerspective;
    minLon_        = minLon;
    maxLon_        = maxLon;
    moveUp_        = moveUp;
    request_       = request;
    zoomState_     = zoomState;
    diffLevels_    = std::abs(request.begin()->z_ - currZoom_);
    currZoom_      = request.begin()->z_;

    //QString state = (zoomState_ == ZoomState::kIn ? "IN" : zoomState_ == ZoomState::kOut ? "OUT" : "---");
    //qDebug() << "ON NEW REQ" << currZoom_ << state << request.size();

    addTiles(request);
    removeFarDBRequests(request);
    removeFarDWRequests(request);
    emit mvClearAppendTasks();

    QSet<TileIndex> filtDbReq;
    for (const auto& reqTileIndx : request) {
        if (auto* rTile = getTileByIndx(reqTileIndx); rTile) {
            bool inLoading = dbReq_.contains(reqTileIndx) ||
                             dwReq_.contains(reqTileIndx) ||
                             dbSvd_.contains(reqTileIndx);

            if (!rTile->getHasValidImage() && !inLoading) {
                filtDbReq.insert(reqTileIndx);
                tryCopyImage(rTile);
            }

            updateVertices(rTile);

            if (rTile->getInUse()) {
                emit mvUpdateTileVertices(rTile->getIndex(), rTile->getVerticesRef());
            }
            else {
                tryRenderTile(*rTile);
            }
        }
    }

    removeFarTilesFromRender(request);
    tryReduceSetSize();

    if (!filtDbReq.empty()) {
        dbReq_.unite(filtDbReq);
        emit dbLoadTiles(filtDbReq);
    }
}

void TileSet::onNewLlaRef(LLARef viewLlaRef)
{
    viewLlaRef_ = viewLlaRef;

    for (auto& tile : tileList_) {
        if (tile.getInUse()) {
            if (updateVertices(&tile)) {
                emit mvUpdateTileVertices(tile.getIndex(), tile.getVerticesRef());
            }
        }
    }
}

void TileSet::setTextureIdByTileIndx(const TileIndex &tileIndx, GLuint textureId)
{
    if (auto* tile = getTileByIndx(tileIndx); tile) {
        tile->setTextureId(textureId);
    }
}

void TileSet::onTileLoaded(const TileIndex &tileIndx, const QImage &image)
{
    dbReq_.remove(tileIndx);

    if (image.isNull()) {
        qWarning() << "TileSet::onTileLoaded: loaded 'null' image";
        return;
    }

    if (auto it = tileHash_.find(tileIndx); it != tileHash_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second);
        Tile& tile = *(it->second);

        QImage rImage = image;
        //drawNumberOnImage(rImage, tileIndx);
        QTransform trans;
        trans.rotate(90);
        rImage = rImage.transformed(trans);
        tile.setImage(rImage);
        tile.setState(Tile::State::kReady);
        tile.setIsCopied(false);

        updateVertices(&tile);
        tryRenderTile(tile, true);
    }
}

void TileSet::onTileLoadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    dbReq_.remove(tileIndx);

    Q_UNUSED(errorString);

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

void TileSet::onTileDownloaded(const TileIndex &tileIndx, const QImage &image)
{
    dwReq_.remove(tileIndx);

    if (image.isNull()) {
        qWarning() << "TileSet::onTileDownloaded: downloaded 'null' image";
        return;
    }

    dbSvd_.insert(tileIndx);
    emit dbSaveTile(tileIndx, image);

    if (auto it = tileHash_.find(tileIndx); it != tileHash_.end()) {
        tileList_.splice(tileList_.begin(), tileList_, it->second);
        Tile& tile = *(it->second);

        QImage rImage = image;
        //drawNumberOnImage(rImage, tileIndx);
        QTransform trans;
        trans.rotate(90);
        rImage = rImage.transformed(trans);
        tile.setImage(rImage);
        tile.setState(Tile::State::kReady);
        tile.setIsCopied(false);

        updateVertices(&tile);
        tryRenderTile(tile, true);
    }
}

void TileSet::onTileDownloadFailed(const TileIndex &tileIndx, const QString &errorString)
{
    dwReq_.remove(tileIndx);

    Q_UNUSED(tileIndx);
    Q_UNUSED(errorString);

    //qWarning() << "Failed to download tile" << tileIndx << "from:" << tileProvider_.lock()->createURL(tileIndx) << "Error:" << errorString;
}

void TileSet::onTileDownloadStopped(const TileIndex &tileIndx)
{
    dwReq_.remove(tileIndx);
}

void TileSet::onDeletedFromAppend(const TileIndex &tileIndx)
{
    if (auto* tile = getTileByIndx(tileIndx); tile) {
        tile->setInUse(false);
    }
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

bool TileSet::addTile(const TileIndex& tileIndx)
{
    bool retVal{ false };

    if (auto it = tileHash_.find(tileIndx); it != tileHash_.end()) {
        it->second->setRequestLastTime(QDateTime::currentDateTimeUtc());
        tileList_.splice(tileList_.begin(), tileList_, it->second); // move to front
    }
    else {
        Tile newTile(tileIndx);

        if (auto sharedProvider = tileProvider_.lock(); sharedProvider) {
            TileInfo info = sharedProvider->indexToTileInfo(newTile.getIndex());
            newTile.setOriginTileInfo(info);
        }

        newTile.setRequestLastTime(QDateTime::currentDateTimeUtc());
        tileList_.push_front(std::move(newTile));
        tileHash_[tileIndx] = tileList_.begin();
        retVal = true;
    }

    return retVal;
}

void TileSet::tryReduceSetSize()
{
    if (tileHash_.size() > maxCapacity_) {
        while (tileHash_.size() > minCapacity_) { // remove from back (map and list)
            auto lastIt = std::prev(tileList_.end());
            const TileIndex& indexToRemove = lastIt->getIndex();

            if (lastIt->getInUse()) {
                emit mvDeleteTile(indexToRemove);
            }

            tileHash_.erase(indexToRemove);
            tileList_.erase(lastIt);
        }
    }
}

bool TileSet::processIn(Tile* tile) const
{
    QImage currImage;

    auto getCurrImageRef = [&]() -> QImage& {
        if (!currImage.isNull()) {
            return currImage;
        }
        currImage = QImage(defaultSize_, defaultImageFormat_);
        return currImage;
    };

    auto originTileIndx = tile->getIndex();
    int depth = std::min(std::max(1, diffLevels_), propagationLevel_);
    bool beenCopied = false;

    for (int currDepth = 1; currDepth <= depth; ++currDepth) {
        if (auto* pTile = getTileByIndx(originTileIndx.getParent(currDepth).first); pTile) {
            if (pTile->getImageIsNull()) {
                continue;
            }

            QImage& parentImage = pTile->getImageRef();

            int parentSize = 1 << currDepth;
            int tileX = originTileIndx.x_ % parentSize;
            int tileY = originTileIndx.y_ % parentSize;
            int newTileX = parentSize - 1 - tileY;
            int newTileY = tileX;
            int cellWidth = parentImage.width() / parentSize;
            int cellHeight = parentImage.height() / parentSize;

            QRect regionToCopy(newTileX * cellWidth, newTileY * cellHeight, cellWidth, cellHeight);
            if (!parentImage.rect().contains(regionToCopy)) {
                continue;
            }

            QPainter painter(&getCurrImageRef());
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            QRect targetRect(0, 0, currImage.width(), currImage.height());
            painter.drawImage(targetRect, parentImage, regionToCopy);
            painter.end();

            beenCopied = true;
            break;
        }
    }

    if (beenCopied) {
        tile->setImage(currImage);
        tile->setIsCopied(true);
        tile->setState(Tile::State::kReady);
    }

    return beenCopied;
}

bool TileSet::processOut(Tile* tile) const
{
    QImage currImage;

    auto getCurrImageRef = [&]() -> QImage& {
        if (!currImage.isNull()) {
            return currImage;
        }
        currImage = QImage(defaultSize_, defaultImageFormat_);
        return currImage;
    };

    auto originTileIndx = tile->getIndex();
    int depth = std::min(std::max(1, diffLevels_), propagationLevel_);
    bool beenCopied = false;

    for (int currDepth = 1; currDepth <= depth; ++currDepth) {
        auto childTilesResult = originTileIndx.getChilds(currDepth);
        if (!childTilesResult.second) {
            continue;
        }
        const auto& childIndices = childTilesResult.first;
        if (childIndices.empty()) {
            continue;
        }
        int numChildren = static_cast<int>(childIndices.size());
        int gridSize = static_cast<int>(std::sqrt(numChildren));
        if (gridSize <= 0) {
            continue;
        }

        int cellWidth = getCurrImageRef().width() / gridSize;
        int cellHeight = getCurrImageRef().height() / gridSize;

        bool allChildsHasAnImage = true;
        QList<Tile*> choosedChildPtrs;
        choosedChildPtrs.reserve(childIndices.size());

        for (const auto& childIndex : childIndices) {
            if (auto* childTile = getTileByIndx(childIndex); childTile) {
                if (childTile->getImageIsNull()) {
                    allChildsHasAnImage = false;
                    break;
                }
                choosedChildPtrs.append(childTile);
            }
            else {
                allChildsHasAnImage = false;
                break;
            }
        }

        if (!allChildsHasAnImage) {
            continue;
        }

        QPainter painter(&getCurrImageRef());
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        for (auto& childTile : choosedChildPtrs) {
            QImage& childImage = childTile->getImageRef();
            auto childIndx = childTile->getIndex();
            int tileX = childIndx.x_ % gridSize;
            int tileY = childIndx.y_ % gridSize;
            int newTileX = gridSize - 1 - tileY;
            int newTileY = tileX;

            QRect targetRect(newTileX * cellWidth, newTileY * cellHeight, cellWidth, cellHeight);

            if (!getCurrImageRef().rect().contains(targetRect)) {
                continue;
            }

            painter.drawImage(targetRect, childImage, childImage.rect());

            beenCopied = true;
        }

        painter.end();

        if (beenCopied) {
            break;
        }
    }

    if (beenCopied) {
        tile->setImage(currImage);
        tile->setIsCopied(true);
        tile->setState(Tile::State::kReady);
    }

    return beenCopied;
}

void TileSet::removeFarTilesFromRender(const QSet<TileIndex>& request)
{
    for (auto& lTile : tileList_) {
        if (!lTile.getInUse()) {
            continue;
        }

        auto lTileIndx = lTile.getIndex();
        if (!request.contains(lTileIndx)) {
            lTile.setInUse(false);
            emit mvDeleteTile(lTileIndx);
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

bool TileSet::tryRenderTile(Tile &tile, bool force)
{
    if (tile.getImageIsNull()) {
        return false;
    }

    auto tileIndx = tile.getIndex();
    if (!request_.contains(tileIndx)) {
        return false;
    }

    if (tile.getInUse() && !force) {
        return false;
    }
    else if (tile.getInUse() && force) {
        emit mvUpdateTileImage(tileIndx, tile.getImageRef());
    }
    else {
        tile.setInUse(true);
        emit mvAppendTile(tile);
    }

    return true;
}

Tile* TileSet::getTileByIndx(const TileIndex &tileIndx) const
{
    if (auto it = tileHash_.find(tileIndx); it != tileHash_.end()) {
        return &(*(it->second));
    }
    return nullptr;
}

void TileSet::removeFarDBRequests(const QSet<TileIndex>& request)
{
    auto dbReqCopy = dbReq_;

    for (auto dbTileIndx : dbReqCopy) {
        if (!request.contains(dbTileIndx)) {
            emit dbStopLoadingTile(dbTileIndx);
        }
    }
}

void TileSet::removeFarDWRequests(const QSet<TileIndex>& request)
{
    auto dwReqCopy = dwReq_;
    for (auto dwTileIndx : dwReqCopy) {
        if (!request.contains(dwTileIndx)) {
            tileDownloader_.lock()->deleteRequest(dwTileIndx);
        }
    }
}

bool TileSet::tryCopyImage(Tile* tile) const
{
    bool retVal = false;

    switch (zoomState_) {
    case map::ZoomState::kIn: {
        retVal = processIn(tile);
        break;
    }
    case map::ZoomState::kOut: {
        retVal = processOut(tile);
        break;
    }

    default:
        break;
    }

    return retVal;
}

bool TileSet::updateVertices(Tile* tile) const
{
    if (tile) {
        if (auto sharedProvider = tileProvider_.lock(); sharedProvider) {
            auto updatedInfo = sharedProvider->indexToTileInfo(tile->getIndex(), getTilePosition(minLon_, maxLon_, tile->getOriginTileInfo()));
            tile->setModifiedTileInfo(updatedInfo);
            tile->updateVertices(viewLlaRef_, isPerspective_);
            return true;
        }
    }
    return false;
}


} // namespace map
