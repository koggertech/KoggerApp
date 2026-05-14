#pragma once

#include <QObject>
#include <QVector>
#include <QPair>
#include <QSet>
#include "surface_processor.h"
#include "surface_tile.h"
#include "isobaths_processor.h"
#include "mosaic_processor.h"


struct WorkBundle
{
    QVector<QPair<char,int>> surfaceVec;
    QVector<int>             mosaicVec;
    bool                     doIsobaths{false};

    void clear() {
        surfaceVec.clear();
        surfaceVec.shrink_to_fit();
        mosaicVec.clear();
        mosaicVec.shrink_to_fit();
        doIsobaths = false;
    }
};
Q_DECLARE_METATYPE(WorkBundle) // NOLINT(performance-enum-size)

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
    ~ComputeWorker() override;

    const QSet<TileKey>& getVisibleTileKeysCPtr();

public slots:
    // service
    void clearAll();
    void clearSurfaceMosaicContext();
    void clearSurface();
    void clearMosaic();
    void clearIsobaths();

    // settings
    void setDatasetPtr(Dataset* ds);
    void setBottomTrackPtr(BottomTrack* bt);
    void setSurfaceThemeId(int id);
    void setSurfaceEdgeLimit(float v);
    void reapplySurfaceEdgeLimit();
    void setSurfaceExtraWidth(int v);
    void setSurfaceIsobathsStepSize(float v);
    void setIsobathsLabelStepSize(float v);
    void setMosaicChannels(const ChannelId& ch1, uint8_t sub1, const ChannelId& ch2, uint8_t sub2);
    void setMosaicLAngleOffset(float val);
    void setMosaicRAngleOffset(float val);
    void setMosaicTileResolution(float res);
    void setMosaicSource(int source);
    void applySurfaceZoomChange(const TileMap& cached, bool fullCoverage);
    void setMinZ(float v);
    void setMaxZ(float v);

    // tasks
    void processBundle(const WorkBundle& wb); // выполнить пачку задач последовательно
    void setVisibleTileKeys(const QSet<TileKey>& val);

signals:
    void jobFinished(); // для dataProcessor (нормально, отмена)

private:
    inline bool isCanceled() const noexcept;

private:
    DataProcessor*       dp_;
    Dataset*             dataset_;
    SurfaceMesh          surfaceMesh_;

    SurfaceProcessor     surface_;
    IsobathsProcessor    isobaths_;
    MosaicProcessor      mosaic_;

    QSet<TileKey> visibleTileKeys_;
};
