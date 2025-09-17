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
    requestedMask_(0)
{
    qRegisterMetaType<WorkBundle>("WorkBundle");
    qRegisterMetaType<DatasetChannel>("DatasetChannel");
    qRegisterMetaType<ChannelId>("ChannelId");
    qRegisterMetaType<BottomTrackParam>("BottomTrackParam");
    qRegisterMetaType<QVector<IsobathUtils::LabelParameters>>("QVector<IsobathUtils::LabelParameters>");
    qRegisterMetaType<TileMap>("TileMap");
    qRegisterMetaType<Dataset*>("Dataset*");
    qRegisterMetaType<std::uint8_t>("std::uint8_t");

    pendingWorkTimer_.setParent(this);
    pendingWorkTimer_.setSingleShot(true);
    pendingWorkTimer_.setInterval(10);
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
    requestCancel();

    closeDB();

    computeThread_.quit();
    computeThread_.wait();
    worker_->deleteLater();
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
    // TODO отдельный сброс настроек

    requestCancel();

    switch (procType) {
    case DataProcessorType::kUndefined:   clearAllProcessings();        emit allProcessingCleared();         break;
    case DataProcessorType::kBottomTrack: clearBottomTrackProcessing(); emit bottomTrackProcessingCleared(); break;
    case DataProcessorType::kIsobaths:    clearIsobathsProcessing();    emit isobathsProcessingCleared();    break;
    case DataProcessorType::kMosaic:      clearMosaicProcessing();      emit mosaicProcessingCleared();      break;
    case DataProcessorType::kSurface:     clearSurfaceProcessing();     emit surfaceProcessingCleared();     break;
    default: break;
    }

    chartsCounter_ = 0;
    bottomTrackCounter_ = 0;
    epochCounter_ = 0;
    positionCounter_ = 0;
    attitudeCounter_ = 0;
    mosaicCounter_ = 0;
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
    updateSurface_ = state;

    if (updateSurface_ && !pendingSurfaceIndxs_.isEmpty()) {
        scheduleLatest(WorkSet(WF_Surface));
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

    if (updateMosaic_) {
        requestTilesFromDB();
    }
    else if (!updateMosaic_) {
        pendingIsobathsWork_ = true; // мозаика могла изменить поверхность
        scheduleLatest(WorkSet(WF_Isobaths));
    }
}

void DataProcessor::setIsOpeningFile(bool state)
{
    isOpeningFile_ = state;
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
                                          Q_ARG(bool, false));
            }
            bottomTrackWindowCounter_ = currCount;
        }
    }
}

void DataProcessor::onBottomTrackAdded(const QVector<int> &indxs, bool manual, bool isDel)
{
    if (indxs.isEmpty()) {
        return;
    }

    if (!isDel) {
        for (int itm : indxs) {
            epIndxsFromBottomTrack_.insert(itm);
            pendingSurfaceIndxs_.insert(qMakePair(manual ? '1' : '0', itm));
            pendingMosaicIndxs_.insert(itm);
        }

        pendingIsobathsWork_ = true;

        //return; // TODO: ruins last saving if run on file opening

        scheduleLatest(WorkSet(WF_All));
    }
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

void DataProcessor::bottomTrackProcessing(const DatasetChannel &ch1, const DatasetChannel &ch2, const BottomTrackParam &p, bool manual)
{
    if (btBusy_) {
        //qDebug() << "bt skip - busy";
        return;
    }

    QMetaObject::invokeMethod(worker_, "bottomTrackProcessing", Qt::QueuedConnection,
                              Q_ARG(DatasetChannel, ch1),
                              Q_ARG(DatasetChannel, ch2),
                              Q_ARG(BottomTrackParam, p),
                              Q_ARG(bool, manual));
}

void DataProcessor::setSurfaceColorTableThemeById(int id)
{
    QMetaObject::invokeMethod(worker_, "setSurfaceThemeId", Qt::QueuedConnection, Q_ARG(int, id));
}

void DataProcessor::setSurfaceEdgeLimit(int val)
{
    QMetaObject::invokeMethod(worker_, "setSurfaceEdgeLimit", Qt::QueuedConnection, Q_ARG(float, float(val)));

    pendingIsobathsWork_ = true;

    for (auto it = epIndxsFromBottomTrack_.cbegin(); it != epIndxsFromBottomTrack_.cend(); ++it) {
        pendingSurfaceIndxs_.insert(qMakePair('0', *it));
    }

    scheduleLatest(WorkSet(WF_All), /*replace*/true);
}

void DataProcessor::setExtraWidth(int val)
{
    QMetaObject::invokeMethod(worker_, "setSurfaceExtraWidth", Qt::QueuedConnection, Q_ARG(int, val));
}

void DataProcessor::setIsobathsLabelStepSize(float val)
{
    QMetaObject::invokeMethod(worker_, "setIsobathsLabelStepSize", Qt::QueuedConnection, Q_ARG(float, val));
}

void DataProcessor::setSurfaceIsobathsStepSize(float val)
{
    QMetaObject::invokeMethod(worker_, "setSurfaceIsobathsStepSize", Qt::QueuedConnection, Q_ARG(float, val));

    pendingIsobathsWork_ = true;

    for (auto it = epIndxsFromBottomTrack_.cbegin(); it != epIndxsFromBottomTrack_.cend(); ++it) {
        pendingSurfaceIndxs_.insert(qMakePair('0', *it));
    }

    scheduleLatest(WorkSet(WF_All), /*replace*/true);
}

void DataProcessor::setMosaicChannels(const ChannelId &ch1, uint8_t sub1, const ChannelId &ch2, uint8_t sub2)
{
    QMetaObject::invokeMethod(worker_, "clearAll", Qt::QueuedConnection);

    emit isobathsProcessingCleared();
    emit surfaceProcessingCleared();
    emit mosaicProcessingCleared();

    QMetaObject::invokeMethod(worker_, "setMosaicChannels", Qt::QueuedConnection,
                              Q_ARG(ChannelId, ch1), Q_ARG(uint8_t, sub1),
                              Q_ARG(ChannelId, ch2), Q_ARG(uint8_t, sub2));

    for (auto it = epIndxsFromBottomTrack_.cbegin(); it != epIndxsFromBottomTrack_.cend(); ++it) {
        pendingMosaicIndxs_.insert(*it);
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
    //qDebug() << "DataProcessor::setMosaicTileResolution" << val;
    if (qFuzzyIsNull(val)) {
        return;
    }

    const float convertedResolution = 1.0f / val;
    if (qFuzzyCompare(1.0f + tileResolution_, 1.0f + convertedResolution)) {
        return;
    }

    tileResolution_ = convertedResolution;

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
    if (jobRunning_.load()) {
        return;
    }

    const uint32_t maskNow = requestedMask_.exchange(0);
    const bool wantSurface  = maskNow & WF_Surface;
    const bool wantMosaic   = maskNow & WF_Mosaic;
    const bool wantIsobaths = maskNow & WF_Isobaths;

    WorkBundle wb;

    if (wantSurface && !pendingSurfaceIndxs_.isEmpty() && updateSurface_) {
        wb.surfaceVec.reserve(pendingSurfaceIndxs_.size());

        for (auto it = pendingSurfaceIndxs_.cbegin(); it != pendingSurfaceIndxs_.cend(); ++it) {
            wb.surfaceVec.append(*it);
        }

        std::sort(wb.surfaceVec.begin(), wb.surfaceVec.end(), [](const QPair<char,int>& a, const QPair<char,int>& b){ return a.second < b.second; });

        pendingSurfaceIndxs_.clear();
    }
    //qDebug() << " runCoalescedWork pendingMosaicIndxs_" << pendingMosaicIndxs_.size();

    if (wantMosaic && !pendingMosaicIndxs_.isEmpty() && updateMosaic_) {
        wb.mosaicVec.reserve(pendingMosaicIndxs_.size());
        for (auto it = pendingMosaicIndxs_.cbegin(); it != pendingMosaicIndxs_.cend(); ++it) {
            if (mosaicCounter_ >= *it) {
                wb.mosaicVec.append(*it);
            }
        }

        std::sort(wb.mosaicVec.begin(), wb.mosaicVec.end());
        pendingMosaicIndxs_.clear();
    }

    if (wantIsobaths && pendingIsobathsWork_ && updateIsobaths_ && !updateMosaic_) {
        wb.doIsobaths = true;
        pendingIsobathsWork_ = false;
    }

    if (wb.surfaceVec.isEmpty() && wb.mosaicVec.isEmpty() && !wb.doIsobaths) {
        return;
    }

    nextRunPending_.store(false);
    cancelRequested_.store(false);
    jobRunning_.store(true);

    // queue в воркер в его поток
    QMetaObject::invokeMethod(worker_, "processBundle", Qt::QueuedConnection, Q_ARG(WorkBundle, wb));
}

void DataProcessor::startTimerIfNeeded()
{
    if (QThread::currentThread() == this->thread()) {
        if (!pendingWorkTimer_.isActive()) pendingWorkTimer_.start();
    }
    else {
        QMetaObject::invokeMethod(this, [this](){
            if (!pendingWorkTimer_.isActive()) pendingWorkTimer_.start();
        }, Qt::QueuedConnection);
    }
}

void DataProcessor::onWorkerFinished()
{
    jobRunning_.store(false);

    if (nextRunPending_.load()) {
        startTimerIfNeeded();
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

void DataProcessor::postLastBottomTrackEpochChanged(const ChannelId &channelId, int val, const BottomTrackParam &btP, bool manual)
{
    emit lastBottomTrackEpochChanged(channelId, val, btP, manual);
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

void DataProcessor::postIsobathsLabels(const QVector<IsobathUtils::LabelParameters>& labels)
{
    emit sendIsobathsLabels(labels);
}

void DataProcessor::postSurfaceTiles(const TileMap& tiles, bool useTextures)
{
    if (tiles.isEmpty()) {
        return;
    }

    qDebug() << "CALCULATED zoom:" << requestedZoom_ << "tiles:" << tiles.size() <<"mosaic:"<< useTextures;
    if (persistToDb_ && db_ && useTextures && !loadingFromDb_) { // только с мозайки сохраняем
        emit dbSaveTiles(engineVer_, tiles, useTextures, defaultTileSidePixelSize, defaultTileHeightMatrixRatio);
    }

    emit sendSurfaceTiles(tiles, useTextures); // to render
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

    QMetaObject::invokeMethod(worker_, "clearSurface", Qt::QueuedConnection);
}

void DataProcessor::clearAllProcessings()
{
    closeDB();
    filePath_.clear();

    pendingIsobathsWork_ = false;
    pendingMosaicIndxs_.clear();
    pendingSurfaceIndxs_.clear();
    epIndxsFromBottomTrack_.clear();
    bottomTrackWindowCounter_ = 0;
    btBusy_ = false;

    QMetaObject::invokeMethod(worker_, "clearAll", Qt::QueuedConnection);

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
    if (!db_) {
        db_ = new MosaicDB(filePath_);
        db_->moveToThread(&dbThread_);
        connect(&dbThread_, &QThread::finished, db_, &QObject::deleteLater);
        connect(&dbThread_, &QThread::started, db_, [this](){
            if (!db_->open()) {
                qWarning() << "DB open failed";
            }
        }, Qt::QueuedConnection);

        connect(this, &DataProcessor::dbLoadTilesForZoom, db_,  &MosaicDB::loadTilesForZoom,            Qt::QueuedConnection);
        connect(this, &DataProcessor::dbSaveTiles,        db_,  &MosaicDB::saveTiles,                   Qt::QueuedConnection);
        connect(db_,  &MosaicDB::tilesLoadedForZoom,      this, &DataProcessor::onDbTilesLoadedForZoom, Qt::QueuedConnection);

        dbThread_.setObjectName("MosaicDBThread");
        dbThread_.start();

        qDebug() << "DB opened by path" << filePath_;
    }
}

void DataProcessor::closeDB()
{
    if (!db_) {
        return;
    }

    QMetaObject::invokeMethod(db_, "close", Qt::BlockingQueuedConnection);
    db_->deleteLater();

    dbThread_.quit();
    dbThread_.wait();
    db_ = nullptr;
    qDebug() << "DB close by path" << filePath_;
}

void DataProcessor::requestTilesFromDB()
{
    if (db_) {
        if (!dbInFlight_) {
            dbInFlight_ = true;
            const auto seq = ++reqSeq_;
            QMetaObject::invokeMethod(db_, [this, seq](){
                db_->loadTilesForZoom(requestedZoom_, seq);
            }, Qt::QueuedConnection);
        }
    }
}

void DataProcessor::requestCancel() noexcept
{
    nextRunPending_.store(true);
    cancelRequested_.store(true);
}

void DataProcessor::onUpdateMosaic(int zoom)
{
    if (zoom < 1 || zoom > 7) {
        return;
    }

    if (zoom == currentZoom_) {
        qDebug() << "Skip: zoom already applied" << zoom;
        return;
    }

    requestedZoom_ = zoom;

    const float pxPerMeter = ZL[zoom - 1].pxPerMeter;
    if (!std::isfinite(pxPerMeter)) {
        return;
    }
    //qDebug() << "pxPerMeter" << pxPerMeter;

    tileResolution_ = 1.0f / pxPerMeter;
    QMetaObject::invokeMethod(worker_, "setMosaicTileResolution", Qt::QueuedConnection, Q_ARG(float, tileResolution_));

    if (!updateMosaic_) {
        return;
    }

    emit mosaicProcessingCleared();
    emit surfaceProcessingCleared();

    requestTilesFromDB();
}

void DataProcessor::setFilePath(QString filePath)
{
    filePath_ = filePath;

    openDB();
}

void DataProcessor::onDbTilesLoadedForZoom(int zoom, const QList<DbTile>& dbTiles)
{
    dbInFlight_ = false;

    if (zoom != requestedZoom_) { 
        return; 
    }

    if (!dbTiles.isEmpty()) {
        TileMap out; out.reserve(dbTiles.size());
        for (const DbTile& dt : dbTiles) {
            SurfaceTile tile(dt.key, QVector3D(dt.originX, dt.originY, 0.0f));
            tile.init(dt.tilePx, dt.hmRatio, mppFromZoom(dt.key.zoom));
            if (dt.hasMosaic && !dt.mosaicBlob.isEmpty()) {
                auto bytes = MosaicDB::unpackRaw8(dt.mosaicBlob);
                auto& img = tile.getMosaicImageDataRef();
                if (int(bytes.size()) == int(img.size())) {
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
            tile.updateHeightIndices();
            tile.setIsUpdated(false);
            out.insert(dt.key, std::move(tile));
        }

        qDebug() << "LOADED" << zoom << out.size();
        loadingFromDb_ = true;
        emit sendSurfaceTiles(out, true);
        loadingFromDb_ = false;

        currentZoom_ = zoom;
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

    currentZoom_ = zoom;
}
