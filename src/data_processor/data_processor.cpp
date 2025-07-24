#include "data_processor.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>

#include "bottom_track.h"


DataProcessor::DataProcessor(QObject *parent)
    : QObject(parent),
      datasetPtr_(nullptr),
      bottomTrackProcessor_(this),
      isobathsProcessor_(this),
      state_(DataProcessorType::kUndefined),
      updateBottomTrack_(false),
      updateIsobaths_(false),
      updateMosaic_(false),
      isOpeningFile_(false),
      bottomTrackCounter_(0),
      epochCounter_(0),
      positionCounter_(0),
      attitudeCounter_(0),
      bottomTrackWindowCounter_(0)
{


    //qDebug() << "DataProcessor ctr" << QThread::currentThreadId();

    qRegisterMetaType<BottomTrackParam>("BottomTrackParam");
    qRegisterMetaType<DataProcessorType>("DataProcessorState");
    qRegisterMetaType<QVector<LabelParameters>>("QVector<LabelParameters>");
}

DataProcessor::~DataProcessor()
{
    //qDebug() << "DataProcessor dtr" << QThread::currentThreadId();
}

void DataProcessor::clear(DataProcessorType procType)
{
    qDebug() << "DataProcessor::clear" << static_cast<int>(procType);

    switch (procType) {
    case DataProcessorType::kUndefined:   clearAllProcessings();        emit allProcessingCleared();         break;
    case DataProcessorType::kBottomTrack: clearBottomTrackProcessing(); emit bottomTrackProcessingCleared(); break;
    case DataProcessorType::kIsobaths:    clearIsobathsProcessing();    emit isobathsProcessingCleared();    break;
    case DataProcessorType::kMosaic:      clearMosaicProcessing();      emit mosaicProcessingCleared();      break;
    default: break;
    }
}

void DataProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;

    bottomTrackProcessor_.setDatasetPtr(datasetPtr_);
    isobathsProcessor_.setDatasetPtr(datasetPtr_);
}

void DataProcessor::setBottomTrackPtr(BottomTrack *bottomTrackPtr) // TODO: using BottomTrack data from this?
{
    isobathsProcessor_.setBottomTrackPtr(bottomTrackPtr);
}

void DataProcessor::bottomTrackProcessing(const ChannelId &channel1, const ChannelId &channel2, const BottomTrackParam &bottomTrackParam_)
{
    bottomTrackProcessor_.bottomTrackProcessing(channel1, channel2, bottomTrackParam_);
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
    if (workerFuture_.isRunning()) {
        workerFuture_.cancel();
        workerFuture_.waitForFinished();
    }

    {
        QMutexLocker lk(&pendingMtx_);
        pending_.clear();
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

void DataProcessor::setColorTableThemeById(int id)
{
    qDebug() << "DataProcessor::setColorTableThemeById" << id;

    if (isobathsProcessor_.themeId_ == id) {
        return;
    }

    isobathsProcessor_.themeId_ = id;

    isobathsProcessor_.rebuildColorIntervals();
}

void DataProcessor::setSurfaceStepSize(float val)
{
    qDebug() << "DataProcessor::setSurfaceStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.surfaceStepSize_, val)) {
        return;
    }

    isobathsProcessor_.surfaceStepSize_ = val;

    isobathsProcessor_.rebuildColorIntervals();
}

void DataProcessor::setLineStepSize(float val)
{
    qDebug() << "DataProcessor::setLineStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.lineStepSize_, val)) {
        return;
    }

    isobathsProcessor_.lineStepSize_ = val;

    emit sendIsobathsLineStepSize(isobathsProcessor_.lineStepSize_);

    enqueueWork({}, true, false);
}

void DataProcessor::setLabelStepSize(float val)
{
    qDebug() << "DataProcessor::setLabelStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.labelStepSize_, val)) {
        return;
    }

    isobathsProcessor_.labelStepSize_ = val;

    enqueueWork({}, true, false);
}

void DataProcessor::setEdgeLimit(int val)
{
    qDebug() << "DataProcessor::setEdgeLimit" << val;

    if (isobathsProcessor_.edgeLimit_ == val) {
        return;
    }

    isobathsProcessor_.edgeLimit_ = val;

    enqueueWork({}, true, true);
}

void DataProcessor::handleWorkerFinished()
{
    QMutexLocker lk(&pendingMtx_);
    if (!pending_.indxs.isEmpty() || pending_.rebuildLineLabels) {
        PendingWork copy = pending_;
        pending_ = PendingWork{};
        lk.unlock();
        enqueueWork(copy.indxs, copy.rebuildLineLabels, copy.rebuildAll);
    }
}

void DataProcessor::enqueueWork(const QVector<int> &indxs, bool rebuildLinesLabels, bool rebuildAll)
{
    {
        QMutexLocker lk(&pendingMtx_);
        pending_.indxs += indxs;
        pending_.rebuildLineLabels |= rebuildLinesLabels;
        pending_.rebuildAll |= rebuildAll;
    }

    if (workerFuture_.isRunning()) {
        return;
    }

    workerFuture_ = QtConcurrent::run([this] {
        if (workerFuture_.isCanceled()) {
            return;
        }

        PendingWork todo;
        {
            QMutexLocker lk(&pendingMtx_);
            todo = std::move(pending_);
            pending_.clear();
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

    if (!workerWatcher_.isRunning()) {
        connect(&workerWatcher_, &QFutureWatcher<void>::finished, this, &DataProcessor::handleWorkerFinished, Qt::QueuedConnection);
    }

    workerWatcher_.setFuture(workerFuture_);
}
