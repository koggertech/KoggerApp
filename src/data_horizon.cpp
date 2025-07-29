#include "data_horizon.h"

#include <QDebug>


DataHorizon::DataHorizon() :
    QObject(),
    emitChanges_(true),
    isFileOpening_(false),
    isSeparateReading_(false),
    epochIndx_(0),
    positionIndx_(0),
    chartIndx_(0),
    attitudeIndx_(0),
    bottomTrackIndx_(0),
    mosaicIndx_(0)
{
#ifdef SEPARATE_READING
    isSeparateReading_ = true;
#endif

    qRegisterMetaType<uint64_t>("uint64_t");
}

void DataHorizon::clear()
{
    isFileOpening_ = false;

    epochIndx_ = 0;
    positionIndx_ = 0;
    chartIndx_ = 0;
    attitudeIndx_ = 0;
    bottomTrackIndx_ = 0;
    bottomTrack3DIndxs_.clear();
    mosaicIndx_ = 0;
}

void DataHorizon::setEmitChanges(bool state)
{
    emitChanges_ = state;
}

void DataHorizon::setIsFileOpening(bool state)
{
    //qDebug() << "DataHorizon::setIsFileOpening" << state;

    isFileOpening_ = state;

    if (!isFileOpening_ && !isSeparateReading_ && emitChanges_) { // emit all
        emit epochAdded(epochIndx_);
        emit positionAdded(positionIndx_);
        emit chartAdded(chartIndx_);
        emit attitudeAdded(attitudeIndx_);
    }
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

void DataHorizon::onAddedChart(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedChart" << indx;

    bool beenChanged = indx != chartIndx_; // TODO: delete this (fix on processing)

    chartIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        emit chartAdded(chartIndx_);
    }

    tryEmitMosaicIndx();
}

void DataHorizon::onAddedAttitude(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedAttitude" << indx;

    bool beenChanged = attitudeIndx_ != indx;

    attitudeIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        emit attitudeAdded(attitudeIndx_);
    }

    tryEmitMosaicIndx();
}

void DataHorizon::onAddedBottomTrack(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedBottomTrack" << indx;

    if (indx < bottomTrackIndx_) { // discard changes by editing bTr on plot
        return;
    }

    bool beenChanged = bottomTrackIndx_ != indx;

    bottomTrackIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        emit bottomTrackAdded(bottomTrackIndx_);
    }

    tryEmitMosaicIndx();
}

void DataHorizon::onAddedBottomTrack3D(const QVector<int>& indx)
{
    //qDebug() << "DataHorizon::onAddedBottomTrack3D" << indx;

    bool beenChanged = true; //bottomTrackIndxs_ != indx;

    bottomTrack3DIndxs_ = indx;

    if (canEmitHorizon(beenChanged)) {
        emit bottomTrack3DAdded(bottomTrack3DIndxs_);
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

void DataHorizon::tryEmitMosaicIndx()
{
    uint64_t minMosaicHorizon = std::min(std::min(bottomTrackIndx_, chartIndx_), attitudeIndx_);
    if (minMosaicHorizon > mosaicIndx_) {
        mosaicIndx_ = minMosaicHorizon;
        emit mosaicCanCalc(mosaicIndx_);
    }
}
