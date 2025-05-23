#include "black_stripes_processor.h"


BlackStripesProcessor::BlackStripesProcessor() :
    state_(false),
    forwardSteps_(15),
    backwardSteps_(15)
{}

bool BlackStripesProcessor::update(const ChannelId& channelId, Epoch* epoch, Direction direction, float resolution, float offset)
{
    bool beenUpdated = false;

    if (!epoch) {
        return beenUpdated;
    }

    if (!epoch->getChartsSizeByChannelId(channelId)) {
        auto& data = direction == Direction::kForward ? forwardEthalonData_ : backwardEthalonData_;
        QVector<VecCntAndBrightness>* allDirData = nullptr;
        if (data.contains(channelId)) {
            allDirData = &data[channelId];
        }
        if (allDirData) {
            int newNumSubCh = allDirData->size();
            QVector<QVector<uint8_t>> data(newNumSubCh);
            epoch->setChart(channelId, data, resolution, offset);
        }
    }

    uint8_t chartSize = epoch->getChartsSizeByChannelId(channelId);

    for (uint8_t subChannelId = 0; subChannelId < chartSize; ++subChannelId) {
        const int dataSize = epoch->chartSize(channelId, subChannelId);
        const int lastValidEthalonIndex = getLastValidEthalonIndex(channelId, subChannelId, direction);
        const int newDataSize = (lastValidEthalonIndex >= dataSize) ? lastValidEthalonIndex + 1 : dataSize;
        const bool isForward = direction == Direction::kForward;

        auto& allForwardEthalonDataByChannelId = forwardEthalonData_[channelId];
        auto& allBackwardEthalonDataByChannelId = backwardEthalonData_[channelId];
        if (isForward) {
            if (subChannelId >= allForwardEthalonDataByChannelId.size()) {
                allForwardEthalonDataByChannelId.resize(subChannelId + 1);
            }
        }
        else {
            if (subChannelId >= allBackwardEthalonDataByChannelId.size()) {
                allBackwardEthalonDataByChannelId.resize(subChannelId + 1);
            }
        }

        auto& ethalonVector = isForward ? allForwardEthalonDataByChannelId[subChannelId] : allBackwardEthalonDataByChannelId[subChannelId];
        if (ethalonVector.size() < newDataSize) {
            ethalonVector.resize(newDataSize);
        }

        if (epoch->chartAvail(channelId, subChannelId)) {
            auto& amplitude = epoch->chart(channelId, subChannelId)->amplitude;
            auto chartParameters = epoch->getChartParameters(channelId);

            if (dataSize < newDataSize) {
                chartParameters.errList.append(Segment(dataSize, newDataSize));
                epoch->setChartParameters(channelId, chartParameters);
                amplitude.resize(newDataSize);
                beenUpdated = true;
            }
        }
        else {
            if (lastValidEthalonIndex == -1) {
                continue;
            }

            QVector<uint8_t> data(lastValidEthalonIndex + 1, 0);
            for (int i = 0; i < data.size(); ++i) {
                if (ethalonVector[i].first) {
                    data[i] = ethalonVector[i].second;
                    --ethalonVector[i].first;
                }
            }

            epoch->setChartBySubChannelId(channelId, subChannelId, data, resolution, offset);
            auto chartParameters = epoch->getChartParameters(channelId);
            chartParameters.errList.append(Segment(0, data.size()));
            epoch->setChartParameters(channelId, chartParameters);
            beenUpdated = true;
        }

        auto& amplitude = epoch->chart(channelId, subChannelId)->amplitude;
        auto chartParameters = epoch->getChartParameters(channelId);

        const auto errorMask = createErrorMask(chartParameters.errList, newDataSize);
        const bool isMaskAvailable = !errorMask.isEmpty();

        for (int i = 0; i < newDataSize; ++i) {
            if (isMaskAvailable && errorMask[i]) {
                if (ethalonVector[i].first) {
                    beenUpdated = true;
                    amplitude[i] = ethalonVector[i].second;
                    --ethalonVector[i].first;
                }
            }
            else {
                ethalonVector[i] = qMakePair(isForward ? forwardSteps_ : backwardSteps_, amplitude.at(i));
            }
        }
    }

    return beenUpdated;
}

void BlackStripesProcessor::clear()
{
    forwardEthalonData_.clear();
    backwardEthalonData_.clear();
}

void BlackStripesProcessor::clearEthalonData(const ChannelId& channelId, Direction direction)
{
    auto& ethalonData = direction == Direction::kForward ? forwardEthalonData_ : backwardEthalonData_;

    if (ethalonData.contains(channelId)) {
        ethalonData[channelId].clear();
    }
}

void BlackStripesProcessor::tryResizeEthalonData(const ChannelId& channelId, uint8_t numSubChannels, Direction direction, int size)
{
    auto& ethalonData = direction == Direction::kForward ? forwardEthalonData_ : backwardEthalonData_;

    if (ethalonData.contains(channelId)) {
        auto& allChannelData = ethalonData[channelId];

        if (allChannelData.size() < numSubChannels) {
            allChannelData.resize(numSubChannels);
        }

        for (auto& iChannelData : allChannelData) {
            if (size < iChannelData.size()) {
                iChannelData.resize(size);
            }
        }
    }
}

void BlackStripesProcessor::setState(bool state)
{
    state_ = state;
}

void BlackStripesProcessor::setForwardSteps(int val)
{
    forwardSteps_ = val;
}

void BlackStripesProcessor::setBackwardSteps(int val)
{
    backwardSteps_ = val;
}

bool BlackStripesProcessor::getState() const
{
    return state_;
}

int BlackStripesProcessor::getForwardSteps() const
{
    return forwardSteps_;
}

int BlackStripesProcessor::getBackwardSteps() const
{
    return backwardSteps_;
}

int BlackStripesProcessor::getLastValidEthalonIndex(const ChannelId& channelId, uint8_t subChannelId, Direction direction) const
{
    int retVal = -1;

    const auto& ethData = direction == Direction::kForward ? forwardEthalonData_ : backwardEthalonData_;

    if (!ethData.contains(channelId)) {
        return retVal;
    }

    auto& allChannelData = ethData[channelId];


    if (subChannelId >= allChannelData.size()) {
        return retVal;
    }

    auto& selectedChannelData = allChannelData[subChannelId];
    for (int i = selectedChannelData.size() - 1; i >= 0; --i) {
        if (selectedChannelData.at(i).first) {
            return i;
        }
    }

    return retVal;
}

QVector<uint8_t> BlackStripesProcessor::createErrorMask(const QList<Segment>& errList, int dataSize) const
{
    if (dataSize <= 0) {
        return {};
    }

    QVector<uint8_t> retVal(dataSize, 0);

    for (const auto& seg : errList) {
        const int start = std::max(static_cast<int>(seg.first), 0);
        const int end   = std::min(static_cast<int>(seg.second), dataSize);
        if (start > dataSize ||
            end > dataSize) {
            return {};
        }

        for (int i = start; i < end; ++i) {
            retVal[i] = 1;
        }
    }

    return retVal;
}
