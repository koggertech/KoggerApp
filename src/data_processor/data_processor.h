#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QVector>
#include <QVector3D>
#include "bottom_track_processor.h"
#include "isobaths_processor.h"
#include "mosaic_processor.h"
#include "surface_processor.h"


enum class DataProcessorType {
    kUndefined = 0,
    kBottomTrack,
    kIsobaths,
    kMosaic
};

class Dataset;
class DataProcessor : public QObject
{
    Q_OBJECT
public:
    explicit DataProcessor(QObject* parent = nullptr);
    ~DataProcessor() override;

    void setDatasetPtr(Dataset* datasetPtr);

public slots:
    // this
    void setBottomTrackPtr(BottomTrack* bottomTrackPtr);
    void clear(DataProcessorType = DataProcessorType::kUndefined);
    void setUpdateBottomTrack (bool state);
    void setUpdateIsobaths (bool state);
    void setUpdateMosaic (bool state);
    void setIsOpeningFile (bool state);
    // from DataHorizon
    void onChartsAdded(const ChannelId& channelId, uint64_t indx); // external calling realtime
    void onBottomTrackAdded(const QVector<int>& indxs);
    void onEpochAdded(uint64_t indx);
    void onPositionAdded(uint64_t indx);
    void onAttitudeAdded(uint64_t indx);
    // BottomTrackProcessor
    void bottomTrackProcessing(const ChannelId& channel1, const ChannelId& channel2, const BottomTrackParam& bottomTrackParam_); // CALC BOTTOM TRACK BY BUTTON
    // IsobathsProcessor
    void setIsobathsColorTableThemeById(int id);
    void setIsobathsSurfaceStepSize(float val);
    void setIsobathsLineStepSize(float val);
    void setIsobathsLabelStepSize(float val);
    void setIsobathsEdgeLimit(int val);

signals:
    // this
    void sendState(DataProcessorType state);
    void bottomTrackProcessingCleared();
    void isobathsProcessingCleared();
    void mosaicProcessingCleared();
    void allProcessingCleared();
    // BottomTrackProcessor
    void distCompletedByProcessing(int epIndx, const ChannelId& channelId, float dist);
    void lastBottomTrackEpochChanged(const ChannelId& channelId, int val, const BottomTrackParam& btP);
    // IsobathsProcessor
    void sendIsobathsLabels(const QVector<IsobathUtils::LabelParameters>& labels);
    void sendIsobathsLineSegments(const QVector<QVector3D>& lineSegments);
    void sendIsobathsPts(const QVector<QVector3D>& pts);
    void sendIsobathsEdgePts(const QVector<QVector3D>& edgePts);
    void sendIsobathsMinZ(float minZ);
    void sendIsobathsMaxZ(float maxZ);
    void sendIsobathsLevelStep(float levelStep);
    void sendIsobathsLineStepSize(float lineStepSize);
    void sendIsobathsTextureTask(const QVector<uint8_t>& textureTask);
    void sendIsobathsColorIntervalsSize(int size);

private slots:
    // IsobathsProcessor
    void handleWorkerFinished();

private:
    // this
    void changeState(const DataProcessorType& state);
    void clearBottomTrackProcessing();
    void clearIsobathsProcessing();
    void clearMosaicProcessing();
    void clearAllProcessings();
    // IsobathsProcessor
    void enqueueWork(const QVector<int>& indxs, bool rebuildLinesLabels, bool rebuildAll);

private:
    friend class BottomTrackProcessor;
    friend class IsobathsProcessor;
    // this
    Dataset* datasetPtr_;
    BottomTrackProcessor bottomTrackProcessor_;
    IsobathsProcessor isobathsProcessor_;
    //MosaicProcessor mosaicProcessor_;
    SurfaceProcessor surfaceProcessor_;
    QHash<ChannelId, uint64_t> chartsCounter_;
    DataProcessorType state_;
    uint64_t bottomTrackCounter_;
    uint64_t epochCounter_;
    uint64_t positionCounter_;
    uint64_t attitudeCounter_;
    bool updateBottomTrack_;
    bool updateIsobaths_;
    bool updateMosaic_;
    bool isOpeningFile_;
    // BottomTrackProcessor
    int bottomTrackWindowCounter_;
    // IsobathsProcessor
    QFuture<void> isobathsWorkerFuture_;
    QFutureWatcher<void> isobathsWorkerWatcher_;
    QMutex isobathsPendingMtx_;
    QVector<int> isobathsPendingIndxs_;
    PendingWork isobathsPending_;
};
