#pragma once

#include <atomic>
#include <QObject>
#include <QTimer>
#include <QSet>
#include <QVector>
#include <QVector3D>
#include <QPair>
#include <QThread>
#include <QUuid>
#include "dataset_defs.h"
#include "bottom_track_processor.h"
#include "isobaths_processor.h"
#include "mosaic_processor.h"
#include "surface_processor.h"


enum class DataProcessorType {
    kUndefined = 0,
    kBottomTrack,
    kIsobaths,
    kMosaic,
    kSurface
};

enum WorkFlag : quint32 {
    WF_None     = 0,
    WF_Surface  = 1u << 0,
    WF_Mosaic   = 1u << 1,
    WF_Isobaths = 1u << 2,
    WF_All      = WF_Surface | WF_Mosaic | WF_Isobaths
};
Q_DECLARE_FLAGS(WorkSet, WorkFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(WorkSet)

class Dataset;
class BottomTrack;
class ComputeWorker;
class DataProcessor : public QObject {
    Q_OBJECT
public:
    explicit DataProcessor(QObject* parent = nullptr, Dataset* datasetPtr = nullptr);
    ~DataProcessor() override;

    void setDatasetPtr(Dataset* datasetPtr);
    inline bool isCancelRequested() const noexcept { return cancelRequested_.load(); }

public slots:
    // this
    void setBottomTrackPtr(BottomTrack* bottomTrackPtr);
    void clearProcessing(DataProcessorType = DataProcessorType::kUndefined);
    void setUpdateBottomTrack (bool state);
    void setUpdateIsobaths (bool state);
    void setUpdateMosaic (bool state);
    void setIsOpeningFile (bool state);
    // from DataHorizon
    void onChartsAdded(uint64_t indx); // external calling realtime
    void onBottomTrack3DAdded(const QVector<int>& epIndxs, const QVector<int> &vertIndxs, bool manual);
    void onEpochAdded(uint64_t indx);
    void onPositionAdded(uint64_t indx);
    void onAttitudeAdded(uint64_t indx);
    void onMosaicCanCalc(uint64_t indx);
    // BottomTrackProcessor
    void bottomTrackProcessing(const DatasetChannel& channel1, const DatasetChannel& channel2, const BottomTrackParam& bottomTrackParam, bool manual, bool redrawAll); // CALC BOTTOM TRACK BY BUTTON, qPlot2D
    // SurfaceProcessor
    void setSurfaceColorTableThemeById(int id);
    void setSurfaceEdgeLimit(int val);
    void setExtraWidth(int val);
    // IsobathsProcessor
    void setIsobathsLabelStepSize(float val);
    // Surface/IsobathsProcessor
    void setSurfaceIsobathsStepSize(float val);
    // MosaicProcessor
    void setMosaicChannels(const ChannelId& ch1, uint8_t sub1, const ChannelId& ch2, uint8_t sub2);
    void setMosaicTheme(int indx);
    void setMosaicLAngleOffset(float val);
    void setMosaicRAngleOffset(float val);
    void setMosaicTileResolution(float val);
    void setMosaicLevels(float lowLevel, float highLevel);
    void setMosaicLowLevel(float val);
    void setMosaicHighLevel(float val);
    void askColorTableForMosaic();

    //
    void onIsobathsUpdated();
    void onMosaicUpdated();
    void requestCancel() noexcept;

signals:
    // this
    void sendState(DataProcessorType state);
    void bottomTrackProcessingCleared();
    void isobathsProcessingCleared();
    void mosaicProcessingCleared();
    void surfaceProcessingCleared();
    void allProcessingCleared();
    // BottomTrackProcessor
    void distCompletedByProcessing(int epIndx, const ChannelId& channelId, float dist);
    void lastBottomTrackEpochChanged(const ChannelId& channelId, int val, const BottomTrackParam& btP, bool manual, bool redrawAll);
    // SurfaceProcessor
    void sendSurfaceMinZ(float minZ);
    void sendSurfaceMaxZ(float maxZ);
    void sendSurfaceTextureTask(const std::vector<uint8_t>& textureTask);
    void sendSurfaceColorIntervalsSize(int size);
    void sendSurfaceStepSize(float lineStepSize);
    // IsobathsProcessor
    void sendIsobathsLabels(const QVector<IsobathUtils::LabelParameters>& labels);
    void sendIsobathsLineSegments(const QVector<QVector3D>& lineSegments);
    void sendIsobathsPts(const QVector<QVector3D>& pts);
    void sendIsobathsEdgePts(const QVector<QVector3D>& edgePts);
    void sendIsobathsLineStepSize(float lineStepSize);
    // MosaicProcessor
    void sendMosaicColorTable(const std::vector<uint8_t>& colorTable);
    void sendSurfaceTiles(const TileMap& tiles, bool useTextures);

private slots:
    void runCoalescedWork();
    void startTimerIfNeeded();
    void onWorkerFinished(); // слот на сигнал ComputeWorker::jobFinished

    // All
    void postState(DataProcessorType s);
    // BottomTrack
    void postDistCompletedByProcessing(int epIndx, const ChannelId& channelId, float dist);
    void postLastBottomTrackEpochChanged(const ChannelId& channelId, int val, const BottomTrackParam& btP, bool manual, bool redrawAll);
    // Surface/Mosaic
    void postSurfaceTiles(const TileMap& tiles, bool useTextures);
    // Surface
    void postMinZ(float val);
    void postMaxZ(float val);
    void postSurfaceColorTable(const std::vector<uint8_t>& t);
    void postSurfaceColorIntervalsSize(int size);
    void postSurfaceStepSize(float lineStepSize);
    // Mosaic
    void postMosaicColorTable(const std::vector<uint8_t>& t);
    // Isobaths
    void postIsobathsLabels(const QVector<IsobathUtils::LabelParameters>& labels);
    void postIsobathsLineSegments(const QVector<QVector3D>& lineSegments);

    void onBottomTrackStarted();
    void onBottomTrackFinished();

private:
    // this
    void changeState(const DataProcessorType& state);
    void clearBottomTrackProcessing();
    void clearIsobathsProcessing();
    void clearMosaicProcessing();
    void clearSurfaceProcessing();
    void clearAllProcessings();
    void scheduleLatest(WorkSet mask = WorkSet(WF_All),
                        bool replace = false,
                        bool clearUnrequestedPending = false) noexcept;
    bool isCanStartCalculations() const;

private:
    friend class SurfaceProcessor;
    friend class BottomTrackProcessor;
    friend class IsobathsProcessor;
    friend class MosaicProcessor;

    // this
    Dataset* datasetPtr_;
    
    // рабочая нить и воркер
    QThread computeThread_;
    ComputeWorker* worker_;

    DataProcessorType state_;
    uint64_t chartsCounter_;
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
    // MosaicProcessor
    int mosaicCounter_;
    // Surface
    float tileResolution_;
    QSet<int> epIndxsFromBottomTrack_;
    QSet<int> vertIndxsFromBottomTrack_;
    QSet<QPair<char, int>> pendingSurfaceIndxs_;
    QSet<int> pendingMosaicIndxs_;
    bool pendingIsobathsWork_;
    QTimer pendingWorkTimer_;

    // отмена/планирование
    std::atomic_bool cancelRequested_{false};
    std::atomic_bool jobRunning_{false};
    std::atomic_bool nextRunPending_{false};
    std::atomic<uint32_t> requestedMask_{0};
    bool btBusy_{false};
};
