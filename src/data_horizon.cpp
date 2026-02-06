#include "data_horizon.h"

#include <QDebug>


DataHorizon::DataHorizon() :
    QObject(),
    emitChanges_(true),
    isFileOpening_(false),
    isSeparateReading_(false),
    isAttitudeExpected_(false),
    epochIndx_(0),
    positionIndx_(0),
    chartIndx_(0),
    attitudeIndx_(0),
    artificalAttitudeIndx_(0),
    bottomTrackIndx_(0),
    mosaicIndx_(0),
    sonarPosIndx_(0),
    dimRectIndx_(0)
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
    artificalAttitudeIndx_ = 0;
    bottomTrackIndx_ = 0;
    mosaicIndx_ = 0;
    sonarPosIndx_ = 0;
    dimRectIndx_ = 0;
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
        //emit epochAdded(epochIndx_);
        emit positionAdded(positionIndx_);
        emit chartAdded(chartIndx_);
        //emit attitudeAdded(attitudeIndx_);
        //emit artificalAttitudeAdded(artificalAttitudeIndx_);
        tryCalcAndEmitSonarPosIndx();
        tryCalcAndEmitMosaicIndx();
        tryCalcAndEmitDimRectIndx();
    }
}

void DataHorizon::setIsAttitudeExpected(bool state)
{
    //qDebug() << "DataHorizon::setIsAttitudeExpected" << state;
    isAttitudeExpected_ = state;
}

void DataHorizon::onAddedEpoch(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedEpoch" << indx;

    //bool beenChanged = epochIndx_ != indx;

    epochIndx_ = indx;

    //if (canEmitHorizon(beenChanged)) {
    //    emit epochAdded(epochIndx_);
    //}
}

void DataHorizon::onAddedPosition(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedPosition" << indx;

    bool beenChanged = positionIndx_ != indx;

    positionIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        emit positionAdded(positionIndx_);
        tryCalcAndEmitDimRectIndx();
        tryCalcAndEmitSonarPosIndx();
        tryCalcAndEmitMosaicIndx();
    }
}

void DataHorizon::onAddedChart(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedChart" << indx;

    bool beenChanged = indx != chartIndx_; // TODO: delete this (fix on processing)

    chartIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        emit chartAdded(chartIndx_);
        tryCalcAndEmitDimRectIndx();
        tryCalcAndEmitMosaicIndx();
    }
}

void DataHorizon::onAddedAttitude(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedAttitude" << indx;

    bool beenChanged = attitudeIndx_ != indx;

    attitudeIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        //emit attitudeAdded(attitudeIndx_);
        tryCalcAndEmitSonarPosIndx();
        tryCalcAndEmitDimRectIndx();
        tryCalcAndEmitMosaicIndx();
    }
}

void DataHorizon::onAddedArtificalAttitude(uint64_t indx)
{
    //qDebug() << "DataHorizon::onAddedArtificalAttitude" << indx;

    bool beenChanged = artificalAttitudeIndx_ != indx;

    artificalAttitudeIndx_ = indx;

    if (canEmitHorizon(beenChanged)) {
        //emit artificalAttitudeAdded(artificalAttitudeIndx_);
        tryCalcAndEmitSonarPosIndx();
        tryCalcAndEmitDimRectIndx();
        tryCalcAndEmitMosaicIndx();
    }
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
        //emit bottomTrackAdded(bottomTrackIndx_);
        tryCalcAndEmitMosaicIndx();
    }
}

void DataHorizon::onAddedBottomTrack3D(const QVector<int>& epIndxs, const QVector<int>& vertIndx, bool isManual)
{
    //qDebug() << "DataHorizon::onAddedBottomTrack3D" << epIndxs;

    bool beenChanged = true; // NEED COMPARE?

    if (canEmitHorizon(beenChanged)) {
        emit bottomTrack3DAdded(epIndxs, vertIndx, isManual);
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
        if (!isFileOpening_ && beenChanged) {
            retVal = true;
        }
    }

    return retVal;
}

void DataHorizon::tryCalcAndEmitMosaicIndx()
{
    uint64_t minMosaicHorizon = std::min(std::min(std::min(bottomTrackIndx_, chartIndx_), std::max(attitudeIndx_, artificalAttitudeIndx_)), sonarPosIndx_);
    if (minMosaicHorizon > mosaicIndx_) {
        mosaicIndx_ = minMosaicHorizon;
        emit mosaicCanCalc(mosaicIndx_);
    }
}

void DataHorizon::tryCalcAndEmitSonarPosIndx()
{
    uint64_t minSonarIndx = isAttitudeExpected_ ? std::min(positionIndx_, std::max(attitudeIndx_, artificalAttitudeIndx_)) : positionIndx_;
    if (minSonarIndx > sonarPosIndx_) {
        sonarPosIndx_ = minSonarIndx;
        emit sonarPosCanCalc(sonarPosIndx_);
    }
}

void DataHorizon::tryCalcAndEmitDimRectIndx()
{
    uint64_t minDimRectIndx = sonarPosIndx_;
    if (minDimRectIndx > dimRectIndx_) {
        dimRectIndx_ = minDimRectIndx;
        emit dimRectsCanCalc(dimRectIndx_);
    }
}
