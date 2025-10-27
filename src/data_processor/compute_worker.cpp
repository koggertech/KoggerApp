#include "compute_worker.h"
#include "data_processor.h"
#include "dataset.h"
#include "surface_mesh.h"
#include <QMetaType>
#include <QDebug>

ComputeWorker::ComputeWorker(DataProcessor* ownerDp,
                             Dataset* dataset,
                             QObject* parent)
    : QObject(parent),
      dp_(ownerDp),
      dataset_(dataset),
      surfaceMesh_(defaultTileSidePixelSize, defaultTileHeightMatrixRatio, defaultTileResolution),
      surface_(ownerDp),
      isobaths_(ownerDp),
      mosaic_(ownerDp),
      bottom_(ownerDp)
{
    qRegisterMetaType<WorkBundle>("WorkBundle");

    surfaceMesh_.setMaxInitedTiles(256); // не меньше, чем для разовой трассировки (возможно)

    surface_.setSurfaceMeshPtr(&surfaceMesh_);
    isobaths_.setSurfaceMeshPtr(&surfaceMesh_);
    mosaic_.setSurfaceMeshPtr(&surfaceMesh_);

    bottom_.setDatasetPtr(dataset_);
    mosaic_.setDatasetPtr(dataset_);
}

ComputeWorker::~ComputeWorker() = default;

void ComputeWorker::clearAll()
{
    surface_.clear();
    mosaic_.clear();
    isobaths_.clear();
    bottom_.clear();

    surfaceMesh_.clear();
}

void ComputeWorker::clearSurface()
{
    surface_.clear();
}

void ComputeWorker::clearMosaic()
{
    mosaic_.clear();
}

void ComputeWorker::clearIsobaths()
{
    isobaths_.clear();
}

void ComputeWorker::clearBottomTrack()
{
    bottom_.clear();
}

inline bool ComputeWorker::isCanceled() const noexcept
{
    return dp_ && dp_->isCancelRequested();
}

void ComputeWorker::setDatasetPtr(Dataset* ds)
{
    dataset_ = ds;
    bottom_.setDatasetPtr(ds);
    mosaic_.setDatasetPtr(ds);
}

void ComputeWorker::setBottomTrackPtr(BottomTrack* bt)
{
    surface_.setBottomTrackPtr(bt);
}

void ComputeWorker::setSurfaceThemeId(int id)
{
    surface_.setThemeId(id);

    surface_.rebuildColorIntervals(); // перестройка интервалов цвета
}

void ComputeWorker::setSurfaceEdgeLimit(float v)
{
    surface_.setEdgeLimit(v);
}

void ComputeWorker::setSurfaceExtraWidth(int v)
{
    surface_.setExtraWidth(v);
}

void ComputeWorker::setSurfaceIsobathsStepSize(float v)
{
    surface_.setSurfaceStepSize(v);
    isobaths_.setLineStepSize(v);
}

void ComputeWorker::setIsobathsLabelStepSize(float v)
{
    isobaths_.setLabelStepSize(v);
}

void ComputeWorker::setMosaicChannels(const ChannelId& ch1, uint8_t sub1,
                                      const ChannelId& ch2, uint8_t sub2)
{
    clearAll();

    mosaic_.setChannels(ch1, sub1, ch2, sub2);
}

void ComputeWorker::setMosaicTheme(int id)
{
    mosaic_.setColorTableThemeById(id);
}

void ComputeWorker::setMosaicLAngleOffset(float val)
{
    mosaic_.setLAngleOffset(val);
}

void ComputeWorker::setMosaicRAngleOffset(float val)
{
    mosaic_.setRAngleOffset(val);
}

void ComputeWorker::setMosaicLevels(float lo, float hi)
{
    mosaic_.setColorTableLevels(lo, hi);
}

void ComputeWorker::setMosaicLowLevel(float v)
{
    mosaic_.setColorTableLowLevel(v);
}

void ComputeWorker::setMosaicHighLevel(float v)
{
    mosaic_.setColorTableHighLevel(v);
}

void ComputeWorker::setMosaicTileResolution(float res)
{
    //qDebug() << "ComputeWorker::setMosaicTileResolution" << res;
    if (res <= 0.f) {
        return;
    }

    surfaceMesh_.reinit(defaultTileSidePixelSize, defaultTileHeightMatrixRatio, res);
    surface_.setTileResolution(res);
    mosaic_.setTileResolution(res);
}

void ComputeWorker::askColorTableForMosaic()
{
    mosaic_.askColorTableForMosaic();
}

void ComputeWorker::setMinZ(float v)
{
    isobaths_.setMinZ(v);
}

void ComputeWorker::setMaxZ(float v)
{
    isobaths_.setMaxZ(v);
}

void ComputeWorker::bottomTrackProcessing(const DatasetChannel& ch1,
                                          const DatasetChannel& ch2,
                                          const BottomTrackParam& p,
                                          bool manual)
{
    emit bottomTrackStarted();

    bottom_.bottomTrackProcessing(ch1, ch2, p, manual);

    emit bottomTrackFinished();
}

void ComputeWorker::processBundle(const WorkBundle& wb)
{
    //qDebug() << "ComputeWorker::processBundle task, mos:" << wb.mosaicVec.size() << "sur:" <<  wb.surfaceVec << "iso:" << wb.doIsobaths;
    // последовательно. cабы сами шлют сигналы наружу
    if (!wb.surfaceVec.isEmpty() && !isCanceled()) {
        surface_.onUpdatedBottomTrackData(wb.surfaceVec);
        surface_.rebuildColorIntervals();
    }

    if (!wb.mosaicVec.isEmpty() && !isCanceled()) {
        mosaic_.updateDataWrapper(wb.mosaicVec);
    }

    if (wb.doIsobaths && !isCanceled()) {
        isobaths_.onUpdatedBottomTrackData();
    }

    emit jobFinished();
}
