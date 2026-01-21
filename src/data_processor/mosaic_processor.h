#pragma once

#include "dataset_defs.h"
#include "epoch.h"
#include "surface_tile.h"
#include "math_defs.h"


class ComputeWorker;
class Dataset;
class DataProcessor;
class SurfaceMesh;
class MosaicProcessor
{
public:
    explicit MosaicProcessor(DataProcessor* parent, ComputeWorker* computeWorker);
    ~MosaicProcessor();

    void clear();

    void setDatasetPtr(Dataset* datasetPtr);
    void setSurfaceMeshPtr(SurfaceMesh* surfaceMeshPtr);

    // PROCESSING
    void setChannels(const ChannelId& firstChId, uint8_t firstSubChId, const ChannelId& secondChId, uint8_t secondSubChId);
    void updateDataWrapper(const QVector<int>& indxs);
    void setLAngleOffset(float val);
    void setRAngleOffset(float val);
    void setTileResolution(float tileResolution);
    void setGenerageGridContour(bool state);

    QPair<ChannelId, uint8_t> getFirstChannelId()  const { return qMakePair(segFChannelId_, segFSubChannelId_); };
    QPair<ChannelId, uint8_t> getSecondChannelId() const { return qMakePair(segSChannelId_, segSSubChannelId_); };

private:
    void postUpdate(const QSet<SurfaceTile*>& updatedIn, QSet<SurfaceTile*>& changedOut);
    void updateUnmarkedHeightVertices(SurfaceTile* tilePtr) const;
    void updateData(const QVector<int>& indxs, QSet<int>& usedEpochs, QSet<int>& blockedEpochs);
    inline int getColorIndx(Epoch::Echogram* charts, int ampIndx) const;
    bool canceled() const noexcept;

    // prepairing tiles
    QSet<TileKey> forecastTilesToTouch(const QVector<QVector3D>& meas, const QVector<char>& isOdds, const QVector<int>& epochIndxs, int marginTiles = 0) const;
    void putTilesIntoMesh(const TileMap& tiles);
    bool prefetchTiles(const QSet<TileKey>& keys);
    QVector<QVector<int>> splitContinuousSegments(const QVector<int>& indxs, int minSegmentLen, int maxSegmentLen);

private:
    const int expandMargin_ = 1;
    ComputeWorker* computeWorker_;
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_;
    SurfaceMesh* surfaceMeshPtr_;
    kmath::MatrixParams lastMatParams_;
    float tileResolution_;
    int pixOnMeters_;
    int aliasWindow_;
    uint64_t currIndxSec_;
    ChannelId segFChannelId_;
    uint8_t segFSubChannelId_;
    ChannelId segSChannelId_;
    uint8_t segSSubChannelId_;
    int tileSidePixelSize_;
    int tileHeightMatrixRatio_;
    int lastCalcEpoch_;
    int lastAcceptedEpoch_;
    float lAngleOffset_;
    float rAngleOffset_;
    bool generateGridContour_;
};
