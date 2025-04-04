#pragma once

#include <stdint.h>
#include <QMap>
#include <QPair>
#include <QVector>

#include "plotcash.h"


class BlackStripesProcessor
{
public:
    enum class Direction {
        kForward = 0,
        kBackward
    };

    BlackStripesProcessor();

    void update(int16_t channelId, Epoch* epoch, Direction direction, float resolution, float offset);
    void clear();
    void clearEthalonData(int channelId, Direction direction);
    void tryResizeEthalonData(int channelId, Direction direction, int size);

    void setState(bool state);
    void setForwardSteps(int val);
    void setBackwardSteps(int val);
    bool getState() const;
    int  getForwardSteps() const;
    int  getBackwardSteps() const;

private:
    /*methods*/
    int getLastValidEthalonIndex(int16_t channelId, Direction direction) const;
    QVector<uint8_t> createErrorMask(const QList<Segment>& errList, int dataSize) const;

    /*data*/
    QMap<int16_t, QVector<QPair<uint8_t, uint8_t>>> forwardEthalonData_;
    QMap<int16_t, QVector<QPair<uint8_t, uint8_t>>> backwardEthalonData_;
    bool state_;
    int forwardSteps_;
    int backwardSteps_;
};
