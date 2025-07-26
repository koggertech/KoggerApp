// #pragma once


// #include <QMutex>
// #include <QQueue>
// #include <QReadWriteLock>

// #include "dataset.h"
// #include "mosaic_mesh.h"
// #include "mosaic_tile.h"
// #include "draw_utils.h"



// class Dataset;
// class DataProcessor;
// class MosaicProcessor
// {
// public:

//     enum class Mode {
//         kUndefined = 0,
//         kRealtime,
//         kPerformance
//     };

//     explicit MosaicProcessor(DataProcessor* parent);
//     ~MosaicProcessor();

//     void clear(bool force = false);
//     void setDatasetPtr(Dataset* datasetPtr);











//     // PROCESSING
//     bool updateChannelsIds();
//     void startUpdateDataInThread(int endIndx, int endOffset = 0); // вызывался в реалтайме по интерполяции, вызывается в контроллере по кнопке
//     void resetTileSettings(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution);
//     void setGenerateGridContour(bool state);
//     void setColorTableThemeById(int id);
//     void setColorTableLevels(float lowVal, float highVal);
//     void setColorTableLowLevel(float val);
//     void setColorTableHighLevel(float val);
//     void setWorkMode(Mode mode);
//     Mode getWorkMode() const;
//     void setLAngleOffset(float val);
//     void setRAngleOffset(float val);
//     void setChannels(const ChannelId& firstChId, uint8_t firstSubChId, const ChannelId& secondChId, uint8_t secondSubChId);

// signals:
//     void sendStartedInThread(bool);
//     void sendUpdatedWorkMode(Mode);








// private:
//     mosaic::PlotColorTable colorTable_;


//     DataProcessor* dataProcessor_;
//     Dataset* datasetPtr_ = nullptr;


//     // PROCESSING

//     void postUpdate(); //

//     void updateUnmarkedHeightVertices(Tile* tilePtr) const; //

//     void updateData(int endIndx, int endOffset = 0, bool backgroungThread = false);
//     inline bool checkLength(float dist) const;
//     MatrixParams getMatrixParams(const QVector<QVector3D> &vertices) const;
//     void concatenateMatrixParameters(MatrixParams& srcDst, const MatrixParams& src) const;
//     inline int getColorIndx(Epoch::Echogram* charts, int ampIndx) const;
//     /*data*/
//     static constexpr int colorTableSize_ = 255;
//     static constexpr int interpLineWidth_ = 1;




//     GlobalMesh globalMesh_;


//     MatrixParams lastMatParams_;
//     float tileResolution_;
//     uint64_t currIndxSec_;
//     ChannelId segFChannelId_;
//     uint8_t segFSubChannelId_;
//     ChannelId segSChannelId_;
//     uint8_t segSSubChannelId_;
//     int tileSidePixelSize_;
//     int tileHeightMatrixRatio_;
//     int lastCalcEpoch_;
//     int lastAcceptedEpoch_;
//     Mode workMode_;
//     bool manualSettedChannels_;
//     float lAngleOffset_;
//     float rAngleOffset_;
//     QMutex mutex_;
//     QReadWriteLock rWLocker_;
//     bool startedInThread_;



// };
