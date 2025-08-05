#include "data_processor.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>
#include "bottom_track.h"
#include "dataset.h"


DataProcessor::DataProcessor(QObject *parent)
    : QObject(parent),
    datasetPtr_(nullptr),
    surfaceMesh_(defaultTileSidePixelSize, defaultTileHeightMatrixRatio, defaultTileResolution),
    bottomTrackProcessor_(this),
    isobathsProcessor_(this),
    mosaicProcessor_(this),
    surfaceProcessor_(this),
    state_(DataProcessorType::kUndefined),
    chartsCounter_(0),
    bottomTrackCounter_(0),
    epochCounter_(0),
    positionCounter_(0),
    attitudeCounter_(0),
    updateBottomTrack_(false),
    updateIsobaths_(false),
    updateMosaic_(false),
    isOpeningFile_(false),
    bottomTrackWindowCounter_(0),
    mosaicCounter_(0),
    tileResolution_(defaultTileResolution)
{
    qRegisterMetaType<BottomTrackParam>("BottomTrackParam");
    qRegisterMetaType<DataProcessorType>("DataProcessorState");
    qRegisterMetaType<QVector<IsobathUtils::LabelParameters>>("QVector<IsobathUtils::LabelParameters>");
    qRegisterMetaType<QHash<QUuid, SurfaceTile>>("QHash<QUuid, SurfaceTile>");

    surfaceProcessor_.setSurfaceMeshPtr(&surfaceMesh_);
    isobathsProcessor_.setSurfaceMeshPtr(&surfaceMesh_);
    mosaicProcessor_.setSurfaceMeshPtr(&surfaceMesh_);
}

DataProcessor::~DataProcessor()
{
}

void DataProcessor::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;

    bottomTrackProcessor_.setDatasetPtr(datasetPtr_);
    mosaicProcessor_.setDatasetPtr(datasetPtr_);
}

void DataProcessor::setBottomTrackPtr(BottomTrack *bottomTrackPtr)
{
    surfaceProcessor_.setBottomTrackPtr(bottomTrackPtr);
}

void DataProcessor::clear(DataProcessorType procType)
{
    switch (procType) {
    case DataProcessorType::kUndefined:   clearAllProcessings();        emit allProcessingCleared();         break;
    case DataProcessorType::kBottomTrack: clearBottomTrackProcessing(); emit bottomTrackProcessingCleared(); break;
    case DataProcessorType::kIsobaths:    clearIsobathsProcessing();    emit isobathsProcessingCleared();    break;
    case DataProcessorType::kMosaic:      clearMosaicProcessing();      emit mosaicProcessingCleared();      break;
    case DataProcessorType::kSurface:     clearSurfaceProcessing();     emit mosaicProcessingCleared();      break;
    default: break;
    }

    // this
    chartsCounter_ = 0;
    bottomTrackCounter_ = 0;
    epochCounter_ = 0;
    positionCounter_ = 0;
    attitudeCounter_ = 0;
    mosaicCounter_ = 0;
}

void DataProcessor::setUpdateBottomTrack(bool state)
{
    //qDebug() << "DataProcessor::setUpdateBottomTrack" << state;

    updateBottomTrack_ = state;
}

void DataProcessor::setUpdateIsobaths(bool state)
{
    //qDebug() << "DataProcessor::setUpdateIsobaths" << state;

    updateIsobaths_ = state;
}

void DataProcessor::setUpdateMosaic(bool state)
{
    //qDebug() << "DataProcessor::setUpdateMosaic" << state;

    updateMosaic_ = state;
}

void DataProcessor::setIsOpeningFile(bool state)
{
    isOpeningFile_ = state;
}

void DataProcessor::onChartsAdded(uint64_t indx)
{
    chartsCounter_ = indx;

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
                bottomTrackProcessor_.bottomTrackProcessing(it->channelId_, ChannelId(), btP);
            }

            bottomTrackWindowCounter_ = currCount;
        }
    }
}

void DataProcessor::onBottomTrackAdded(const QVector<int> &indxs) // indexes from 3D
{
    //qDebug() << "DataProcessor::onUpdatedBottomTrackDataWrapper" << indxs.size();

    if (indxs.empty()) {
        return;
    }

    if (updateIsobaths_ || updateMosaic_) {
        surfaceProcessor_.onUpdatedBottomTrackData(indxs);
    }

    //if (updateIsobaths_) {
    //    doIsobathsWork(indxs, false, false);
    //}

    if (updateMosaic_) {
        mosaicProcessor_.updateDataWrapper(mosaicCounter_, 0);
    }
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

void DataProcessor::onMosaicCanCalc(uint64_t indx)
{
    //qDebug() << "DataProcessor::onMosaicCanCalc" << indx;

    mosaicCounter_ = indx;
}

void DataProcessor::bottomTrackProcessing(const ChannelId &channel1, const ChannelId &channel2, const BottomTrackParam &bottomTrackParam_)
{
    bottomTrackProcessor_.bottomTrackProcessing(channel1, channel2, bottomTrackParam_);
}

void DataProcessor::setSurfaceColorTableThemeById(int id)
{
    //qDebug() << "DataProcessor::setSurfaceColorTableThemeById" << id;

    if (surfaceProcessor_.getThemeId() == id) {
        return;
    }

    surfaceProcessor_.setThemeId(id);

    surfaceProcessor_.rebuildColorIntervals();
}

void DataProcessor::setSurfaceStepSize(float val)
{
    //qDebug() << "DataProcessor::setSurfaceStepSize" << val;

    if (qFuzzyCompare(surfaceProcessor_.getSurfaceStepSize(), val)) {
        return;
    }

    surfaceProcessor_.setSurfaceStepSize(val);
    isobathsProcessor_.setLineStepSize(val);

    surfaceProcessor_.rebuildColorIntervals();
}

void DataProcessor::setIsobathsLineStepSize(float val)
{
    //qDebug() << "DataProcessor::setIsobathsLineStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.getLineStepSize(), val)) { // also for surface
        return;
    }

    surfaceProcessor_.setSurfaceStepSize(val);
    isobathsProcessor_.setLineStepSize(val);

    emit sendSurfaceStepSize(surfaceProcessor_.getSurfaceStepSize());
    emit sendIsobathsLineStepSize(isobathsProcessor_.getLineStepSize());

    doIsobathsWork({}, true, false);
}

void DataProcessor::setIsobathsLabelStepSize(float val)
{
    //qDebug() << "DataProcessor::setIsobathsLabelStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.getLabelStepSize(), val)) {
        return;
    }

    isobathsProcessor_.setLabelStepSize(val);

    doIsobathsWork({}, true, false);
}

void DataProcessor::setSurfaceEdgeLimit(int val)
{
    //qDebug() << "DataProcessor::setIsobathsEdgeLimit" << val;

    auto edgeLimit = static_cast<float>(val);

    if (qFuzzyCompare(surfaceProcessor_.getEdgeLimit(), edgeLimit)) {
        return;
    }

    surfaceProcessor_.setEdgeLimit(edgeLimit);

    doIsobathsWork({}, true, true);
}

void DataProcessor::setMosaicChannels(const ChannelId &ch1, uint8_t sub1, const ChannelId &ch2, uint8_t sub2)
{
    //qDebug() << "DataProcessor::setMosaicChannels" << ch1.toShortName() << sub1 << ch2.toShortName() << sub2;

    mosaicProcessor_.setChannels(ch1, sub1, ch2, sub2);
}

void DataProcessor::setMosaicTheme(int indx)
{
    //qDebug() << "DataProcessor::setMosaicTheme" << indx;

    mosaicProcessor_.setColorTableThemeById(indx);
}

void DataProcessor::setMosaicLAngleOffset(float val)
{
    //qDebug() << "DataProcessor::setMosaicLAngleOffset" << val;

    mosaicProcessor_.setLAngleOffset(val);
}

void DataProcessor::setMosaicRAngleOffset(float val)
{
    //qDebug() << "DataProcessor::setMosaicRAngleOffset" << val;

    mosaicProcessor_.setRAngleOffset(val);
}

void DataProcessor::setMosaicTileResolution(float val)
{
    //qDebug() << "DataProcessor::setMosaicResolution" << val;

    if (qFuzzyIsNull(val)) {
        return;
    }

    const float convertedResolution = 1.0f / val;

    if (qFuzzyCompare(1.0f + tileResolution_, 1.0f + convertedResolution)) {
        return;
    }

    tileResolution_ = convertedResolution;

    surfaceMesh_.reinit(defaultTileSidePixelSize, defaultTileHeightMatrixRatio, tileResolution_);

    surfaceProcessor_.setTileResolution(tileResolution_);
    mosaicProcessor_.setTileResolution(tileResolution_);
}

void DataProcessor::setMosaicLevels(float lowLevel, float highLevel)
{
    //qDebug() << "DataProcessor::setMosaicLevels" << lowLevel << highLevel;

    mosaicProcessor_.setColorTableLevels(lowLevel, highLevel);
}

void DataProcessor::setMosaicLowLevel(float val)
{
    //qDebug() << "DataProcessor::setMosaicLowLevel" << val;

    mosaicProcessor_.setColorTableLowLevel(val);
}

void DataProcessor::setMosaicHighLevel(float val)
{
    //qDebug() << "DataProcessor::setMosaicHighLevel" << val;

    mosaicProcessor_.setColorTableHighLevel(val);
}

void DataProcessor::setMosaicGenerateGridContour(bool state)
{
    //qDebug() << "DataProcessor::setMosaicGenerateGridContour" << state;

    mosaicProcessor_.setGenerateGridContour(state);
}

void DataProcessor::askColorTableForMosaic()
{
    mosaicProcessor_.askColorTableForMosaic();
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
    isobathsProcessor_.clear();
}

void DataProcessor::clearMosaicProcessing()
{
    mosaicProcessor_.clear();
}

void DataProcessor::clearSurfaceProcessing()
{
    surfaceProcessor_.clear();
}

void DataProcessor::clearAllProcessings()
{
    clearBottomTrackProcessing();
    clearIsobathsProcessing();
    clearMosaicProcessing();
    clearSurfaceProcessing();

    surfaceMesh_.clear();
}

void DataProcessor::doIsobathsWork(const QVector<int> &indxs, bool rebuildLinesLabels, bool rebuildAll)
{
    if (rebuildAll) {
        isobathsProcessor_.rebuildTrianglesBuffers();
        isobathsProcessor_.fullRebuildLinesLabels();
        surfaceProcessor_.rebuildColorIntervals();
    }
    else {
        // TODO: now surface
        if (!indxs.isEmpty()) {
            isobathsProcessor_.onUpdatedBottomTrackData(indxs);
        }
        if (rebuildLinesLabels) {
            isobathsProcessor_.fullRebuildLinesLabels();
        }
    }
}
