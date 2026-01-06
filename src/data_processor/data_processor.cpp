#include "data_processor.h"

#include <cmath>
#include <QtConcurrent/QtConcurrentRun>
#include <QMetaObject>
#include <QDebug>
#include <QtGlobal>
#include <cmath>
#include "compute_worker.h"
#include "dataset.h"
#include "data_processor_defs.h"


inline void markTilesBySource(TileMap& tiles, DataSource src)
{
    UpdateHint hint = UpdateHint::kUndefined;
    if (src == DataSource::kCalculation) {
        hint = UpdateHint::kAddOrUpdateTexture; // гарантированно обновить/создать текстуру
    } 
    else {
        hint = UpdateHint::kUndefined;          // кэш/БД: текстуру не трогаем
    }

    //const bool updated = (hint != UpdateHint::kUndefined);
    for (auto it = tiles.begin(); it != tiles.end(); ++it) {
        it.value().setUpdateHint(hint);
        //it.value().setIsUpdated(updated); // на всякий случай держим синхрон
    }
}

// static TileDiff diffTiles(const QSet<TileKey>& prev, const QSet<TileKey>& curr)
// {
//     TileDiff d;

//     d.stayed = prev;
//     d.stayed.intersect(curr);

//     d.added = curr;
//     d.added.subtract(prev);

//     d.removed = prev;
//     d.removed.subtract(curr);

//     return d;
// }

static inline uint32_t toMask(WorkSet s){ return static_cast<uint32_t>(s); }

DataProcessor::DataProcessor(QObject *parent, Dataset* datasetPtr)
    : QObject(parent),
    datasetPtr_(datasetPtr),
    worker_(nullptr),
    state_(DataProcessorType::kUndefined),
    chartsCounter_(0),
    bottomTrackCounter_(0),
    epochCounter_(0),
    positionCounter_(0),
    attitudeCounter_(0),
    updateBottomTrack_(false),
    updateSurface_(false),
    updateIsobaths_(false),
    updateMosaic_(false),
    isOpeningFile_(false),
    bottomTrackWindowCounter_(0),
    mosaicCounter_(0),
    tileResolution_(defaultTileResolution),
    pendingIsobathsWork_(false),
    cancelRequested_(false),
    jobRunning_(false),
    nextRunPending_(false),
    requestedMask_(0),
    btBusy_(false),
    hotCache_(hotCacheMaxSize_, hotCacheMinSize_),
    dbReader_(nullptr),
    dbReaderInWork_(false),
    dbWriter_(nullptr),
    engineVer_(1),
    lastZoom_(0),
    surfaceZoomChangedPending_(false),
    dbIsReady_(false),
    prefetchTick_(0),
    lastDataZoomIndx_(0)
{
    qRegisterMetaType<WorkBundle>("WorkBundle");
    qRegisterMetaType<DatasetChannel>("DatasetChannel");
    qRegisterMetaType<ChannelId>("ChannelId");
    qRegisterMetaType<BottomTrackParam>("BottomTrackParam");
    qRegisterMetaType<QVector<IsobathUtils::LabelParameters>>("QVector<IsobathUtils::LabelParameters>");
    qRegisterMetaType<TileMap>("TileMap");
    qRegisterMetaType<Dataset*>("Dataset*");
    qRegisterMetaType<std::uint8_t>("std::uint8_t");
    qRegisterMetaType<QSet<TileKey>>("QSet<TileKey>");

    hotCache_.setDataProcessorPtr(this);

    pendingWorkTimer_.setParent(this);
    pendingWorkTimer_.setSingleShot(true);
    pendingWorkTimer_.setInterval(333);
    connect(&pendingWorkTimer_, &QTimer::timeout, this, &DataProcessor::runCoalescedWork);

    worker_ = new ComputeWorker(this, datasetPtr_);
    worker_->setDatasetPtr(datasetPtr_);

    worker_->moveToThread(&computeThread_);

    connect(worker_, &ComputeWorker::jobFinished,          this, &DataProcessor::onWorkerFinished,      Qt::QueuedConnection);
    connect(worker_, &ComputeWorker::bottomTrackStarted,   this, &DataProcessor::onBottomTrackStarted,  Qt::QueuedConnection);
    connect(worker_, &ComputeWorker::bottomTrackFinished,  this, &DataProcessor::onBottomTrackFinished, Qt::QueuedConnection);

    computeThread_.setObjectName("ComputeWorkerThread");
    computeThread_.start();
}

DataProcessor::~DataProcessor()
{
    // shutdown(); // from about to quit
}

void DataProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;

    QMetaObject::invokeMethod(worker_, "setDatasetPtr", Qt::QueuedConnection, Q_ARG(Dataset*, datasetPtr_));
}

void DataProcessor::setBottomTrackPtr(BottomTrack *bottomTrackPtr)
{
    QMetaObject::invokeMethod(worker_, "setBottomTrackPtr", Qt::QueuedConnection, Q_ARG(BottomTrack*, bottomTrackPtr));
}

void DataProcessor::clearProcessing(DataProcessorType procType)
{
    // TODO

    requestCancel();

    switch (procType) {
    case DataProcessorType::kUndefined:   clearAllProcessings();        emit allProcessingCleared();         break;
    case DataProcessorType::kBottomTrack: clearBottomTrackProcessing(); emit bottomTrackProcessingCleared(); break;
    case DataProcessorType::kIsobaths:    clearIsobathsProcessing();    emit isobathsProcessingCleared();    break;
    case DataProcessorType::kMosaic:      clearMosaicProcessing();      emit mosaicProcessingCleared();      break;
    case DataProcessorType::kSurface:     clearSurfaceProcessing();     emit surfaceProcessingCleared();     break;
    default: break;
    }

    // prefetch db
    clearDbNotFoundCache();
    prefetchTick_ = 0;

    chartsCounter_ = 0;
    bottomTrackCounter_ = 0;
    epochCounter_ = 0;
    positionCounter_ = 0;
    attitudeCounter_ = 0;
    mosaicCounter_ = 0;

    tileEpochIndxsByZoom_.clear();
    tileEpochIndxsByZoom_.shrink_to_fit();
    surfaceTaskEpochIndxsByZoom_.clear();
    surfaceManualEpochIndxsByZoom_.clear();

    postState(DataProcessorType::kUndefined); //
}

void DataProcessor::setUpdateBottomTrack(bool state)
{
    updateBottomTrack_ = state;

    if ((updateBottomTrack_ || updateSurface_ || updateIsobaths_ || updateMosaic_) && !pendingSurfaceIndxs_.empty()) {
        scheduleLatest(WorkSet(WF_Surface));
    }
}

void DataProcessor::setUpdateSurface(bool state)
{
    const bool wasSurface = updateSurface_;
    updateSurface_ = state;

    if (updateSurface_ && !wasSurface) {
        int zoom = (lastDataZoomIndx_ > 0) ? lastDataZoomIndx_ : lastZoom_;
        if (zoom <= 0) {
            zoom = zoomFromMpp(tileResolution_);
        }

        enqueueSurfaceMissingForZoom(zoom);
        onCameraMovedSec();
    }
}

void DataProcessor::setUpdateIsobaths(bool state)
{
    updateIsobaths_ = state;

    if (updateIsobaths_) {
        scheduleLatest(WorkSet(WF_Isobaths));
    }
}

void DataProcessor::setUpdateMosaic(bool state)
{
    updateMosaic_ = state;

    //return;

    if (updateMosaic_) {
        if (hotCache_.checkAnyTileForZoom(lastZoom_)) {
            pumpVisible();
        }
        else {
            if (dbIsReady_) {
                emit dbCheckAnyTileForZoom(lastZoom_);
            }
        }
    }
    else {
        pendingIsobathsWork_ = true; // мозаика могла изменить поверхность
        scheduleLatest(WorkSet(WF_Isobaths));
    }
}

void DataProcessor::setIsOpeningFile(bool state)
{
    isOpeningFile_ = state;
}

void DataProcessor::onCameraMovedSec()
{
    if (!updateMosaic_ && !updateSurface_) {
        return;
    }

    auto epTiles =  collectEpochsForTiles(lastDataZoomIndx_, lastVisTileKeys_);

    for (auto it = epTiles.cbegin(); it != epTiles.cend(); ++it) {
        auto itm = (*it).first;
        pendingSurfaceIndxs_.insert(qMakePair('0', itm));
    }

    //qDebug() << "add pending" << epIndxs.size();

    for (auto it = epTiles.cbegin(); it != epTiles.cend(); ++it) {
        pendingMosaicIndxs_.insert((*it).first);
    }

    scheduleLatest(WorkSet(WF_All)); // all?
}

void DataProcessor::onCameraMoved(const QVector<QPair<int, QSet<TileKey>>> &epTiles)
{
    //qDebug() << "DataProcessor::onCameraMoved" << epTiles;

    if (!updateMosaic_ || epTiles.isEmpty()) {
        return;
    }

    //qDebug() << "add pending" << epIndxs.size();

    for (const auto& itm : epTiles) {
        pendingMosaicIndxs_.insert(itm.first);
    }

    scheduleLatest(WorkSet(WF_All)); // all?
}

void DataProcessor::onSendVisibleTileKeys(int zoomIndx, const QSet<TileKey> &visTileKeys)
{
    QMetaObject::invokeMethod(worker_, "setVisibleTileKeys", Qt::QueuedConnection, Q_ARG(QSet<TileKey>, visTileKeys));

    //mosaicCounter_; // head?

    const int prevZoom = lastDataZoomIndx_;
    lastDataZoomIndx_ = zoomIndx;
    lastVisTileKeys_  = visTileKeys;

    if (updateSurface_ && zoomIndx != prevZoom) {
        enqueueSurfaceMissingForZoom(zoomIndx);
    }

    onCameraMovedSec();
}

void DataProcessor::onChartsAdded(uint64_t indx)
{
    chartsCounter_ = indx;

#ifndef SEPARATE_READING
    if (isOpeningFile_) {
        return;
    }
#endif
    
    if (updateMosaic_ || updateIsobaths_ || updateBottomTrack_) {
        auto btP = datasetPtr_->getBottomTrackParam();

        const int endIndx    = static_cast<int>(indx);
        const int windowSize = btP.windowSize;

        int currCount = std::floor(endIndx / windowSize);

        if (bottomTrackWindowCounter_ != currCount) {
            auto additionalBTPGap = windowSize / 2;
            btP.indexFrom = std::max(0, windowSize * bottomTrackWindowCounter_ - (windowSize / 2 + 1) - additionalBTPGap);
            btP.indexTo   = std::max(0, windowSize * currCount - (windowSize / 2 + 1) - additionalBTPGap);

            const auto channels = datasetPtr_->channelsList();
            if (!channels.isEmpty()) {
                const DatasetChannel ch1 = channels[0];
                const DatasetChannel ch2 = (channels.size() >= 2) ? channels[1] : DatasetChannel();
                QMetaObject::invokeMethod(worker_, "bottomTrackProcessing", Qt::QueuedConnection,
                                          Q_ARG(DatasetChannel, ch1),
                                          Q_ARG(DatasetChannel, ch2),
                                          Q_ARG(BottomTrackParam, btP),
                                          Q_ARG(bool, false),/*manual*/
                                          Q_ARG(bool, false)/*redrawAll*/);
            }
            bottomTrackWindowCounter_ = currCount;
        }
    }
}

void DataProcessor::onBottomTrack3DAdded(const QVector<int>& epIndxs, const QVector<int>& vertIndxs, bool isManual)
{
    //return;

    if (epIndxs.isEmpty() || vertIndxs.isEmpty()) {
        return;
    }

    for (int itm : epIndxs) {
        epIndxsFromBottomTrack_.insert(itm);
        pendingMosaicIndxs_.insert(itm);
    }

    for (int itm : vertIndxs) {
        vertIndxsFromBottomTrack_.insert(itm);
        pendingSurfaceIndxs_.insert(qMakePair(isManual ? '1' : '0', itm));
    }

    scheduleLatest(WorkSet(WF_All));
}

void DataProcessor::onEpochAdded(uint64_t indx)
{
    epochCounter_    = indx;
}

void DataProcessor::onPositionAdded(uint64_t indx)
{
    positionCounter_ = indx;
}

void DataProcessor::onAttitudeAdded(uint64_t indx)
{
    attitudeCounter_ = indx;
}

void DataProcessor::onMosaicCanCalc(uint64_t indx)
{
    mosaicCounter_   = indx;
}

void DataProcessor::bottomTrackProcessing(const DatasetChannel &ch1, const DatasetChannel &ch2, const BottomTrackParam &p, bool manual, bool redrawAll)
{
    if (btBusy_) {
        //qDebug() << "bt skip - busy";
        return;
    }

    QMetaObject::invokeMethod(worker_, "bottomTrackProcessing", Qt::QueuedConnection,
                              Q_ARG(DatasetChannel, ch1),
                              Q_ARG(DatasetChannel, ch2),
                              Q_ARG(BottomTrackParam, p),
                              Q_ARG(bool, manual),
                              Q_ARG(bool, redrawAll));
}

void DataProcessor::setSurfaceColorTableThemeById(int id)
{
    QMetaObject::invokeMethod(worker_, "setSurfaceThemeId", Qt::QueuedConnection, Q_ARG(int, id));
}

void DataProcessor::setSurfaceEdgeLimit(int val)
{
    QMetaObject::invokeMethod(worker_, "setSurfaceEdgeLimit", Qt::QueuedConnection, Q_ARG(float, float(val)));

    surfaceTaskEpochIndxsByZoom_.clear();
    surfaceManualEpochIndxsByZoom_.clear();
    pendingSurfaceIndxs_.clear();
    pendingIsobathsWork_ = true;

    for (auto it = vertIndxsFromBottomTrack_.cbegin(); it != vertIndxsFromBottomTrack_.cend(); ++it) {
        pendingSurfaceIndxs_.insert(qMakePair('0', *it));
    }

    scheduleLatest(WorkSet(WF_All), /*replace*/true);
}

void DataProcessor::setExtraWidth(int val)
{
    QMetaObject::invokeMethod(worker_, "setSurfaceExtraWidth", Qt::QueuedConnection, Q_ARG(int, val));

    // REBUILD?
    //surfaceTaskEpochIndxsByZoom_.clear();
    //surfaceManualEpochIndxsByZoom_.clear();
    //pendingSurfaceIndxs_.clear();
    //pendingIsobathsWork_ = true;

    //for (auto it = vertIndxsFromBottomTrack_.cbegin(); it != vertIndxsFromBottomTrack_.cend(); ++it) {
    //    pendingSurfaceIndxs_.insert(qMakePair('0', *it));
    //}

    //scheduleLatest(WorkSet(WF_All), /*replace*/true);
}

void DataProcessor::setIsobathsLabelStepSize(float val)
{
    QMetaObject::invokeMethod(worker_, "setIsobathsLabelStepSize", Qt::QueuedConnection, Q_ARG(float, val));
}

void DataProcessor::setSurfaceIsobathsStepSize(float val)
{
    QMetaObject::invokeMethod(worker_, "setSurfaceIsobathsStepSize", Qt::QueuedConnection, Q_ARG(float, val));

    surfaceTaskEpochIndxsByZoom_.clear();
    surfaceManualEpochIndxsByZoom_.clear();
    pendingSurfaceIndxs_.clear();
    pendingIsobathsWork_ = true;

    for (auto it = vertIndxsFromBottomTrack_.cbegin(); it != vertIndxsFromBottomTrack_.cend(); ++it) {
        pendingSurfaceIndxs_.insert(qMakePair('0', *it));
    }

    scheduleLatest(WorkSet(WF_All), /*replace*/true);
}

void DataProcessor::setMosaicChannels(const ChannelId &ch1, uint8_t sub1, const ChannelId &ch2, uint8_t sub2)
{
    hotCache_.clear();

    QMetaObject::invokeMethod(worker_, "clearAll", Qt::QueuedConnection);

    surfaceTaskEpochIndxsByZoom_.clear();
    surfaceManualEpochIndxsByZoom_.clear();

    emit isobathsProcessingCleared();
    emit surfaceProcessingCleared();
    emit mosaicProcessingCleared();

    QMetaObject::invokeMethod(worker_, "setMosaicChannels", Qt::QueuedConnection,
                              Q_ARG(ChannelId, ch1), Q_ARG(uint8_t, sub1),
                              Q_ARG(ChannelId, ch2), Q_ARG(uint8_t, sub2));

    //return;

    for (auto it = epIndxsFromBottomTrack_.cbegin(); it != epIndxsFromBottomTrack_.cend(); ++it) {
        pendingMosaicIndxs_.insert(*it);
    }
    for (auto it = vertIndxsFromBottomTrack_.cbegin(); it != vertIndxsFromBottomTrack_.cend(); ++it) {
        pendingSurfaceIndxs_.insert(qMakePair('0', *it));
    }

    pendingIsobathsWork_ = true;

    scheduleLatest(WorkSet(WF_All), /*replace*/true);
}

void DataProcessor::setMosaicTheme(int indx)
{
    QMetaObject::invokeMethod(worker_, "setMosaicTheme", Qt::QueuedConnection, Q_ARG(int, indx));
}

void DataProcessor::setMosaicLAngleOffset(float val)
{
    QMetaObject::invokeMethod(worker_, "setMosaicLAngleOffset", Qt::QueuedConnection, Q_ARG(float, val));
}

void DataProcessor::setMosaicRAngleOffset(float val)
{
    QMetaObject::invokeMethod(worker_, "setMosaicRAngleOffset", Qt::QueuedConnection, Q_ARG(float, val));
}

void DataProcessor::setMosaicTileResolution(float val)
{
    return;

    //qDebug() << "DataProcessor::setMosaicTileResolution" << val;
    if (qFuzzyIsNull(val)) {
        return;
    }

    const float convertedResolution = 1.0f / val;
    if (qFuzzyCompare(1.0f + tileResolution_, 1.0f + convertedResolution)) {
        return;
    }

    hotCache_.clear(); // db?

    tileResolution_ = convertedResolution;

    emit isobathsProcessingCleared();
    emit surfaceProcessingCleared();
    emit mosaicProcessingCleared();

    for (auto it = epIndxsFromBottomTrack_.cbegin(); it != epIndxsFromBottomTrack_.cend(); ++it) {
        pendingMosaicIndxs_.insert(*it);
    }
    for (auto it = vertIndxsFromBottomTrack_.cbegin(); it != vertIndxsFromBottomTrack_.cend(); ++it) {
        pendingSurfaceIndxs_.insert(qMakePair('0', *it));
    }

    pendingIsobathsWork_ = true;

    QMetaObject::invokeMethod(worker_, "setMosaicTileResolution", Qt::QueuedConnection, Q_ARG(float, tileResolution_));
    scheduleLatest(WorkSet(WF_All), /*replace*/true);
}

void DataProcessor::setMosaicLevels(float lowLevel, float highLevel)
{
    QMetaObject::invokeMethod(worker_, "setMosaicLevels", Qt::QueuedConnection, Q_ARG(float, lowLevel), Q_ARG(float, highLevel));
}

void DataProcessor::setMosaicLowLevel(float val)
{
    QMetaObject::invokeMethod(worker_, "setMosaicLowLevel", Qt::QueuedConnection, Q_ARG(float, val));
}

void DataProcessor::setMosaicHighLevel(float val)
{
    QMetaObject::invokeMethod(worker_, "setMosaicHighLevel", Qt::QueuedConnection, Q_ARG(float, val));
}

void DataProcessor::askColorTableForMosaic()
{
    QMetaObject::invokeMethod(worker_, "askColorTableForMosaic", Qt::QueuedConnection);
}

void DataProcessor::onIsobathsUpdated()
{
    if (!updateIsobaths_) {
        return;
    }

    pendingIsobathsWork_ = true;

    scheduleLatest(WorkSet(WF_Isobaths));
}

void DataProcessor::onMosaicUpdated() {
    if (!updateMosaic_ || pendingMosaicIndxs_.isEmpty()) {
        return;
    }

    scheduleLatest(WorkSet(WF_Mosaic));
}

void DataProcessor::runCoalescedWork()
{
    //qDebug() << "DataProcessor::runCoalescedWork";

    if (jobRunning_.load()) {
        return;
    }

    if (!isCanStartCalculations()) {
        return;
    }

    const uint32_t maskNow = requestedMask_.exchange(0);
    const bool wantSurface  = maskNow & WF_Surface;
    const bool wantMosaic   = maskNow & WF_Mosaic;
    const bool wantIsobaths = maskNow & WF_Isobaths;

    WorkBundle wb;

    if (wantSurface && (updateSurface_ /*|| updateMosaic_*/)) { // TODO: or     if (wantSurface && !pendingSurfaceIndxs_.isEmpty() && (/*updateBottomTrack_ || */updateIsobaths_ || updateMosaic_)) { ???
        int zoom = (lastDataZoomIndx_ > 0) ? lastDataZoomIndx_ : lastZoom_;
        if (zoom <= 0) {
            zoom = zoomFromMpp(tileResolution_);
        }

        if (!pendingSurfaceIndxs_.isEmpty()) {
            for (auto it = pendingSurfaceIndxs_.cbegin(); it != pendingSurfaceIndxs_.cend(); ++it) {
                if (it->first != '1') {
                    continue;
                }
                for (const auto& z : ZL) {
                    surfaceManualEpochIndxsByZoom_[z.zoom].insert(it->second);
                }
            }
        }

        auto& processed = surfaceTaskEpochIndxsByZoom_[zoom];
        auto& manualPending = surfaceManualEpochIndxsByZoom_[zoom];

        if (!pendingSurfaceIndxs_.isEmpty() || !manualPending.isEmpty()) {
            wb.surfaceVec.reserve(pendingSurfaceIndxs_.size() + manualPending.size());

            if (!manualPending.isEmpty()) {
                for (int idx : std::as_const(manualPending)) {
                    processed.insert(idx);
                    wb.surfaceVec.append(qMakePair('1', idx));
                }
                manualPending.clear();
            }

            if (!pendingSurfaceIndxs_.isEmpty()) {
                for (auto it = pendingSurfaceIndxs_.cbegin(); it != pendingSurfaceIndxs_.cend(); ++it) {
                    if (it->first == '1') {
                        continue;
                    }
                    if (processed.contains(it->second)) {
                        continue;
                    }
                    processed.insert(it->second);
                    wb.surfaceVec.append(*it);
                }

                pendingSurfaceIndxs_.clear();
            }

            if (!wb.surfaceVec.isEmpty()) {
                std::sort(wb.surfaceVec.begin(), wb.surfaceVec.end(), [](const QPair<char,int>& a, const QPair<char,int>& b){ return a.second < b.second; });
            }
        }
    }

    if (wantMosaic && !pendingMosaicIndxs_.isEmpty() && updateMosaic_) {
        auto it = pendingMosaicIndxs_.begin();
        while (it != pendingMosaicIndxs_.end()) {
            const int idx = *it;

            if (idx <= mosaicCounter_) {
                wb.mosaicVec.append(idx);
                it = pendingMosaicIndxs_.erase(it);
            }
            else {
                ++it;
            }
        }

        if (!wb.mosaicVec.isEmpty()) {
            std::sort(wb.mosaicVec.begin(), wb.mosaicVec.end());
        }
    }

    if (wantIsobaths && updateIsobaths_ && !updateMosaic_) {
        wb.doIsobaths = pendingIsobathsWork_ || !wb.surfaceVec.isEmpty();
        if (pendingIsobathsWork_) {
            pendingIsobathsWork_ = false;
        }
    }

    if (wb.surfaceVec.isEmpty() && wb.mosaicVec.isEmpty() && !wb.doIsobaths) {
        pendingIsobathsWork_ = false;
        return;
    }

    if (updateMosaic_) {
        wb.doIsobaths = false;
        pendingIsobathsWork_ = false;
    }

    //wb.doIsobaths = false;
    //pendingIsobathsWork_ = false;

    nextRunPending_.store(false);
    cancelRequested_.store(false);
    jobRunning_.store(true);

    // queue в воркер в его поток
    QMetaObject::invokeMethod(worker_, "processBundle", Qt::QueuedConnection, Q_ARG(WorkBundle, wb));
}

void DataProcessor::startTimerIfNeeded()
{
    if (defProcType_) {
        auto startOrRestart = [this]() {
            pendingWorkTimer_.stop();
            pendingWorkTimer_.start();
        };

        if (QThread::currentThread() == this->thread()) {
            startOrRestart();
        }
        else {
            QMetaObject::invokeMethod(this, startOrRestart, Qt::QueuedConnection);
        }
    }
    else {
        if (QThread::currentThread() == this->thread()) {
            if (!pendingWorkTimer_.isActive()) pendingWorkTimer_.start();
        }
        else {
            QMetaObject::invokeMethod(this, [this](){
                if (!pendingWorkTimer_.isActive()) pendingWorkTimer_.start();
            }, Qt::QueuedConnection);
        }
    }
}

void DataProcessor::onWorkerFinished()
{
    jobRunning_.store(false);

    if (nextRunPending_.load()) {
        startTimerIfNeeded();
    }
}

void DataProcessor::onDbTilesLoaded(const QList<DbTile> &dbTiles)
{
    dbReaderInWork_ = false;
    const QSet<TileKey> requestedNow = dbInWorkKeys_;
    dbInWorkKeys_.clear();

    TileMap outTiles; // for HotCache
    outTiles.reserve(dbTiles.size());
    QVector<TileKey> loadedKeys;
    loadedKeys.reserve(dbTiles.size());

    for (const DbTile& dt : dbTiles) {
        SurfaceTile tile(dt.key, QVector3D(dt.originX, dt.originY, 0.0f));
        tile.init(dt.tilePx, dt.hmRatio, mppFromZoom(dt.key.zoom));
        if (dt.headIndx != -1 &&!dt.mosaicBlob.isEmpty()) {
            auto bytes = MosaicDB::unpackRaw8(dt.mosaicBlob);
            auto& img = tile.getMosaicImageDataRef();
            if (static_cast<int>(bytes.size()) == int(img.size())) {
                memcpy(img.data(), bytes.data(), size_t(bytes.size()));
            }
        }
        if (dt.hasHeight && !dt.heightBlob.isEmpty()) {
            auto& verts = tile.getHeightVerticesRef();
            MosaicDB::unpackFloat32(dt.heightBlob, verts, dt.hmRatio);
        }
        if (dt.hasMarks && !dt.marksBlob.isEmpty()) {
            auto& marks = tile.getHeightMarkVerticesRef();
            MosaicDB::unpackMarksU8(dt.marksBlob, marks);
        }
        tile.setHeadIndx(dt.headIndx);
        tile.updateHeightIndices();
        tile.setIsUpdated(false);

        outTiles.insert(dt.key, std::move(tile));
        loadedKeys.push_back(dt.key);

        nfErase(dt.key);
    }

    if (!outTiles.isEmpty()) {
        hotCache_.putBatch(std::move(outTiles), DataSource::kDataBase);
        notifyPrefetchProgress(); // сообщить префетчерам
    }

    if (!loadedKeys.isEmpty()) {
        QSet<TileKey> addNow;
        for (const auto& k : loadedKeys) {
            if (lastKeys_.contains(k) && !renderedKeys_.contains(k)) {
                addNow.insert(k);
            }
        }

        if (!addNow.isEmpty()) {
            QSet<TileKey> dummy;
            TileMap delta = hotCache_.getForKeys(addNow, &dummy);
            if (!delta.isEmpty()) {
                emitDelta(std::move(delta), DataSource::kDataBase);
            }
        }
    }

    if (!requestedNow.isEmpty()) {
        QSet<TileKey> loadedSet(loadedKeys.cbegin(), loadedKeys.cend());
        QSet<TileKey> notFoundNow = requestedNow;
        notFoundNow.subtract(loadedSet);
        if (!notFoundNow.isEmpty()) {
            for (const auto& k : notFoundNow) {
                nfTouch(k);
            }
            notifyPrefetchProgress(); // сообщить префетчерам
        }
    }

    if (!dbPendingKeys_.isEmpty()) {
        flushPendingDbKeys();
    }
}

void DataProcessor::postState(DataProcessorType s)
{
    emit sendState(state_ = s);
}

void DataProcessor::postDistCompletedByProcessing(int epIndx, const ChannelId &channelId, float dist)
{
    emit distCompletedByProcessing(epIndx, channelId, dist);
}

void DataProcessor::postLastBottomTrackEpochChanged(const ChannelId &channelId, int val, const BottomTrackParam &btP, bool manual, bool redrawAll)
{
    emit lastBottomTrackEpochChanged(channelId, val, btP, manual, redrawAll);
}

void DataProcessor::postMosaicColorTable(const std::vector<uint8_t>& t)
{
    emit sendMosaicColorTable(t);
}

void DataProcessor::postIsobathsLineSegments(const QVector<QVector3D>& lineSegments)
{
    emit sendIsobathsLineSegments(lineSegments);
}

void DataProcessor::onBottomTrackStarted()
{
    btBusy_ = true;
}

void DataProcessor::onBottomTrackFinished()
{
    btBusy_ = false;
}

void DataProcessor::onSendSavedKeys(QVector<TileKey> savedKeys)
{
    hotCache_.onSendSavedTiles(savedKeys);
}

void DataProcessor::requestTilesFromDB(const QSet<TileKey>& keys)
{
    if (!dbReader_ || keys.isEmpty()) {
        return;
    }

    dbPendingKeys_.unite(keys);

    flushPendingDbKeys();
}

quint64 DataProcessor::prefetchProgressTick() const
{
    return prefetchTick_.load(std::memory_order_relaxed);
}

void DataProcessor::prefetchWait(quint64 lastTick)
{
    QMutexLocker lk(&prefetchMu_);

    // ждём пока тик изменится (или разбудят)
    while (prefetchTick_.load(std::memory_order_relaxed) == lastTick) {
        prefetchCv_.wait(&prefetchMu_);
    }
}

void DataProcessor::flushPendingDbKeys()
{
    if (!dbReader_ || dbPendingKeys_.isEmpty() || dbReaderInWork_) {
        return;
    }

    QSet<TileKey> dbReq;
    dbReq.reserve(dbPendingKeys_.size());

    for (auto it = dbPendingKeys_.begin(); it != dbPendingKeys_.end(); ) {
        const auto& k = *it;
        if (!dbInWorkKeys_.contains(k)) {
            dbReq.insert(k);
            dbInWorkKeys_.insert(k);
            it = dbPendingKeys_.erase(it);
        }
        else {
            ++it;
        }
    }

    if (dbReq.isEmpty()) {
        return;
    }

    if (dbIsReady_.load(std::memory_order_relaxed)) {
        dbReaderInWork_ = true;
        emit dbLoadTilesForKeys(dbReq);
    }
}

void DataProcessor::postIsobathsLabels(const QVector<IsobathUtils::LabelParameters>& labels)
{
    emit sendIsobathsLabels(labels);
}

void DataProcessor::postSurfaceTiles(TileMap tiles, bool isMosaic)
{
    if (tiles.isEmpty()) {
        return;
    }

    for (auto it = tiles.cbegin(); it != tiles.cend(); ++it) { // удалить из не найдено
        nfErase(it.key());
    }

    bool canSendToRender = false;
    if ((isMosaic && updateMosaic_) ||
        (!isMosaic && updateSurface_ && !updateMosaic_)) {
        canSendToRender = true;
    }

    if (canSendToRender) { // в рендер — только видимые
        TileMap prepaired;
        for (auto it = tiles.cbegin(); it != tiles.cend(); ++it) {
            const auto& k = it.key();
            if (lastKeys_.contains(k)) {
                prepaired.insert(k, it.value());
            }
        }

        if (!prepaired.isEmpty()) {
            emitDelta(std::move(prepaired), DataSource::kCalculation);
        }
    }

    hotCache_.putBatch(std::move(tiles), DataSource::kCalculation);
    notifyPrefetchProgress(); // сообщить префетчерам
}

void DataProcessor::postMinZ(float val)
{
    QMetaObject::invokeMethod(worker_, "setMinZ", Qt::QueuedConnection, Q_ARG(float, val));

    emit sendSurfaceMinZ(val); // to surface view

    pendingIsobathsWork_ = true;

    scheduleLatest(WorkSet(WF_Isobaths));
}

void DataProcessor::postMaxZ(float val)
{
    QMetaObject::invokeMethod(worker_, "setMaxZ", Qt::QueuedConnection, Q_ARG(float, val));

    emit sendSurfaceMaxZ(val); // to surface view

    pendingIsobathsWork_ = true;

    scheduleLatest(WorkSet(WF_Isobaths));
}

void DataProcessor::postSurfaceColorTable(const std::vector<uint8_t> &t)
{
    emit sendSurfaceTextureTask(t);
}

void DataProcessor::postSurfaceColorIntervalsSize(int size)
{
    emit sendSurfaceColorIntervalsSize(size);
}

void DataProcessor::postSurfaceStepSize(float lineStepSize)
{
    emit sendSurfaceStepSize(lineStepSize);
}

void DataProcessor::changeState(const DataProcessorType& state)
{
    emit sendState(state_ = state);
}

void DataProcessor::clearBottomTrackProcessing()
{
    bottomTrackWindowCounter_ = 0;
    btBusy_ = false;

    QMetaObject::invokeMethod(worker_, "clearBottomTrack", Qt::QueuedConnection);
}

void DataProcessor::clearIsobathsProcessing()
{
    pendingIsobathsWork_ = false;

    QMetaObject::invokeMethod(worker_, "clearIsobaths", Qt::QueuedConnection);
}

void DataProcessor::clearMosaicProcessing()
{
    pendingMosaicIndxs_.clear();

    QMetaObject::invokeMethod(worker_, "clearMosaic", Qt::QueuedConnection);
}

void DataProcessor::clearSurfaceProcessing()
{
    pendingSurfaceIndxs_.clear();
    surfaceTaskEpochIndxsByZoom_.clear();
    surfaceManualEpochIndxsByZoom_.clear();

    QMetaObject::invokeMethod(worker_, "clearSurface", Qt::QueuedConnection);
}

void DataProcessor::clearAllProcessings()
{
    closeDB();
    pendingWorkTimer_.stop();
    pendingIsobathsWork_ = false;
    pendingMosaicIndxs_.clear();
    pendingSurfaceIndxs_.clear();
    surfaceTaskEpochIndxsByZoom_.clear();
    surfaceManualEpochIndxsByZoom_.clear();
    epIndxsFromBottomTrack_.clear();
    vertIndxsFromBottomTrack_.clear();
    bottomTrackWindowCounter_ = 0;
    btBusy_ = false;

    QMetaObject::invokeMethod(worker_, "clearAll", Qt::QueuedConnection);

    state_ = DataProcessorType::kUndefined;

    chartsCounter_ = 0;
    bottomTrackCounter_ = 0;
    epochCounter_ = 0;
    positionCounter_ = 0;
    attitudeCounter_ = 0;
    mosaicCounter_ = 0;
    cancelRequested_ = false;
    jobRunning_ = false;
    nextRunPending_ = false;
    requestedMask_ = 0;

    hotCache_.clear();
    filePath_.clear();
    dbReaderInWork_ = false;
    lastViewRect_ = QRectF();
    lastZoom_ = 0;
    lastDataZoomIndx_ = 0;
    lastKeys_.clear();
    dbPendingKeys_.clear();
    dbInWorkKeys_.clear();
    renderedKeys_.clear();

    dbNotFoundIndxs_.clear();
    dbNotFoundOrder_.clear();
    dbNotFoundPos_.clear();

    emit isobathsProcessingCleared();
    emit surfaceProcessingCleared();
    emit mosaicProcessingCleared();
}

void DataProcessor::scheduleLatest(WorkSet mask, bool replace, bool clearUnrequestedPending) noexcept
{
    const uint32_t m = toMask(mask);
    if (replace) {
        requestedMask_.store(m);
    }
    else {
        requestedMask_.fetch_or(m);
    }

    if (clearUnrequestedPending) {
        if (!(m & WF_Surface)) {
            pendingSurfaceIndxs_.clear();
        }
        if (!(m & WF_Mosaic)) {
            pendingMosaicIndxs_.clear();
        }
        if (!(m & WF_Isobaths)) {
            pendingIsobathsWork_ = false;
        }
    }

    nextRunPending_.store(true);

    if (replace && jobRunning_.load()) {
        cancelRequested_.store(true);
    }

    startTimerIfNeeded();
}

void DataProcessor::openDB()
{
    if (dbReader_ || dbWriter_ || filePath_.isEmpty()) {
        return;
    }

    // writer
    dbWriter_ = new MosaicDB(filePath_, DbRole::Writer, true /*delete temp db files*/);
    dbWriter_->moveToThread(&dbWriteThread_);

    connect(&dbWriteThread_, &QThread::finished, dbWriter_, &QObject::deleteLater);
    connect(&dbWriteThread_, &QThread::started,  dbWriter_, [this]() {
        if (!dbWriter_->open()) {
            qWarning() << "DB Writer open failed";
        }
    }, Qt::QueuedConnection);

    // схема готова - запускаем reader
    connect(dbWriter_, &MosaicDB::schemaReady, this, [this]() {
        dbIsReady_.store(true, std::memory_order_relaxed);

        // reader
        dbReader_ = new MosaicDB(filePath_, DbRole::Reader);
        dbReader_->moveToThread(&dbReadThread_);
        connect(&dbReadThread_, &QThread::finished, dbReader_, &QObject::deleteLater);
        connect(&dbReadThread_, &QThread::started,  dbReader_, [this]() {
            if (!dbReader_->open()) {
                qWarning() << "DB Reader open failed";
            }
        }, Qt::QueuedConnection);

        // reader conns
        connect(this,      &DataProcessor::dbLoadTilesForKeys,    dbReader_, &MosaicDB::loadTilesForKeys,            Qt::QueuedConnection);
        connect(dbReader_, &MosaicDB::tilesLoadedForKeys,         this,      &DataProcessor::onDbTilesLoaded,        Qt::QueuedConnection);
        connect(this,      &DataProcessor::dbCheckAnyTileForZoom, dbReader_, &MosaicDB::checkAnyTileForZoom,         Qt::QueuedConnection);
        connect(dbReader_, &MosaicDB::anyTileForZoom,             this,      &DataProcessor::onDbAnyTileForZoom,     Qt::QueuedConnection);

        dbReadThread_.setObjectName("MosaicDBReaderThread");
        dbReadThread_.start();

        flushPendingDbKeys(); // были запросы
    }, Qt::QueuedConnection);

    // writer conns
    connect(this,      &DataProcessor::dbSaveTiles,  dbWriter_, &MosaicDB::saveTiles,            Qt::QueuedConnection);
    connect(dbWriter_, &MosaicDB::sendSavedKeys,     this,      &DataProcessor::onSendSavedKeys, Qt::QueuedConnection);

    dbWriteThread_.setObjectName("MosaicDBWriterThread");
    dbWriteThread_.start();
}

void DataProcessor::closeDB()
{
    // Reader
    if (dbReader_) {
        QMetaObject::invokeMethod(dbReader_, "finalizeAndClose", Qt::BlockingQueuedConnection);
        dbReadThread_.quit();
        dbReadThread_.wait();
        dbReader_ = nullptr;
    }

    // Writer
    if (dbWriter_) {
        QMetaObject::invokeMethod(dbWriter_, "finalizeAndClose", Qt::BlockingQueuedConnection);
        dbWriteThread_.quit();
        dbWriteThread_.wait();
        dbWriter_ = nullptr;
    }

    dbIsReady_.store(false, std::memory_order_relaxed);
    notifyPrefetchProgress(); // сообщить префетчерам

    if (!filePath_.isEmpty() && !MosaicDB::removeDbFiles(filePath_)) {
        qWarning() << "Failed to remove mosaic cache" << filePath_;
    }

    qDebug() << "DB closed";
}

void DataProcessor::emitDelta(TileMap &&upserts, DataSource src)
{
    if (upserts.isEmpty()) {
        return;
    }

    markTilesBySource(upserts, src);

    // в рендер: сам удалит всё, чего нет
    emit sendSurfaceTilesIncremental(upserts, lastKeys_);

    // «что отрисовано»
    renderedKeys_.intersect(lastKeys_); // выкидываем то, что больше не видно (проверить)
    for (auto it = upserts.cbegin(); it != upserts.cend(); ++it) {
        renderedKeys_.insert(it.key());
    }
}

void DataProcessor::pumpVisible()
{
    QSet<TileKey> needAdd = lastKeys_;

    // check RENDER, PENDING DB/PROC
    needAdd.subtract(renderedKeys_);
    needAdd.subtract(dbPendingKeys_);
    if (needAdd.empty()) {
        //qDebug() << "needAdd.empty() - return";
        return;
    }

    // CHECK IN HOT CACHE
    QSet<TileKey> missFromHotCache;
    TileMap fromHotCache = hotCache_.getForKeys(needAdd, &missFromHotCache);

    if (!fromHotCache.isEmpty()) {
        emitDelta(std::move(fromHotCache), DataSource::kHotCache);
    }
    if (missFromHotCache.isEmpty()) {
        return;
    }

    missFromHotCache.subtract(dbNotFoundIndxs_); // убрать те, которых нет в бд

    if (missFromHotCache.isEmpty()) {
        return;
    }

    requestTilesFromDB(missFromHotCache);
}

bool DataProcessor::isValidZoomIndx(int zoomIndx) const
{
    return zoomIndx >= mosaicIndexProvider_.getMaxZoom() && zoomIndx <= mosaicIndexProvider_.getMinZoom();
}

void DataProcessor::handleSurfaceZoomChangeIfReady(int zoom, const QSet<TileKey>& keys)
{
    if (!surfaceZoomChangedPending_) {
        return;
    }

    if (!updateSurface_) {
        surfaceZoomChangedPending_ = false;
        return;
    }

    if (keys.isEmpty()) {
        return;
    }

    for (const TileKey& k : keys) {
        if (k.zoom != zoom) {
            return;
        }
    }

    QSet<TileKey> missing;
    TileMap cached = hotCache_.getForKeys(keys, &missing);

    const bool fullCoverage = missing.isEmpty() && !cached.isEmpty();
    if (!fullCoverage) {
        cached.clear();
    }
    QMetaObject::invokeMethod(worker_, "applySurfaceZoomChange", Qt::QueuedConnection,
                              Q_ARG(TileMap, cached),
                              Q_ARG(bool, fullCoverage));

    if (updateIsobaths_ && !updateMosaic_) {
        pendingIsobathsWork_ = true;
        scheduleLatest(WorkSet(WF_Isobaths));
    }

    surfaceZoomChangedPending_ = false;
}

void DataProcessor::nfTouch(const TileKey &k)
{
    auto it = dbNotFoundPos_.find(k);
    if (it != dbNotFoundPos_.end()) {
        dbNotFoundOrder_.splice(dbNotFoundOrder_.end(), dbNotFoundOrder_, it.value()); // в хвост
        return;
    }

    dbNotFoundOrder_.push_back(k); // новый
    auto newIt = dbNotFoundOrder_.end();
    --newIt; // только что добавленный
    dbNotFoundPos_.insert(k, newIt);
    dbNotFoundIndxs_.insert(k);

    if (static_cast<int>(dbNotFoundOrder_.size()) > dbNotFoundLimit_) { // переполнение
        const TileKey& old = dbNotFoundOrder_.front();
        dbNotFoundPos_.remove(old);
        dbNotFoundIndxs_.remove(old);
        dbNotFoundOrder_.pop_front();
    }
}

void DataProcessor::nfErase(const TileKey &k)
{
    auto it = dbNotFoundPos_.find(k);
    if (it == dbNotFoundPos_.end()) {
        return;
    }

    dbNotFoundOrder_.erase(it.value());
    dbNotFoundPos_.erase(it);
    dbNotFoundIndxs_.remove(k);
}

bool DataProcessor::isCanStartCalculations() const
{
    return chartsCounter_ || bottomTrackCounter_ || epochCounter_ || positionCounter_ || attitudeCounter_ || mosaicCounter_;
}

void DataProcessor::requestCancel() noexcept
{
    nextRunPending_.store(true);
    cancelRequested_.store(true);
    notifyPrefetchProgress(); // сообщить префетчерам
}

void DataProcessor::onUpdateDataZoom(int zoom) // calc or db
{
    //return;

    if (!isValidZoomIndx(zoom)) {
        return;
    }

    if (zoom == lastZoom_) {
        qDebug() << "Skip: zoom already applied" << zoom;
        return;
    }

    lastZoom_ = zoom;

    const float pxPerMeter = ZL[zoom - 1].pxPerMeter;
    if (!std::isfinite(pxPerMeter)) {
        return;
    }
    //qDebug() << "pxPerMeter" << pxPerMeter;

    tileResolution_ = 1.0f / pxPerMeter;
    QMetaObject::invokeMethod(worker_, "setMosaicTileResolution", Qt::QueuedConnection, Q_ARG(float, tileResolution_));

    surfaceZoomChangedPending_ = updateSurface_;
    handleSurfaceZoomChangeIfReady(lastZoom_, lastKeys_);

    if (!updateMosaic_) {
        return;
    }

    emit mosaicProcessingCleared();
    emit surfaceProcessingCleared();

    if (hotCache_.checkAnyTileForZoom(lastZoom_)) {
        pumpVisible();
    }
    else {
        if (dbIsReady_) {
            emit dbCheckAnyTileForZoom(lastZoom_);
        }
    }
}

void DataProcessor::setFilePath(QString filePath)
{
    Q_UNUSED(filePath);

    const QString dbPath = MosaicDB::surfaceDbPath();
    if (!MosaicDB::removeDbFiles(dbPath)) {
        qWarning() << "Failed to cleanup previous mosaic cache" << dbPath;
    }

    filePath_ = dbPath;

    openDB();
}

void DataProcessor::onSendDataRectRequest(QVector<NED> rect, int zoomIndx, bool moveUp)
{
    Q_UNUSED(moveUp)

    if (rect.size() < 4 || !isValidZoomIndx(zoomIndx)) {
        return;
    }

    // dimensions
    double minN =  std::numeric_limits<double>::max();
    double minE =  std::numeric_limits<double>::max();
    double maxN = -std::numeric_limits<double>::max();
    double maxE = -std::numeric_limits<double>::max();
    for (const auto& p : rect) {
        minN = std::min(minN, p.n);
        minE = std::min(minE, p.e);
        maxN = std::max(maxN, p.n);
        maxE = std::max(maxE, p.e);
    }

    // rect
    const QRectF viewRect(QPointF(minN, minE), QPointF(maxN, maxE));
    if (viewRect == lastViewRect_ && zoomIndx == lastZoom_) {
        return;
    }

    // tiles
    // been
    //const auto tiles = mosaicIndexProvider_.tilesInRectNed(viewRect, zoomIndx, /*padTiles*/1);
    //if (tiles == lastKeys_) {
    //    return;
    //}
    // now
    std::array<QPointF, 4> visQuad;
    visQuad[0] = viewRect.topLeft();
    visQuad[1] = viewRect.topRight();
    visQuad[2] = viewRect.bottomRight();
    visQuad[3] = viewRect.bottomLeft();

    const auto tiles = mosaicIndexProvider_.tilesInQuadNed(visQuad, zoomIndx, 1); // TILES
    if (tiles == lastKeys_) {
        return;
    }

    // store new req
    lastViewRect_ = viewRect;
    lastKeys_     = tiles;
    lastZoom_     = zoomIndx;

    handleSurfaceZoomChangeIfReady(zoomIndx, tiles);

    if (!updateSurface_ && !updateMosaic_) {
        return;
    }

    // check // calc starts in in updateMosaic
    //if (state_ != DataProcessorType::kUndefined) {
    //    return;
    //}

    pumpVisible();
}

void DataProcessor::tryCalcTiles()
{
    //qDebug() << "DataProcessor::tryCalcTiles";

    emit isobathsProcessingCleared();
    emit surfaceProcessingCleared();
    emit mosaicProcessingCleared();

    // замена на то что пришло с камеры?
    for (auto it = epIndxsFromBottomTrack_.cbegin(); it != epIndxsFromBottomTrack_.cend(); ++it) {
        pendingMosaicIndxs_.insert(*it);
        pendingSurfaceIndxs_.insert(qMakePair('0', *it));
    }
    pendingIsobathsWork_ = true;

    QMetaObject::invokeMethod(worker_, "setMosaicTileResolution", Qt::QueuedConnection, Q_ARG(float, tileResolution_));
    scheduleLatest(WorkSet(WF_All), /*replace*/true);
}

TileMap DataProcessor::fetchFromHotCache(const QSet<TileKey> &keys, QSet<TileKey> *missing)
{
    auto retVal = hotCache_.getForKeys(keys, missing);

    //if (!missing->empty())
    //{
    //    qDebug() << "perhaps marginTiles" << missing->size();
    //}

    return retVal;
}

void DataProcessor::filterNotFoundOut(const QSet<TileKey> &in, QSet<TileKey> *out)
{
    if (!out) {
        return;
    }

    QSet<TileKey> res = in;
    res.subtract(dbNotFoundIndxs_);

    *out = std::move(res);
}

void DataProcessor::shutdown()
{
    //qDebug() << "DataProcessor::shutdown()";

    requestCancel();

    if (dbReader_) {
        QMetaObject::invokeMethod(dbReader_, "finalizeAndClose", Qt::BlockingQueuedConnection);
        dbReadThread_.quit();
        dbReadThread_.wait();
        dbReader_ = nullptr;
    }

    if (dbWriter_) {
        QMetaObject::invokeMethod(dbWriter_, "finalizeAndClose", Qt::BlockingQueuedConnection);
        dbWriteThread_.quit();
        dbWriteThread_.wait();
        dbWriter_ = nullptr;
    }

    computeThread_.quit();
    computeThread_.wait();
    worker_->deleteLater();

    dbIsReady_ = false;

    qDebug() << "DataProcessor::shutdown()";
}

void DataProcessor::onSendTilesByZoom(int epochIndx, const QMap<int, QSet<TileKey> > &tilesByZoom)
{
    //qDebug() << "DataProcessor::onSendDimRects" << epochIndx;

    const int minZoom = 7;
    if (tileEpochIndxsByZoom_.size() < minZoom) {
        tileEpochIndxsByZoom_.resize(minZoom);
    }

    for (auto it = tilesByZoom.cbegin(); it != tilesByZoom.cend(); ++it) {
        const int zoom = it.key() - 1;
        if (zoom < 0 || zoom >= tileEpochIndxsByZoom_.size()) {
            continue;
        }

        auto& indexForZoom = tileEpochIndxsByZoom_[zoom];
        const QSet<TileKey>& tileSet = it.value();
        for (const TileKey& tk : tileSet) {
            auto& epochList = indexForZoom[tk];
            if (epochList.isEmpty() || epochList.back() != epochIndx) {
                epochList.push_back(epochIndx);
            }
        }
    }
}

void DataProcessor::onDatasetStateChanged(int state)
{
    if (datasetState_ == state) {
        return;
    }

    datasetState_ = state;

#ifdef SEPARATE_READING
    defProcType_ = false;
#else
    if (datasetState_ == 0 || datasetState_ == 1) {
        defProcType_ = true;
    }
    if (datasetState_ == 2) {
        defProcType_ = false;
    }
#endif

    pendingWorkTimer_.setInterval(defProcType_ ? 333 : 10);
}

bool DataProcessor::isDbReady() const noexcept
{
    return dbIsReady_.load(std::memory_order_relaxed);
}

void DataProcessor::onDbSaveTiles(const QHash<TileKey, SurfaceTile> &tiles)
{
    emit dbSaveTiles(engineVer_, tiles, true, defaultTileSidePixelSize, defaultTileHeightMatrixRatio);
}

void DataProcessor::notifyPrefetchProgress()
{
    QMutexLocker lk(&prefetchMu_);
    prefetchTick_.fetch_add(1, std::memory_order_relaxed);
    prefetchCv_.wakeAll();
}

void DataProcessor::clearDbNotFoundCache()
{
    dbNotFoundIndxs_.clear();
    dbNotFoundOrder_.clear();
    dbNotFoundPos_.clear();
}

void DataProcessor::enqueueSurfaceMissingForZoom(int zoom)
{
    if (!updateSurface_ || zoom <= 0) {
        return;
    }

    auto& processed = surfaceTaskEpochIndxsByZoom_[zoom];
    const auto& manualPending = surfaceManualEpochIndxsByZoom_[zoom];

    for (int idx : std::as_const(vertIndxsFromBottomTrack_)) {
        if (processed.contains(idx) || manualPending.contains(idx)) {
            continue;
        }
        pendingSurfaceIndxs_.insert(qMakePair('0', idx));
    }
}

QVector<QPair<int, QSet<TileKey>>> DataProcessor::collectEpochsForTiles(int zoom, const QSet<TileKey> &tiles) const
{
    int cZoom = zoom - 1;

    QVector<QPair<int, QSet<TileKey>>> result;
    if (tiles.isEmpty()) {
        return result;
    }

    if (cZoom < 0 || cZoom >= tileEpochIndxsByZoom_.size()) {
        return result;
    }

    const auto& indexForZoom = tileEpochIndxsByZoom_.at(cZoom);
    if (indexForZoom.isEmpty()) {
        return result;
    }

    QMap<int, QSet<TileKey>> tilesByEpoch;

    for (const TileKey& tk : tiles) {
        auto it = indexForZoom.constFind(tk);
        if (it == indexForZoom.cend()) {
            continue;
        }

        const QVector<int>& epochList = it.value();
        for (int epochIndx : epochList) {
                tilesByEpoch[epochIndx].insert(tk);
        }
    }

    result.reserve(tilesByEpoch.size());
    for (auto it = tilesByEpoch.cbegin(); it != tilesByEpoch.cend(); ++it) {
        result.push_back(QPair<int, QSet<TileKey>>(it.key(), it.value()));
    }

    return result;
}

void DataProcessor::onDbTilesLoadedForZoom(int zoom, const QList<DbTile>& dbTiles)
{
    qDebug() << "DataProcessor::onDbTilesLoadedForZoom size" << dbTiles.size();
    dbReaderInWork_ = false;

    if (zoom != lastZoom_) {
        return;
    }

    if (!dbTiles.isEmpty()) {
        // можно реализовать массовую подгрузку по zoom, но сейчас используем по-ключевую
        return;
    }

    //  иначе расчёт по всем эпохам
    emit isobathsProcessingCleared();
    emit surfaceProcessingCleared();
    emit mosaicProcessingCleared();

    for (auto it = epIndxsFromBottomTrack_.cbegin(); it != epIndxsFromBottomTrack_.cend(); ++it) {
        pendingMosaicIndxs_.insert(*it);
        pendingSurfaceIndxs_.insert(qMakePair('0', *it));
    }
    pendingIsobathsWork_ = true;

    QMetaObject::invokeMethod(worker_, "setMosaicTileResolution", Qt::QueuedConnection, Q_ARG(float, tileResolution_));
    scheduleLatest(WorkSet(WF_All), /*replace*/true);
}

void DataProcessor::onDbAnyTileForZoom(int zoom, bool exists)
{
    Q_UNUSED(zoom)

    //qDebug() << "DataProcessor::onDbAnyTileForZoom, zoom" << zoom << "exist" << exists;

    if (exists) {
        pumpVisible();
    }
    else {
        tryCalcTiles();
    }
}

void DataProcessor::postTraceLines(const QVector3D &leftBeg, const QVector3D &leftEnd, const QVector3D &rightBeg, const QVector3D &rightEnd)
{
    emit sendTraceLines(leftBeg, leftEnd, rightBeg, rightEnd);
}
