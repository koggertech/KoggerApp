#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVector>
#include <QVector3D>
#include "surface_mesh.h"
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
    void clearProcessing(DataProcessorType = DataProcessorType::kUndefined);
    void setUpdateBottomTrack (bool state);
    void setUpdateIsobaths (bool state);
    void setUpdateMosaic (bool state);
    void setIsOpeningFile (bool state);
    // from DataHorizon
    void onChartsAdded(uint64_t indx); // external calling realtime
    void onBottomTrackAdded(const QVector<int>& indxs, bool manual);
    void onEpochAdded(uint64_t indx);
    void onPositionAdded(uint64_t indx);
    void onAttitudeAdded(uint64_t indx);
    void onMosaicCanCalc(uint64_t indx);
    // BottomTrackProcessor
    void bottomTrackProcessing(const DatasetChannel& channel1, const DatasetChannel& channel2, const BottomTrackParam& bottomTrackParam, bool manual); // CALC BOTTOM TRACK BY BUTTON
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
    void setMinZ(float minZ);
    void setMaxZ(float maxZ);

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
    void lastBottomTrackEpochChanged(const ChannelId& channelId, int val, const BottomTrackParam& btP, bool manual);
    // SurfaceProcessor
    void sendSurfaceMinZ(float minZ);
    void sendSurfaceMaxZ(float maxZ);
    void sendSurfaceTextureTask(const QVector<uint8_t>& textureTask);
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
    void sendMosaicTiles(QHash<QUuid, SurfaceTile> tiles, bool useTextures);

private slots:
    void flushPendingWork();

private:
    // this
    void changeState(const DataProcessorType& state);
    void clearBottomTrackProcessing();
    void clearIsobathsProcessing();
    void clearMosaicProcessing();
    void clearSurfaceProcessing();
    void clearAllProcessings();

private:
    friend class SurfaceProcessor;
    friend class BottomTrackProcessor;
    friend class IsobathsProcessor;
    friend class MosaicProcessor;

    // this
    Dataset* datasetPtr_;
    SurfaceMesh surfaceMesh_;
    BottomTrackProcessor bottomTrackProcessor_; // need Charts
    IsobathsProcessor isobathsProcessor_; // need BottomTrack to calc
    MosaicProcessor mosaicProcessor_; // need BottomTrack, Charts, Attitude to calc
    SurfaceProcessor surfaceProcessor_;
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

    QSet<int> pendingBtIndxs_;
    bool pendingBtManualState_;
    QTimer pendingBtTimer_;
};
