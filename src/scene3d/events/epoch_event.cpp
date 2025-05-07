#include "epoch_event.h"

EpochEvent::EpochEvent(Type eventType,
                       Epoch *epoch,
                       int epochIndex,
                       const DatasetChannel& channelId)
    : QEvent(eventType)
    , m_epoch(epoch)
    , m_epochIndex(epochIndex)
    , m_channelId(-1)
    , m_channel(channelId)
{}

Epoch *EpochEvent::epoch() const
{
    return m_epoch;
}

int EpochEvent::epochIndex() const
{
    return m_epochIndex;
}

DatasetChannel EpochEvent::channel() const
{
    return m_channel;
}

bool EpochEvent::isValid() const
{
    return m_epoch != nullptr;
}

int EpochEvent::getChannelId() const
{
    return m_channelId;
}
