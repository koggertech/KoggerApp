#pragma once

#include "dataset_defs.h"
#include "epoch.h"
#include "surface_tile.h"
#include "draw_utils.h"
#include "math_defs.h"

using namespace mosaic;


class Dataset;
class DataProcessor;
class SurfaceMesh;
class MosaicProcessor
{
public:
    explicit MosaicProcessor(DataProcessor* parent);
    ~MosaicProcessor();

    void clear();

    void setDatasetPtr(Dataset* datasetPtr);
    void setSurfaceMeshPtr(SurfaceMesh* surfaceMeshPtr);

    // PROCESSING
    void setChannels(const ChannelId& firstChId, uint8_t firstSubChId, const ChannelId& secondChId, uint8_t secondSubChId);
    void updateDataWrapper(const QVector<int>& indxs);
    void resetTileSettings(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution);
    void setColorTableThemeById(int id);
    void setColorTableLevels(float lowVal, float highVal);
    void setColorTableLowLevel(float val);
    void setColorTableHighLevel(float val);
    void setLAngleOffset(float val);
    void setRAngleOffset(float val);
    void setTileResolution(float tileResolution);
    void setGenerageGridContour(bool state);

    void askColorTableForMosaic(); // first init colorTable in render

    QPair<ChannelId, uint8_t> getFirstChannelId()  const { return qMakePair(segFChannelId_, segFSubChannelId_); };
    QPair<ChannelId, uint8_t> getSecondChannelId() const { return qMakePair(segSChannelId_, segSSubChannelId_); };

private:
    void postUpdate(QSet<SurfaceTile*>& changedTiles);
    void updateUnmarkedHeightVertices(SurfaceTile* tilePtr) const;
    void updateData(const QVector<int>& indxs);
    inline int getColorIndx(Epoch::Echogram* charts, int ampIndx) const;
    bool canceled() const noexcept;

private:
    mosaic::PlotColorTable colorTable_;
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_;
    SurfaceMesh* surfaceMeshPtr_;
    kmath::MatrixParams lastMatParams_;
    float tileResolution_;
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
