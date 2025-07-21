#include "data_processor.h"

#include <QDebug>
#include <QThread>


DataProcessor::DataProcessor(QObject *parent) :
    QObject(parent),
    bottomTrackProcessor_(this)
{
    qRegisterMetaType<BottomTrackParam>("BottomTrackParam");
    qRegisterMetaType<DataProcessorState>("DataProcessorState");

    //qDebug() << "DataProcessor ctr" << QThread::currentThreadId();
}

DataProcessor::~DataProcessor()
{
    //qDebug() << "DataProcessor dtr" << QThread::currentThreadId();
}

void DataProcessor::clear()
{
    bottomTrackWindowCounter_ = 0;
}

void DataProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;

    bottomTrackProcessor_.setDatasetPtr(datasetPtr_);
}

void DataProcessor::bottomTrackProcessing(const ChannelId &channel1, const ChannelId &channel2, const BottomTrackParam &bottomTrackParam_)
{
    bottomTrackProcessor_.bottomTrackProcessing(channel1, channel2, bottomTrackParam_);
}

void DataProcessor::onChartsAdded(const ChannelId& channelId, uint64_t indx)
{
    Q_UNUSED(channelId);

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

void DataProcessor::changeState(const DataProcessorState& state)
{
    state_ = state;
    emit sendState(state_);
}
