#include "compute_worker.h"
#include "data_processor.h"
#include "dataset.h"
#include <QMetaType>
#include <QDebug>

ComputeWorker::ComputeWorker(DataProcessor* ownerDp,
                             Dataset* dataset,
                             SurfaceMesh* surfaceMesh,
                             QObject* parent)
    : QObject(parent),
      dp_(ownerDp),
      dataset_(dataset),
      surfaceMesh_(surfaceMesh),
      surface_(ownerDp),
      isobaths_(ownerDp),
      mosaic_(ownerDp),
      bottom_(ownerDp)
{
    qRegisterMetaType<WorkBundle>("WorkBundle");

    surface_.setSurfaceMeshPtr(surfaceMesh_);
    isobaths_.setSurfaceMeshPtr(surfaceMesh_);
    mosaic_.setSurfaceMeshPtr(surfaceMesh_);

    bottom_.setDatasetPtr(dataset_);
    mosaic_.setDatasetPtr(dataset_);
}

ComputeWorker::~ComputeWorker() = default;

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

    surface_.rebuildColorIntervals(); // или сначала считать общее кол-во
}

void ComputeWorker::setIsobathsLabelStepSize(float v)
{
    isobaths_.setLabelStepSize(v);
}

void ComputeWorker::setMosaicChannels(const ChannelId& ch1, uint8_t sub1,
                                      const ChannelId& ch2, uint8_t sub2)
{
    surfaceMesh_->clear();
    mosaic_.setChannels(ch1, sub1, ch2, sub2);

    // TODO: чистки рендера
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
    if (res <= 0.f) {
        return;
    }

    surfaceMesh_->reinit(defaultTileSidePixelSize, defaultTileHeightMatrixRatio, res);
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
    bottom_.bottomTrackProcessing(ch1, ch2, p, manual);
}

void ComputeWorker::processBundle(const WorkBundle& wb)
{
    // последовательно. cабы сами шлют сигналы наружу
    if (!wb.bottomJobs.isEmpty() && !isCanceled()) {
        for (const auto& j : wb.bottomJobs) {
            if (isCanceled()) break;
            bottom_.bottomTrackProcessing(j.ch1, j.ch2, j.p, j.manual);
            if (isCanceled()) break;
        }
    }

    if (!wb.surfaceVec.isEmpty() && !isCanceled()) {
        surface_.onUpdatedBottomTrackData(wb.surfaceVec);
    }

    if (!wb.mosaicVec.isEmpty() && !isCanceled()) {
        mosaic_.updateDataWrapper(wb.mosaicVec);
    }

    if (wb.doIsobaths && !isCanceled()) {
        isobaths_.onUpdatedBottomTrackData();
    }

    emit jobFinished();
}
