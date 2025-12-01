#pragma once

#include <atomic>
#include <list>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include <QSet>
#include <QVector>
#include <QVector3D>
#include <QPair>
#include <QThread>
#include <QWaitCondition>

#include "dataset_defs.h"
#include "bottom_track_processor.h"
#include "isobaths_processor.h"
#include "mosaic_processor.h"
#include "surface_processor.h"
#include "mosaic_db.h"
#include "mosaic_index_provider.h"
#include "hot_tile_cache.h"


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

    void onDbSaveTiles(const QHash<TileKey, SurfaceTile>& tiles);
    bool isDbReady() const noexcept;

public slots:
    // this
    void setBottomTrackPtr(BottomTrack* bottomTrackPtr);
    void clearProcessing(DataProcessorType = DataProcessorType::kUndefined);

    // from 3d controller (visibility)
    void setUpdateBottomTrack (bool state);
    void setUpdateSurface(bool state);
    void setUpdateIsobaths (bool state);
    void setUpdateMosaic (bool state);

    void setIsOpeningFile (bool state);
    //
    void onCameraMoved(const QVector<int>& epIndxs);

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

    // zoom
    void onUpdateMosaic(int zoom); // temp
    void setFilePath(QString filePath);

    void onSendDataRectRequest(QVector<NED> rect, int zoomIndx, bool moveUp); // на движение камеры
    void tryCalcTiles();

    TileMap fetchFromHotCache(const QSet<TileKey>& keys, QSet<TileKey>* missing);

    void requestTilesFromDB(const QSet<TileKey>& keys);
    void filterNotFoundOut(const QSet<TileKey>& in, QSet<TileKey>* out);
    quint64 prefetchProgressTick() const;
    void prefetchWait(quint64 lastTick);
    void shutdown(); // correct termination of processes

private slots:
    //db
    void onDbTilesLoadedForZoom(int zoom, const QList<DbTile>& dbTiles);
    void onDbTilesLoaded(const QList<DbTile>& dbTiles);
    void onDbAnyTileForZoom(int zoom, bool exists);

signals:
    void sendTraceLines(const QVector3D& leftBeg, const QVector3D& leftEnd, const QVector3D& rightBeg, const QVector3D& rightEnd);
    // db
    void dbCheckAnyTileForZoom(int zoom);
    void dbSaveTiles(int engineVer, const QHash<TileKey, SurfaceTile>& tiles, bool useTextures, int tilePx, int hmRatio);
    void dbLoadTilesForKeys(const QSet<TileKey>& keys);

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

    void sendSurfaceTilesIncremental(const TileMap& upserts, const QSet<TileKey>& fullVisibleNow);

private slots:
    void postTraceLines(const QVector3D& leftBeg, const QVector3D& leftEnd, const QVector3D& rightBeg, const QVector3D& rightEnd);

    void runCoalescedWork();
    void startTimerIfNeeded();
    void onWorkerFinished(); // слот на сигнал ComputeWorker::jobFinished

    // All
    void postState(DataProcessorType s);
    // BottomTrack
    void postDistCompletedByProcessing(int epIndx, const ChannelId& channelId, float dist);
    void postLastBottomTrackEpochChanged(const ChannelId& channelId, int val, const BottomTrackParam& btP, bool manual, bool redrawAll);
    // Surface/Mosaic
    void postSurfaceTiles(TileMap tiles, bool useTextures);
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

    // db
    void onSendSavedKeys(QVector<TileKey> savedKeys);

private:
    void flushPendingDbKeys();

    // this
    void changeState(const DataProcessorType& state);
    void clearBottomTrackProcessing();
    void clearIsobathsProcessing();
    void clearMosaicProcessing();
    void clearSurfaceProcessing();
    void clearAllProcessings();
    void scheduleLatest(WorkSet mask = WorkSet(WF_All), bool replace = false, bool clearUnrequestedPending = false) noexcept;
    void openDB();
    void closeDB();
    void initMosaicIndexProvider();
    bool isCanStartCalculations() const;

    void emitDelta(TileMap&& upserts, DataSource src);
    void pumpVisible();
    bool isValidZoomIndx(int zoomIndx) const;

    // not found LRU
    inline void nfTouch(const TileKey& k);
    inline void nfErase(const TileKey& k);

    void notifyPrefetchProgress();
    void clearDbNotFoundCache();

private:
    friend class SurfaceProcessor;
    friend class BottomTrackProcessor;
    friend class IsobathsProcessor;
    friend class MosaicProcessor;

    static constexpr int maxZoom_         = 1;
    static constexpr int minZoom_         = 7;
    static constexpr int hotCacheMaxSize_    = 2048;
    static constexpr int hotCacheMinSize_    = 1024;
    static constexpr int dbNotFoundLimit_ = 4096;

    // this
    MosaicIndexProvider mosaicIndexProvider_;
    Dataset* datasetPtr_;
    
    QThread computeThread_;
    ComputeWorker* worker_;

    DataProcessorType state_;
    uint64_t chartsCounter_;
    uint64_t bottomTrackCounter_;
    uint64_t epochCounter_;
    uint64_t positionCounter_;
    uint64_t attitudeCounter_;
    bool updateBottomTrack_;
    bool updateSurface_;
    bool updateIsobaths_;
    bool updateMosaic_;
    bool isOpeningFile_;
    // BottomTrackProcessor
    int bottomTrackWindowCounter_;
    // MosaicProcessor
    int mosaicCounter_;
    // Surface
    float tileResolution_;

    // processing (scheduling/interrupt)
    QSet<int>              epIndxsFromBottomTrack_;
    QSet<int> vertIndxsFromBottomTrack_;
    QSet<QPair<char, int>> pendingSurfaceIndxs_;
    QSet<int>              pendingMosaicIndxs_;
    bool                   pendingIsobathsWork_;
    QTimer                 pendingWorkTimer_;
    std::atomic_bool       cancelRequested_;
    std::atomic_bool       jobRunning_;
    std::atomic_bool       nextRunPending_;
    std::atomic<uint32_t>  requestedMask_;
    bool                   btBusy_;
    // hot cache/db
    HotTileCache           hotCache_; // LRU
    MosaicDB*              dbReader_;
    bool                   dbReaderInWork_;
    MosaicDB*              dbWriter_;
    QThread                dbReadThread_;
    QThread                dbWriteThread_;
    QString                filePath_;
    int                    engineVer_;
    QRectF                 lastViewRect_;
    int                    lastZoom_;
    QSet<TileKey>          lastKeys_;
    QSet<TileKey>          dbPendingKeys_;
    QSet<TileKey>          dbInWorkKeys_;
    QSet<TileKey>          renderedKeys_;

    // not found LRU
    QSet<TileKey>                                dbNotFoundIndxs_;
    std::list<TileKey>                           dbNotFoundOrder_; // LRU: front - oldest
    QHash<TileKey, std::list<TileKey>::iterator> dbNotFoundPos_;

    // prefetch
    std::atomic_bool     dbIsReady_;
    QMutex               prefetchMu_;
    QWaitCondition       prefetchCv_;
    std::atomic<quint64> prefetchTick_;
};
