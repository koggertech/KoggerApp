#include "compute_worker.h"
#include "data_processor.h"
#include "dataset.h"
#include "surface_mesh.h"
#include <QMetaType>
#include <QDebug>

namespace {
constexpr int kSurfaceMeshHighWM = 512;
constexpr int kSurfaceMeshLowWM = 256;
}

ComputeWorker::ComputeWorker(DataProcessor* ownerDp,
                             Dataset* dataset,
                             QObject* parent)
    : QObject(parent),
      dp_(ownerDp),
      dataset_(dataset),
      surfaceMesh_(defaultTileSidePixelSize, defaultTileHeightMatrixRatio, defaultTileResolution),
      surface_(ownerDp),
      isobaths_(ownerDp),
      mosaic_(ownerDp, this)
{
    qRegisterMetaType<WorkBundle>("WorkBundle");

    surfaceMesh_.setLRUWatermarks(kSurfaceMeshHighWM, kSurfaceMeshLowWM);

    surface_.setSurfaceMeshPtr(&surfaceMesh_);
    isobaths_.setSurfaceMeshPtr(&surfaceMesh_);
    mosaic_.setSurfaceMeshPtr(&surfaceMesh_);

    mosaic_.setDatasetPtr(dataset_);
}

ComputeWorker::~ComputeWorker() = default;

const QSet<TileKey> &ComputeWorker::getVisibleTileKeysCPtr()
{
    return visibleTileKeys_;
}

void ComputeWorker::clearAll()
{
    surface_.clear();
    mosaic_.clear();
    isobaths_.clear();

    surfaceMesh_.clear();
}

void ComputeWorker::clearSurfaceMosaicContext()
{
    surface_.clear();
    mosaic_.clear();
    isobaths_.clear();
    surfaceMesh_.clear();
}

void ComputeWorker::clearSurface()
{
    surface_.clear();
}

void ComputeWorker::clearMosaic()
{
    mosaic_.clear();

    for (SurfaceTile* t : surfaceMesh_.getTilesCRef()) {
        if (!t) {
            continue;
        }
        t->setHeadIndx(-1);
        auto& img = t->getMosaicImageDataRef();
        std::fill(img.begin(), img.end(), uint8_t(0));
        t->setIsUpdated(true);
    }
}

void ComputeWorker::clearIsobaths()
{
    isobaths_.clear();
}

inline bool ComputeWorker::isCanceled() const noexcept
{
    return dp_ && dp_->isCancelRequested();
}

void ComputeWorker::setDatasetPtr(Dataset* ds)
{
    dataset_ = ds;
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

void ComputeWorker::reapplySurfaceEdgeLimit()
{
    surface_.setEdgeLimit(surface_.getEdgeLimit());
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
    mosaic_.setChannels(ch1, sub1, ch2, sub2);

    clearAll();
}

void ComputeWorker::setMosaicLAngleOffset(float val)
{
    mosaic_.setLAngleOffset(val);
}

void ComputeWorker::setMosaicRAngleOffset(float val)
{
    mosaic_.setRAngleOffset(val);
}

void ComputeWorker::setMosaicSource(int source)
{
    mosaic_.setSource(static_cast<MosaicProcessor::Source>(source));
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

void ComputeWorker::applySurfaceZoomChange(const TileMap& cached, bool fullCoverage)
{
    Q_UNUSED(fullCoverage);

    QMetaObject::invokeMethod(dp_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kSurface));

    if (!cached.isEmpty()) {
        surface_.restoreTilesFromCache(cached);
    }

    QMetaObject::invokeMethod(dp_, "postState", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kUndefined));
}

void ComputeWorker::setMinZ(float v)
{
    isobaths_.setMinZ(v);
}

void ComputeWorker::setMaxZ(float v)
{
    isobaths_.setMaxZ(v);
}

void ComputeWorker::processBundle(const WorkBundle& wb)
{
    //qDebug() << "ComputeWorker::processBundle: task" <<  wb.mosaicVec.size() ;
    // последовательно. cабы сами шлют сигналы наружу
    if (!wb.surfaceVec.isEmpty()) { // && !isCanceled()
        surface_.onUpdatedBottomTrackData(wb.surfaceVec);
        surface_.rebuildColorIntervals();
    }

    if (wb.doIsobaths && !isCanceled()) {
        //isobaths_.onUpdatedBottomTrackData();
    }

    if (!wb.mosaicVec.isEmpty() && !isCanceled()) {
        mosaic_.updateDataWrapper(wb.mosaicVec);
    }

    emit jobFinished();
}

void ComputeWorker::setVisibleTileKeys(const QSet<TileKey>& val)
{
    visibleTileKeys_ = val;
    surface_.setVisibleTileKeys(val);
}
