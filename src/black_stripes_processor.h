#pragma once

#include <stdint.h>
#include <QMap>
#include <QPair>
#include <QVector>

#include "dataset.h"

using VecCntAndBrightness = QVector<QPair<uint8_t, uint8_t>>; // first - counter, second - brightness

class BlackStripesProcessor
{
public:
    enum class Direction {
        kForward = 0,
        kBackward
    };

    BlackStripesProcessor();

    bool update(const ChannelId& channelId, Epoch* epoch, Direction direction, float resolution, float offset);
    void clear();
    void clearEthalonData(const ChannelId& channelId, Direction direction);
    void tryResizeEthalonData(const ChannelId& channelId, uint8_t numSubChannels, Direction direction, int size);

    void setState(bool state);
    void setForwardSteps(int val);
    void setBackwardSteps(int val);
    bool getState() const;
    int  getForwardSteps() const;
    int  getBackwardSteps() const;

private:
    // methods
    int getLastValidEthalonIndex(const ChannelId& channelId, uint8_t subChannelId, Direction direction) const;
    QVector<uint8_t> createErrorMask(const QList<Segment>& errList, int dataSize) const;

    // data
    QMap<ChannelId, QVector<VecCntAndBrightness>> forwardEthalonData_; // QVector - data for subchannels
    QMap<ChannelId, QVector<VecCntAndBrightness>> backwardEthalonData_;
    bool state_;
    int forwardSteps_;
    int backwardSteps_;
};
