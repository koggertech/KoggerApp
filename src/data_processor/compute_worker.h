#pragma once

#include <QObject>
#include <QVector>
#include <QPair>
#include <QSet>
#include <QUuid>
#include "surface_processor.h"
#include "isobaths_processor.h"
#include "mosaic_processor.h"
#include "bottom_track_processor.h"


struct WorkBundle
{
    QVector<QPair<char,int>> surfaceVec;
    QVector<int>             mosaicVec;
    bool                     doIsobaths{false};
};
Q_DECLARE_METATYPE(WorkBundle)

class Dataset;
class DataProcessor;
class SurfaceMesh;

class ComputeWorker : public QObject
{
    Q_OBJECT

public:
    explicit ComputeWorker(DataProcessor* ownerDp,
                           Dataset* dataset,
                           QObject* parent = nullptr);
    ~ComputeWorker();

public slots:
    // service
    void clearAll();
    void clearSurface();
    void clearMosaic();
    void clearIsobaths();
    void clearBottomTrack();

    // settings
    void setDatasetPtr(Dataset* ds);
    void setBottomTrackPtr(BottomTrack* bt);
    void setSurfaceThemeId(int id);
    void setSurfaceEdgeLimit(float v);
    void setSurfaceExtraWidth(int v);
    void setSurfaceIsobathsStepSize(float v);
    void setIsobathsLabelStepSize(float v);
    void setMosaicChannels(const ChannelId& ch1, uint8_t sub1, const ChannelId& ch2, uint8_t sub2);
    void setMosaicTheme(int id);
    void setMosaicLAngleOffset(float val);
    void setMosaicRAngleOffset(float val);
    void setMosaicLevels(float lo, float hi);
    void setMosaicLowLevel(float v);
    void setMosaicHighLevel(float v);
    void setMosaicTileResolution(float res);
    void askColorTableForMosaic();
    void setMinZ(float v);
    void setMaxZ(float v);

    // tasks
    void bottomTrackProcessing(const DatasetChannel& ch1, const DatasetChannel& ch2, const BottomTrackParam& p, bool manual);
    void processBundle(const WorkBundle& wb); // выполнить пачку задач последовательно

signals:
    void jobFinished(); // для dataProcessor (нормально, отмена)
    void bottomTrackStarted();
    void bottomTrackFinished();

private:
    inline bool isCanceled() const noexcept;

private:
    DataProcessor*       dp_;
    Dataset*             dataset_;
    SurfaceMesh          surfaceMesh_;

    SurfaceProcessor     surface_;
    IsobathsProcessor    isobaths_;
    MosaicProcessor      mosaic_;
    BottomTrackProcessor bottom_;
};
