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

    pendingBtTimer_.setParent(this);
    pendingBtTimer_.setSingleShot(true);
    pendingBtTimer_.setInterval(10);
    connect(&pendingBtTimer_, &QTimer::timeout, this, &DataProcessor::flushPendingWork);
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
    pendingBtIndxs_.clear();
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

            const auto channels = datasetPtr_->channelsList();
            for (auto it = channels.begin(); it != channels.end(); ++it) {
                bottomTrackProcessor_.bottomTrackProcessing(it->channelId_, ChannelId(), btP);
            }

            bottomTrackWindowCounter_ = currCount;
        }
    }
}

void DataProcessor::onBottomTrackAdded(const QVector<int> &indxs) // indexes from 3D (conn,open file, edit echo)
{
    //qDebug() << "DataProcessor::onUpdatedBottomTrackDataWrapper" << indxs.size();

    if (indxs.empty()) {
        return;
    }

    for (int v : indxs) {
        pendingBtIndxs_.insert(v);
    }

    if (!pendingBtTimer_.isActive()) {
        pendingBtTimer_.start();
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

    if (updateIsobaths_ || updateMosaic_) {
        surfaceProcessor_.rebuildColorIntervals();
    }
}

void DataProcessor::setIsobathsLabelStepSize(float val)
{
    //qDebug() << "DataProcessor::setIsobathsLabelStepSize" << val;

    if (qFuzzyCompare(isobathsProcessor_.getLabelStepSize(), val)) {
        return;
    }

    isobathsProcessor_.setLabelStepSize(val);

    if (updateIsobaths_) {
        isobathsProcessor_.onUpdatedBottomTrackData(); // full rebuild
    }
}

void DataProcessor::setSurfaceIsobathsStepSize(float val)
{
    if (!qFuzzyCompare(surfaceProcessor_.getSurfaceStepSize(), val)) {
        surfaceProcessor_.setSurfaceStepSize(val);
        emit sendSurfaceStepSize(val);
        if (updateIsobaths_ || updateMosaic_) {
            surfaceProcessor_.rebuildColorIntervals();
        }
    }

    if (!qFuzzyCompare(isobathsProcessor_.getLineStepSize(), val)) {
        isobathsProcessor_.setLineStepSize(val);
        emit sendIsobathsLineStepSize(val);
        if (updateIsobaths_ || updateMosaic_) {
            isobathsProcessor_.onUpdatedBottomTrackData();
        }
    }
}

void DataProcessor::setSurfaceEdgeLimit(int val)
{
    //qDebug() << "DataProcessor::setIsobathsEdgeLimit" << val;

    auto edgeLimit = static_cast<float>(val);

    if (qFuzzyCompare(surfaceProcessor_.getEdgeLimit(), edgeLimit)) {
        return;
    }

    surfaceProcessor_.setEdgeLimit(edgeLimit); // тот же расчет

    if (updateMosaic_) {
        mosaicProcessor_.clear();
        mosaicProcessor_.updateDataWrapper(mosaicCounter_, 0);
    }

    if (updateIsobaths_) {
        isobathsProcessor_.onUpdatedBottomTrackData(); // full rebuild
    }
}

void DataProcessor::setMosaicChannels(const ChannelId &ch1, uint8_t sub1, const ChannelId &ch2, uint8_t sub2)
{
    //qDebug() << "DataProcessor::setMosaicChannels" << ch1.toShortName() << sub1 << ch2.toShortName() << sub2;

    mosaicProcessor_.setChannels(ch1, sub1, ch2, sub2);

    // do calc?
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

    emit surfaceProcessingCleared();
    emit mosaicProcessingCleared(); //

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

void DataProcessor::askColorTableForMosaic()
{
    mosaicProcessor_.askColorTableForMosaic();
}

void DataProcessor::setMinZ(float minZ)
{
    isobathsProcessor_.setMinZ(minZ);
}

void DataProcessor::setMaxZ(float maxZ)
{
    isobathsProcessor_.setMaxZ(maxZ);
}

void DataProcessor::flushPendingWork()
{
    if (pendingBtIndxs_.isEmpty()) {
        return;
    }

    QVector<int> vec;
    vec.reserve(pendingBtIndxs_.size());
    for (auto it = pendingBtIndxs_.cbegin(); it != pendingBtIndxs_.cend(); ++it) {
        vec.append(*it);
    }

    pendingBtIndxs_.clear();

    std::sort(vec.begin(), vec.end());

    if (updateIsobaths_ || updateMosaic_) {
        surfaceProcessor_.onUpdatedBottomTrackData(vec);
    }

    if (updateMosaic_) {
        mosaicProcessor_.updateDataWrapper(mosaicCounter_, 0);
    }

    if (updateIsobaths_) {
        isobathsProcessor_.onUpdatedBottomTrackData(); // full rebuild
    }
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
