#pragma once

#include <QMutex>
#include <QQueue>
#include <QReadWriteLock>

#include "dataset_defs.h"
#include "dataset.h"
#include "mosaic_tile.h"
#include "draw_utils.h"

using namespace mosaic;


class Dataset;
class DataProcessor;
class GlobalMesh;
class MosaicProcessor
{
public:
    explicit MosaicProcessor(DataProcessor* parent);
    ~MosaicProcessor();

    void clear();

    void setDatasetPtr(Dataset* datasetPtr);
    void setGlobalMeshPtr(GlobalMesh* globalMeshPtr);


    // PROCESSING
    void setChannels(const ChannelId& firstChId, uint8_t firstSubChId, const ChannelId& secondChId, uint8_t secondSubChId);
    void startUpdateDataInThread(int endIndx, int endOffset = 0);
    void resetTileSettings(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution);
    void setGenerateGridContour(bool state);
    void setColorTableThemeById(int id);
    void setColorTableLevels(float lowVal, float highVal);
    void setColorTableLowLevel(float val);
    void setColorTableHighLevel(float val);
    void setLAngleOffset(float val);
    void setRAngleOffset(float val);
    void setResolution(float pixPerMeters);
    void setGenerageGridContour(bool state);


private:
    void postUpdate();
    void updateUnmarkedHeightVertices(Tile* tilePtr) const;
    void updateData(int endIndx, int endOffset = 0);
    inline bool checkLength(float dist) const;
    MatrixParams getMatrixParams(const QVector<QVector3D> &vertices) const;
    void concatenateMatrixParameters(MatrixParams& srcDst, const MatrixParams& src) const;
    inline int getColorIndx(Epoch::Echogram* charts, int ampIndx) const;

private:
    static constexpr int colorTableSize_ = 255;
    static constexpr int interpLineWidth_ = 1;

    mosaic::PlotColorTable colorTable_;
    DataProcessor* dataProcessor_;
    Dataset* datasetPtr_ = nullptr;
    GlobalMesh* globalMeshPtr_ = nullptr;
    MatrixParams lastMatParams_;
    float tileResolution_ = 0.1f;
    uint64_t currIndxSec_ = 0;
    ChannelId segFChannelId_;
    uint8_t segFSubChannelId_ = 0;
    ChannelId segSChannelId_;
    uint8_t segSSubChannelId_ = 0;
    int tileSidePixelSize_ = 256;
    int tileHeightMatrixRatio_ = 16;
    int lastCalcEpoch_ = 0;
    int lastAcceptedEpoch_ = 0;
    float lAngleOffset_ = 0.0f;
    float rAngleOffset_ = 0.0f;
    bool generateGridContour_ = false;
};
