#include "data_processor.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>
#include "bottom_track.h"
#include "dataset.h"


DataProcessor::DataProcessor(QObject *parent)
    : QObject(parent),
      datasetPtr_(nullptr),
      bottomTrackProcessor_(this),
      isobathsProcessor_(this),
      mosaicProcessor_(this),
      surfaceProcessor_(this),
      state_(DataProcessorType::kUndefined),
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
    chartsCounter_.clear();
    bottomTrackCounter_ = 0;
    epochCounter_ = 0;
    positionCounter_ = 0;
    attitudeCounter_ = 0;
}

void DataProcessor::setUpdateBottomTrack(bool state)
{
    updateBottomTrack_ = state;
}

void DataProcessor::setUpdateIsobaths(bool state)
{
    updateIsobaths_ = state;
}

void DataProcessor::setUpdateMosaic(bool state)
{
    updateMosaic_ = state;
}

void DataProcessor::setIsOpeningFile(bool state)
{
    isOpeningFile_ = state;
}

void DataProcessor::onChartsAdded(const ChannelId& channelId, uint64_t indx)
{
    Q_UNUSED(channelId);

    chartsCounter_[channelId] = indx;

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
              //  qDebug() << it->channelId_.isValid();
                bottomTrackProcessor_.bottomTrackProcessing(it->channelId_, ChannelId(), btP);
            }
            //bottomTrackProcessor_.bottomTrackProcessing(channelId, ChannelId(), btP); // TODO: fix

            bottomTrackWindowCounter_ = currCount;
        }
    }
}

void DataProcessor::onBottomTrackAdded(const QVector<int> &indxs)
{    
    //qDebug() << "DataProcessor::onUpdatedBottomTrackDataWrapper" << QThread::currentThreadId();
    //qDebug() << indxs.size();

    if (indxs.empty()) {
        return;
    }

    bottomTrackCounter_ = indxs.last();

    if (!updateIsobaths_ && !updateMosaic_) {
        return;
    }

    enqueueWork(indxs, false, false);
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

void DataProcessor::setColorTableThemeById(int id)
{
    //qDebug() << "DataProcessor::setColorTableThemeById" << id;

    if (isobathsProcessor_.getThemeId() == id) {
        return;
    }

    isobathsProcessor_.setThemeId(id);

    isobathsProcessor_.rebuildColorIntervals();
}

void DataProcessor::setSurfaceStepSize(float val)
{
    //qDebug() << "DataProcessor::setSurfaceStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.getSurfaceStepSize(), val)) {
        return;
    }

    isobathsProcessor_.setSurfaceStepSize(val);

    isobathsProcessor_.rebuildColorIntervals();
}

void DataProcessor::setLineStepSize(float val)
{
    //qDebug() << "DataProcessor::setLineStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.getLineStepSize(), val)) {
        return;
    }

    isobathsProcessor_.setLineStepSize(val);

    emit sendIsobathsLineStepSize(isobathsProcessor_.getLineStepSize());

    enqueueWork({}, true, false);
}

void DataProcessor::setLabelStepSize(float val)
{
    //qDebug() << "DataProcessor::setLabelStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.getLabelStepSize(), val)) {
        return;
    }

    isobathsProcessor_.setLabelStepSize(val);

    enqueueWork({}, true, false);
}

void DataProcessor::setEdgeLimit(int val)
{
    //qDebug() << "DataProcessor::setEdgeLimit" << val;

    if (qFuzzyCompare(isobathsProcessor_.getEdgeLimit(), static_cast<float>(val))) {
        return;
    }

    isobathsProcessor_.setEdgeLimit(static_cast<float>(val));

    enqueueWork({}, true, true);
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
