#include "data_horizon.h"

#include <QDebug>


DataHorizon::DataHorizon() :
    QObject(),
    emitChanges_(true),
    isFileOpening_(false),
    isSeparateReading_(false),
    epochIndx_(0),
    positionIndx_(0),
    chartIndx_(std::make_pair(ChannelId(), 0)),
    attitudeIndx_(0)
{
#ifdef SEPARATE_READING
    isSeparateReading_ = true;
#endif

    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<ChannelId>("ChannelId");
}

void DataHorizon::clear()
{
    isFileOpening_ = false;

    epochIndx_ = 0;
    positionIndx_ = 0;
    chartIndx_ = std::make_pair(ChannelId(), 0);
    attitudeIndx_ = 0;
}

void DataHorizon::setEmitChanges(bool state)
{
    emitChanges_ = state;
}

void DataHorizon::setIsFileOpening(bool state)
{
    //qDebug() << "DataHorizon::setIsFileOpening" << state;

    isFileOpening_ = state;
}

void DataHorizon::onAddedEpoch(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedEpoch" << indx;

    bool beenChanged = epochIndx_ != indx;

    epochIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        emit epochAdded(epochIndx_);
    }
}

void DataHorizon::onAddedPosition(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedPosition" << indx;

    bool beenChanged = positionIndx_ != indx;

    positionIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        emit positionAdded(positionIndx_);
    }
}

void DataHorizon::onAddedChart(const ChannelId& channelId, uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedChart" << indx << channelId.toShortName();

    bool beenChanged = indx != chartIndx_.second; // TODO: delete this (fix on processing)

    chartIndx_ = std::make_pair(channelId, indx);

    if (canEmitHorizon(beenChanged)) {
        emit chartAdded(chartIndx_.first, chartIndx_.second);
    }
}

void DataHorizon::onAddedAttitude(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedAttitude" << indx;

    bool beenChanged = attitudeIndx_ != indx;

    attitudeIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        emit attitudeAdded(attitudeIndx_);
    }
}

bool DataHorizon::canEmitHorizon(bool beenChanged) const
{
    bool retVal = false;

    if (!emitChanges_) {
        return retVal;
    }

    if (isSeparateReading_) {
        if (beenChanged) {
            retVal = true;
        }
    }
    else {
        if(!isFileOpening_ && beenChanged) {
            retVal = true;
        }
    }

    return retVal;
}
