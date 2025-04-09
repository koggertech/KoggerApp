#include "black_stripes_processor.h"


BlackStripesProcessor::BlackStripesProcessor() :
    state_(false),
    forwardSteps_(15),
    backwardSteps_(15)
{}

void BlackStripesProcessor::update(int16_t channelId, Epoch* epoch, Direction direction, float resolution, float offset)
{
    if (!epoch) {
        return;
    }

    const int dataSize = epoch->chartSize(channelId);
    const int lastValidEthalonIndex = getLastValidEthalonIndex(channelId, direction);
    const int newDataSize = (lastValidEthalonIndex >= dataSize) ? lastValidEthalonIndex + 1 : dataSize;
    const bool isForward = direction == Direction::kForward;

    auto& ethalonVector = isForward ? forwardEthalonData_[channelId] : backwardEthalonData_[channelId];
    if (ethalonVector.size() < newDataSize) {
        ethalonVector.resize(newDataSize);
    }

    if (epoch->chartAvail(channelId)) {
        auto& amplitude = epoch->chart(channelId)->amplitude;
        auto chartParameters = epoch->getChartParameters(channelId);

        if (dataSize < newDataSize) {
            chartParameters.errList.append(Segment(dataSize, newDataSize));
            epoch->setChartParameters(channelId, chartParameters);
            amplitude.resize(newDataSize);
        }
    }
    else {
        if (lastValidEthalonIndex == -1) {
            return;
        }

        QVector<uint8_t> data(lastValidEthalonIndex + 1, 0);
        for (int i = 0; i < data.size(); ++i) {
            if (ethalonVector[i].first) {
                data[i] = ethalonVector[i].second;
                --ethalonVector[i].first;
            }
        }

        epoch->setChart(channelId, data, resolution, offset);
        auto chartParameters = epoch->getChartParameters(channelId);
        chartParameters.errList.append(Segment(0, data.size()));
        epoch->setChartParameters(channelId, chartParameters);
    }

    auto& amplitude = epoch->chart(channelId)->amplitude;
    auto chartParameters = epoch->getChartParameters(channelId);

    const auto errorMask = createErrorMask(chartParameters.errList, newDataSize);
    const bool isMaskAvailable = !errorMask.isEmpty();

    for (int i = 0; i < newDataSize; ++i) {
        if (isMaskAvailable && errorMask[i]) {
            if (ethalonVector[i].first) {
                amplitude[i] = ethalonVector[i].second;
                --ethalonVector[i].first;
            }
        }
        else {
            ethalonVector[i] = qMakePair(isForward ? forwardSteps_ : backwardSteps_, amplitude.at(i));
        }
    }

    epoch->setWasValidlyRenderedInEchogram(false);
}

void BlackStripesProcessor::clear()
{
    forwardEthalonData_.clear();
    backwardEthalonData_.clear();
}

void BlackStripesProcessor::clearEthalonData(int channelId, Direction direction)
{
    auto& ethalonData = direction == Direction::kForward ? forwardEthalonData_ : backwardEthalonData_;

    if (ethalonData.contains(channelId)) {
        ethalonData[channelId].clear();
    }
}

void BlackStripesProcessor::tryResizeEthalonData(int channelId, Direction direction, int size)
{
    auto& ethalonData = direction == Direction::kForward ? forwardEthalonData_ : backwardEthalonData_;

    if (ethalonData.contains(channelId)) {
        if (size < ethalonData[channelId].size()) {
            ethalonData[channelId].resize(size);
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

int BlackStripesProcessor::getLastValidEthalonIndex(int16_t channelId, Direction direction) const
{
    int retVal = -1;

    const auto& ethData = direction == Direction::kForward ? forwardEthalonData_ : backwardEthalonData_;

    if (!ethData.contains(channelId)) {
        return retVal;
    }

    auto& ethVec = ethData[channelId];

    for (int i = ethVec.size() - 1; i >= 0; --i) {
        if (ethVec.at(i).first) {
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
