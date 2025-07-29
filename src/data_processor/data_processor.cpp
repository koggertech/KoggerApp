#include "data_processor.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>
#include "bottom_track.h"
#include "dataset.h"


DataProcessor::DataProcessor(QObject *parent)
    : QObject(parent),
      datasetPtr_(nullptr),
      globalMesh_(256, 16, 0.1f),
      bottomTrackProcessor_(this),
      isobathsProcessor_(this),
      mosaicProcessor_(this),
      surfaceProcessor_(this),
      state_(DataProcessorType::kUndefined),
      chartsCounter_(0),
      bottomTrackCounter_(0),
      epochCounter_(0),
      positionCounter_(0),
      attitudeCounter_(0),
      updateBottomTrack_(false),
      updateIsobaths_(false),
      updateMosaic_(false),
      isOpeningFile_(false),
      bottomTrackWindowCounter_(0)
{
    qRegisterMetaType<BottomTrackParam>("BottomTrackParam");
    qRegisterMetaType<DataProcessorType>("DataProcessorState");
    qRegisterMetaType<QVector<IsobathUtils::LabelParameters>>("QVector<IsobathUtils::LabelParameters>");
    qRegisterMetaType<QHash<QUuid, Tile>>("QHash<QUuid, Tile>");

    mosaicProcessor_.setGlobalMeshPtr(&globalMesh_);
}

DataProcessor::~DataProcessor()
{
}

void DataProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;

    bottomTrackProcessor_.setDatasetPtr(datasetPtr_);
    mosaicProcessor_.setDatasetPtr(datasetPtr_);
    surfaceProcessor_.setDatasetPtr(datasetPtr_);
}

void DataProcessor::setBottomTrackPtr(BottomTrack *bottomTrackPtr) // TODO: using BottomTrack data from this?
{
    isobathsProcessor_.setBottomTrackPtr(bottomTrackPtr);
}

void DataProcessor::clear(DataProcessorType procType)
{
    switch (procType) {
    case DataProcessorType::kUndefined:   clearAllProcessings();        emit allProcessingCleared();         break;
    case DataProcessorType::kBottomTrack: clearBottomTrackProcessing(); emit bottomTrackProcessingCleared(); break;
    case DataProcessorType::kIsobaths:    clearIsobathsProcessing();    emit isobathsProcessingCleared();    break;
    case DataProcessorType::kMosaic:      clearMosaicProcessing();      emit mosaicProcessingCleared();      break;
    default: break;
    }

    // this
    chartsCounter_ = 0;
    bottomTrackCounter_ = 0;
    epochCounter_ = 0;
    positionCounter_ = 0;
    attitudeCounter_ = 0;
}

void DataProcessor::setUpdateBottomTrack(bool state)
{
    //qDebug() << "DataProcessor::setUpdateBottomTrack" << state;

    updateBottomTrack_ = state;
}

void DataProcessor::setUpdateIsobaths(bool state)
{
    //qDebug() << "DataProcessor::setUpdateIsobaths" << state;

    updateIsobaths_ = state;
}

void DataProcessor::setUpdateMosaic(bool state)
{
    //qDebug() << "DataProcessor::setUpdateMosaic" << state;

    updateMosaic_ = state;
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

        const int endIndx    = indx;
        const int windowSize = btP.windowSize;

        int currCount = std::floor(endIndx / windowSize);

        if (bottomTrackWindowCounter_ != currCount) {
            auto additionalBTPGap = windowSize / 2;
            btP.indexFrom = std::max(0, windowSize * bottomTrackWindowCounter_ - (windowSize / 2 + 1) - additionalBTPGap);
            btP.indexTo   = std::max(0, windowSize * currCount - (windowSize / 2 + 1) - additionalBTPGap);

            const auto channels = datasetPtr_->channelsList(); //
            for (auto it = channels.begin(); it != channels.end(); ++it) {
                bottomTrackProcessor_.bottomTrackProcessing(it->channelId_, ChannelId(), btP);
            }

            bottomTrackWindowCounter_ = currCount;
        }
    }
}

void DataProcessor::onBottomTrackAdded(const QVector<int> &indxs)
{    
    // qDebug() << "DataProcessor::onUpdatedBottomTrackDataWrapper" << QThread::currentThreadId() << indxs.size();
    if (indxs.empty()) {
        return;
    }

    bottomTrackCounter_ = indxs.last(); //

    // calc isobaths
    if (updateIsobaths_) {
        enqueueWork(indxs, false, false); // TODO move to this thread
    }

    // test cals mosaic
    if (updateMosaic_) {
        mosaicProcessor_.startUpdateDataInThread(bottomTrackCounter_, 0);
    }
}

void DataProcessor::onEpochAdded(uint64_t indx)
{
    epochCounter_ = indx;
}

void DataProcessor::onPositionAdded(uint64_t indx)
{
    positionCounter_ = indx;
}

void DataProcessor::onAttitudeAdded(uint64_t indx)
{
    attitudeCounter_ = indx;
}

void DataProcessor::bottomTrackProcessing(const ChannelId &channel1, const ChannelId &channel2, const BottomTrackParam &bottomTrackParam_)
{
    bottomTrackProcessor_.bottomTrackProcessing(channel1, channel2, bottomTrackParam_);
}

void DataProcessor::setIsobathsColorTableThemeById(int id)
{
    //qDebug() << "DataProcessor::setColorTableThemeById" << id;

    if (isobathsProcessor_.getThemeId() == id) {
        return;
    }

    isobathsProcessor_.setThemeId(id);

    isobathsProcessor_.rebuildColorIntervals();
}

void DataProcessor::setIsobathsSurfaceStepSize(float val)
{
    //qDebug() << "DataProcessor::setIsobathsSurfaceStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.getSurfaceStepSize(), val)) {
        return;
    }

    isobathsProcessor_.setSurfaceStepSize(val);

    isobathsProcessor_.rebuildColorIntervals();
}

void DataProcessor::setIsobathsLineStepSize(float val)
{
    //qDebug() << "DataProcessor::setIsobathsLineStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.getLineStepSize(), val)) {
        return;
    }

    isobathsProcessor_.setLineStepSize(val);

    emit sendIsobathsLineStepSize(isobathsProcessor_.getLineStepSize());

    enqueueWork({}, true, false);
}

void DataProcessor::setIsobathsLabelStepSize(float val)
{
    //qDebug() << "DataProcessor::setIsobathsLabelStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.getLabelStepSize(), val)) {
        return;
    }

    isobathsProcessor_.setLabelStepSize(val);

    enqueueWork({}, true, false);
}

void DataProcessor::setIsobathsEdgeLimit(int val)
{
    //qDebug() << "DataProcessor::setIsobathsEdgeLimit" << val;

    if (qFuzzyCompare(isobathsProcessor_.getEdgeLimit(), static_cast<float>(val))) {
        return;
    }

    isobathsProcessor_.setEdgeLimit(static_cast<float>(val));

    enqueueWork({}, true, true);
}

void DataProcessor::setMosaicChannels(const ChannelId &ch1, uint8_t sub1, const ChannelId &ch2, uint8_t sub2)
{
    //qDebug() << "DataProcessor::setMosaicChannels" << ch1.toShortName() << sub1 << ch2.toShortName() << sub2;

    mosaicProcessor_.setChannels(ch1, sub1, ch2, sub2);
}

void DataProcessor::setMosaicTheme(int indx)
{
    //qDebug() << "DataProcessor::setMosaicTheme" << indx;

    mosaicProcessor_.setColorTableThemeById(indx);
}

void DataProcessor::setMosaicLAngleOffset(float val)
{
    //qDebug() << "DataProcessor::setMosaicLAngleOffset" << val;

    mosaicProcessor_.setLAngleOffset(val);
}

void DataProcessor::setMosaicRAngleOffset(float val)
{
    //qDebug() << "DataProcessor::setMosaicRAngleOffset" << val;

    mosaicProcessor_.setRAngleOffset(val);
}

void DataProcessor::setMosaicResolution(float val)
{
    //qDebug() << "DataProcessor::setMosaicResolution" << val;

    mosaicProcessor_.setResolution(val);
}

void DataProcessor::setMosaicLevels(float lowLevel, float highLevel)
{
    //qDebug() << "DataProcessor::setMosaicLevels" << lowLevel << highLevel;

    mosaicProcessor_.setColorTableLevels(lowLevel, highLevel);
}

void DataProcessor::setMosaicLowLevel(float val)
{
    //qDebug() << "DataProcessor::setMosaicLowLevel" << val;

    mosaicProcessor_.setColorTableLowLevel(val);
}

void DataProcessor::setMosaicHighLevel(float val)
{
    //qDebug() << "DataProcessor::setMosaicHighLevel" << val;

    mosaicProcessor_.setColorTableHighLevel(val);
}

void DataProcessor::setMosaicGenerateGridContour(bool state)
{
    //qDebug() << "DataProcessor::setMosaicGenerateGridContour" << state;

    mosaicProcessor_.setGenerateGridContour(state);
}

void DataProcessor::askColorTableForMosaic()
{
    mosaicProcessor_.askColorTableForMosaicView();
}

void DataProcessor::handleWorkerFinished()
{
    QMutexLocker lk(&isobathsPendingMtx_);
    if (!isobathsPending_.indxs.isEmpty() || isobathsPending_.rebuildLineLabels) {
        PendingWork copy = isobathsPending_;
        isobathsPending_ = PendingWork{};
        lk.unlock();
        enqueueWork(copy.indxs, copy.rebuildLineLabels, copy.rebuildAll);
    }
}

void DataProcessor::changeState(const DataProcessorType& state)
{
    state_ = state;
    emit sendState(state_);
}

void DataProcessor::clearBottomTrackProcessing()
{
    bottomTrackWindowCounter_ = 0;

    bottomTrackProcessor_.clear();
}

void DataProcessor::clearIsobathsProcessing()
{
    if (isobathsWorkerFuture_.isRunning()) {
        isobathsWorkerFuture_.cancel();
        isobathsWorkerFuture_.waitForFinished();
    }

    {
        QMutexLocker lk(&isobathsPendingMtx_);
        isobathsPending_.clear();
    }

    isobathsProcessor_.clear();
}

void DataProcessor::clearMosaicProcessing()
{
    mosaicProcessor_.clear();

    globalMesh_.clear();
}

void DataProcessor::clearAllProcessings()
{
    clearBottomTrackProcessing();
    clearIsobathsProcessing();
    clearMosaicProcessing();
}

void DataProcessor::enqueueWork(const QVector<int> &indxs, bool rebuildLinesLabels, bool rebuildAll)
{
    {
        QMutexLocker lk(&isobathsPendingMtx_);
        isobathsPending_.indxs += indxs;
        isobathsPending_.rebuildLineLabels |= rebuildLinesLabels;
        isobathsPending_.rebuildAll |= rebuildAll;
    }

    if (isobathsWorkerFuture_.isRunning()) {
        return;
    }

    isobathsWorkerFuture_ = QtConcurrent::run([this] {
        if (isobathsWorkerFuture_.isCanceled()) {
            return;
        }

        PendingWork todo;
        {
            QMutexLocker lk(&isobathsPendingMtx_);
            todo = std::move(isobathsPending_);
            isobathsPending_.clear();
        }

        if (todo.rebuildAll) {
            isobathsProcessor_.rebuildTrianglesBuffers();
            isobathsProcessor_.fullRebuildLinesLabels();
            isobathsProcessor_.rebuildColorIntervals(); //
        }
        else {
            // TODO: more accuracy
            if (!todo.indxs.isEmpty()) {
                isobathsProcessor_.onUpdatedBottomTrackData(todo.indxs);
            }
            if (todo.rebuildLineLabels) {
                isobathsProcessor_.fullRebuildLinesLabels();
            }
        }
    });

    if (!isobathsWorkerWatcher_.isRunning()) {
        connect(&isobathsWorkerWatcher_, &QFutureWatcher<void>::finished, this, &DataProcessor::handleWorkerFinished, Qt::QueuedConnection);
    }

    isobathsWorkerWatcher_.setFuture(isobathsWorkerFuture_);
}
