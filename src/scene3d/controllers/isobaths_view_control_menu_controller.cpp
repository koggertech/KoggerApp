#include "isobaths_view_control_menu_controller.h"
#include "scene3d_view.h"
#include "isobaths_defs.h"
#include "bottom_track.h"
#include "dataset.h"
#include "notifications.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <QColor>
#include <QDir>
#include <QUrl>
#include <QVariantMap>
#include <QtConcurrent/QtConcurrentRun>

extern Notifications notifications;

namespace {

constexpr int kStatusPollMs = 700;
constexpr int kStatusCollapsedPollMs = 3000;
constexpr int kEstimateRefreshMs = 3000;
const QString kSurfaceExportTag = QStringLiteral("surface-export");

} // namespace


IsobathsViewControlMenuController::IsobathsViewControlMenuController(QObject* parent)
    : QmlComponentController(parent),
    graphicsSceneViewPtr_(nullptr),
    dataProcessorPtr_(nullptr),
    datasetPtr_(nullptr),
    pendingLambda_(nullptr),
    surfaceDepthMin_(std::numeric_limits<float>::quiet_NaN()),
    surfaceDepthMax_(std::numeric_limits<float>::quiet_NaN()),
    surfaceLineStepSize_(3.0f),
    themeId_(0),
    labelStepSize_(100),
    edgeLimit_(100),
    extraWidth_(10),
    visibility_(false),
    bandedColors_(false),
    edgesVisible_(false),
    trianglesVisible_(false),
    debugModeView_(false),
    processState_(true),
    statusMonitorEnabled_(false),
    statusRequestPending_(false),
    statusDetailedPolling_(false)
{
    qRegisterMetaType<DataProcessorType>("DataProcessorType");

    statusTimer_.setInterval(kStatusPollMs);
    QObject::connect(&statusTimer_, &QTimer::timeout, this, &IsobathsViewControlMenuController::refreshPipelineStatus);

    QObject::connect(&exportWatcher_, &QFutureWatcher<SurfaceExportResult>::progressValueChanged,
                     this, &IsobathsViewControlMenuController::onSurfaceExportProgress);
    QObject::connect(&exportWatcher_, &QFutureWatcher<SurfaceExportResult>::finished,
                     this, &IsobathsViewControlMenuController::onSurfaceExportFinished);
    QObject::connect(&estimateWatcher_, &QFutureWatcher<SurfaceExportResult>::finished,
                     this, &IsobathsViewControlMenuController::onSurfaceExportEstimateFinished);

    estimateRefreshTimer_.setSingleShot(true);
    estimateRefreshTimer_.setInterval(kEstimateRefreshMs);
    QObject::connect(&estimateRefreshTimer_, &QTimer::timeout,
                     this, &IsobathsViewControlMenuController::requestSurfaceExportEstimate);
}

IsobathsViewControlMenuController::~IsobathsViewControlMenuController()
{
    exportWatcher_.disconnect(this);
    estimateWatcher_.disconnect(this);
    if (exportCancel_) {
        exportCancel_->store(true);
    }
    exportWatcher_.cancel();
    estimateWatcher_.cancel();
    exportWatcher_.waitForFinished();
    estimateWatcher_.waitForFinished();
}

void IsobathsViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView* sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void IsobathsViewControlMenuController::setDataProcessorPtr(DataProcessor *dataProcessorPtr)
{
    dataProcessorPtr_ = dataProcessorPtr;

    if (dataProcessorPtr_) {
        QObject::connect(dataProcessorPtr_, &DataProcessor::pipelineStats,
                         this, &IsobathsViewControlMenuController::onPipelineStats,
                         Qt::UniqueConnection);
        QObject::connect(dataProcessorPtr_, &DataProcessor::sendSurfaceMinZ,
                         this, &IsobathsViewControlMenuController::onSurfaceMinZ,
                         Qt::UniqueConnection);
        QObject::connect(dataProcessorPtr_, &DataProcessor::sendSurfaceMaxZ,
                         this, &IsobathsViewControlMenuController::onSurfaceMaxZ,
                         Qt::UniqueConnection);
        QObject::connect(dataProcessorPtr_, &DataProcessor::allProcessingCleared,
                         this, &IsobathsViewControlMenuController::onSurfaceCleared,
                         Qt::UniqueConnection);
        QObject::connect(dataProcessorPtr_, &DataProcessor::surfaceExportSnapshotReady,
                         this, &IsobathsViewControlMenuController::onSurfaceExportSnapshotReady,
                         Qt::UniqueConnection);
        QObject::connect(dataProcessorPtr_, &DataProcessor::idleChanged,
                         this, &IsobathsViewControlMenuController::onProcessingIdleChanged,
                         Qt::UniqueConnection);
        QObject::connect(dataProcessorPtr_, &QObject::destroyed,
                         this, &IsobathsViewControlMenuController::onDataProcessorDestroyed,
                         Qt::UniqueConnection);
    }
}

void IsobathsViewControlMenuController::setDatasetPtr(Dataset* datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

float IsobathsViewControlMenuController::isobathStep() const
{
    return surfaceLineStepSize_;
}

bool IsobathsViewControlMenuController::bandedColors() const
{
    return bandedColors_;
}

void IsobathsViewControlMenuController::setBandedColors(bool state)
{
    const bool changed = bandedColors_ != state;
    bandedColors_ = state;

    if (graphicsSceneViewPtr_) {
        if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
            surfacePtr->setBandedColors(bandedColors_);
        }
    }
    else {
        tryInitPendingLambda();
    }

    if (changed) {
        emit bandedColorsChanged();
    }
}

QVariantList IsobathsViewControlMenuController::surfacePaletteColors() const
{
    const QVector<QVector3D> palette = IsobathUtils::surfacePalette(themeId_);
    QVariantList colors;
    colors.reserve(palette.size());
    for (const QVector3D& c : palette) {
        colors.append(QColor::fromRgbF(c.x(), c.y(), c.z()).name());
    }
    return colors;
}

float IsobathsViewControlMenuController::surfaceDepthMin() const
{
    return surfaceDepthMin_;
}

float IsobathsViewControlMenuController::surfaceDepthMax() const
{
    return surfaceDepthMax_;
}

namespace {

bool sameDepth(float a, float b)
{
    if (std::isnan(a) || std::isnan(b)) {
        return std::isnan(a) && std::isnan(b);
    }
    return qFuzzyCompare(1.0f + a, 1.0f + b);
}

} // namespace

void IsobathsViewControlMenuController::onSurfaceMinZ(float minZ)
{
    const float depthMax = std::isfinite(minZ) ? -minZ : std::numeric_limits<float>::quiet_NaN();
    if (sameDepth(depthMax, surfaceDepthMax_)) {
        return;
    }

    surfaceDepthMax_ = depthMax;
    emit surfaceDepthRangeChanged();
}

void IsobathsViewControlMenuController::onSurfaceMaxZ(float maxZ)
{
    const float depthMin = std::isfinite(maxZ) ? -maxZ : std::numeric_limits<float>::quiet_NaN();
    if (sameDepth(depthMin, surfaceDepthMin_)) {
        return;
    }

    surfaceDepthMin_ = depthMin;
    emit surfaceDepthRangeChanged();
}

void IsobathsViewControlMenuController::onSurfaceCleared()
{
    resetSurfaceExportEstimate();

    if (std::isnan(surfaceDepthMin_) && std::isnan(surfaceDepthMax_)) {
        return;
    }

    surfaceDepthMin_ = std::numeric_limits<float>::quiet_NaN();
    surfaceDepthMax_ = std::numeric_limits<float>::quiet_NaN();
    emit surfaceDepthRangeChanged();
}

QVariantMap IsobathsViewControlMenuController::pipelineStatus() const
{
    return pipelineStatus_;
}

bool IsobathsViewControlMenuController::statusMonitorEnabled() const
{
    return statusMonitorEnabled_;
}

void IsobathsViewControlMenuController::setStatusMonitorEnabled(bool state)
{
    if (statusMonitorEnabled_ == state) {
        return;
    }

    statusMonitorEnabled_ = state;

    if (statusMonitorEnabled_) {
        statusTimer_.setInterval(statusDetailedPolling_ ? kStatusPollMs : kStatusCollapsedPollMs);
        statusTimer_.start();
        refreshPipelineStatus();
    }
    else {
        statusTimer_.stop();
        statusRequestPending_ = false;
        pipelineStatus_.clear();
        emit pipelineStatusChanged();
    }

    emit statusMonitorEnabledChanged();
}

bool IsobathsViewControlMenuController::statusDetailedPolling() const
{
    return statusDetailedPolling_;
}

void IsobathsViewControlMenuController::setStatusDetailedPolling(bool state)
{
    if (statusDetailedPolling_ == state) {
        return;
    }

    statusDetailedPolling_ = state;

    if (statusMonitorEnabled_) {
        statusTimer_.setInterval(statusDetailedPolling_ ? kStatusPollMs : kStatusCollapsedPollMs);

        if (statusDetailedPolling_) {
            refreshPipelineStatus();
        }
    }

    emit statusDetailedPollingChanged();
}

void IsobathsViewControlMenuController::refreshPipelineStatus()
{
    if (!dataProcessorPtr_ || statusRequestPending_) {
        return;
    }

    statusRequestPending_ = true;
    QMetaObject::invokeMethod(dataProcessorPtr_, "requestPipelineStats", Qt::QueuedConnection);
}

void IsobathsViewControlMenuController::onPipelineStats(const QVariantMap& stats)
{
    statusRequestPending_ = false;
    ++pipelineStatusSeq_;
    emit pipelineStatusSeqChanged();

    if (!statusMonitorEnabled_) {
        return;
    }

    QVariantMap next = stats;
    next["isobathsRequested"] = visibility_;
    next["processEnabled"]    = processState_;

    if (graphicsSceneViewPtr_) {
        if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
            next["renderTiles"]       = surfacePtr->getRenderTilesCount();
            next["renderIsoLabels"]   = surfacePtr->getRenderIsoLabelsCount();
            next["renderSurfaceStep"] = surfacePtr->getRenderSurfaceStep();
            next["renderIsobathsOn"]  = surfacePtr->getIVisible();
            next["renderMosaicOn"]    = surfacePtr->getMVisible();
        }
    }

    if (next == pipelineStatus_) {
        return;
    }

    pipelineStatus_ = next;

    emit pipelineStatusChanged();
}

void IsobathsViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader");
}

void IsobathsViewControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (graphicsSceneViewPtr_) {
                graphicsSceneViewPtr_->setIsUpdateSurface(processState_);

                if (dataProcessorPtr_) {
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateSurface",                Qt::QueuedConnection, Q_ARG(bool,  processState_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateIsobaths",               Qt::QueuedConnection, Q_ARG(bool,  processState_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceIsobathsStepSize",      Qt::QueuedConnection, Q_ARG(float, surfaceLineStepSize_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsLabelStepSize",        Qt::QueuedConnection, Q_ARG(float, labelStepSize_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceColorTableThemeById",   Qt::QueuedConnection, Q_ARG(int,   themeId_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceEdgeLimit",             Qt::QueuedConnection, Q_ARG(int,   edgeLimit_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setExtraWidth",                   Qt::QueuedConnection, Q_ARG(int,   extraWidth_));
                }

                if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
                    surfacePtr->setIVisible(visibility_);
                    surfacePtr->setBandedColors(bandedColors_);
                    surfacePtr->setIsobathsLabelStepSize(labelStepSize_);
                }
                //if (auto isobathsViewPtr = graphicsSceneViewPtr_->getIsobathsViewPtr(); isobathsViewPtr) {
                //    isobathsViewPtr->setVisible(visibility_);
                //}
            }
        };
    }
}

void IsobathsViewControlMenuController::onIsobathsVisibilityCheckBoxCheckedChanged(bool checked)
{
    visibility_ = checked;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setIVisible(checked);
        //graphicsSceneViewPtr_->getIsobathsViewPtr()->setVisible(checked);

        if (visibility_) {
            if (dataProcessorPtr_) {
                if (checked) {
                    //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kSurface));
                    //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kIsobaths));
                    //QMetaObject::invokeMethod(dataProcessorPtr_, "onIsobathsUpdated", Qt::QueuedConnection);
                }
            }
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onUpdateIsobathsButtonClicked()
{
    //if (graphicsSceneViewPtr_) {
    //    if (dataProcessorPtr_) {
    //        QMetaObject::invokeMethod(dataProcessorPtr_, "onBottomTrackAdded", Qt::QueuedConnection,
    //                                  Q_ARG(QVector<int>, graphicsSceneViewPtr_->bottomTrack()->getAllIndxs()),
    //                                  Q_ARG(bool, false));
    //    }
    //}
}

void IsobathsViewControlMenuController::onTrianglesVisible(bool state)
{
    trianglesVisible_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onEdgesVisible(bool state)
{
    edgesVisible_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onSetSurfaceLineStepSize(float val)
{
    const bool changed = !qFuzzyCompare(1.0f + val, 1.0f + surfaceLineStepSize_);
    if (surfaceLineStepApplied_ && !changed) {
        return;
    }
    surfaceLineStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceIsobathsStepSize", Qt::QueuedConnection, Q_ARG(float, surfaceLineStepSize_));
            surfaceLineStepApplied_ = true;
        }
    }
    else {
        tryInitPendingLambda();
    }

    if (changed) {
        emit isobathStepChanged();
    }
}

void IsobathsViewControlMenuController::onSetLabelStepSize(int val)
{
    labelStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsLabelStepSize", Qt::QueuedConnection, Q_ARG(int , labelStepSize_));
        }
        if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
            surfacePtr->setIsobathsLabelStepSize(labelStepSize_);
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onThemeChanged(int val)
{
    const bool changed = themeId_ != val;
    themeId_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceColorTableThemeById", Qt::QueuedConnection, Q_ARG(int, themeId_));
        }
    }
    else {
        tryInitPendingLambda();
    }

    if (changed) {
        emit surfacePaletteColorsChanged();
    }
}

void IsobathsViewControlMenuController::onDebugModeView(bool state)
{
    debugModeView_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onProcessStateChanged(bool state)
{
    processState_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->setIsUpdateSurface(processState_);

        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateSurface",  Qt::QueuedConnection, Q_ARG(bool, processState_));
            QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateIsobaths", Qt::QueuedConnection, Q_ARG(bool, processState_));
        }

        if (processState_) {
            graphicsSceneViewPtr_->onCameraMoved();
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onResetIsobathsButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType , DataProcessorType::kIsobaths));
        }

        //graphicsSceneViewPtr_->getIsobathsViewPtr()->clear();
    }
}

void IsobathsViewControlMenuController::onEdgeLimitChanged(int val)
{
    if (edgeLimitApplied_ && edgeLimit_ == val) {
        return;
    }
    edgeLimit_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceEdgeLimit", Qt::QueuedConnection, Q_ARG(int, edgeLimit_));
            edgeLimitApplied_ = true;
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onSetExtraWidth(int val)
{
    if (extraWidthApplied_ && extraWidth_ == val) {
        return;
    }
    extraWidth_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setExtraWidth", Qt::QueuedConnection, Q_ARG(int, extraWidth_));
            extraWidthApplied_ = true;
        }
    }
    else {
        tryInitPendingLambda();
    }
}

QVariantList IsobathsViewControlMenuController::themeStops(int index) const
{
    const QVector<QVector3D>& palette = IsobathUtils::colorPalette(index);
    QVariantList stops;
    const int count = palette.size();
    for (int i = 0; i < count; ++i) {
        const QVector3D& c = palette[i];
        QVariantMap stop;
        stop["pos"] = (count > 1) ? static_cast<double>(i) / (count - 1) : 0.0;
        stop["color"] = QColor::fromRgbF(c.x(), c.y(), c.z()).name();
        stops.append(stop);
    }
    return stops;
}

bool IsobathsViewControlMenuController::startSurfaceExport(const QString& path, int quality, bool extrapolation, double depthOffset, int format)
{
    if (exportRunning_ || !dataProcessorPtr_) {
        return false;
    }

#ifdef Q_OS_ANDROID
    const QString filePath = path;
#else
    QString filePath = QUrl(path).toLocalFile();
    if (filePath.isEmpty()) {
        filePath = path;
    }
#endif

    if (filePath.isEmpty()) {
        notifications.warning(tr("Surface export failed: empty path"));
        return false;
    }

    exportParams_ = SurfaceExportParams();
    exportParams_.path = filePath;
    exportParams_.zoom = SurfaceGridExport::zoomFromQuality(quality);
    exportParams_.extrapolation = extrapolation;
    exportParams_.depthOffset = static_cast<float>(depthOffset);
    exportParams_.format = format == 1 ? SurfaceExportFormat::kXyz : SurfaceExportFormat::kCsv;
    if (datasetPtr_) {
        exportParams_.llaRef = datasetPtr_->getLlaRef();
    }
    exportCancel_ = std::make_shared<std::atomic_bool>(false);
    exportParams_.cancel = exportCancel_;

    exportPercent_ = -1;
    setSurfaceExportRunning(true);

    if (exportParams_.format == SurfaceExportFormat::kXyz && !exportParams_.llaRef.isInit) {
        SurfaceExportResult result;
        result.status = SurfaceExportStatus::kNoGeoReference;
        result.nodeStepM = SurfaceGridExport::nodeStepMeters(exportParams_.zoom);
        finishSurfaceExport(result);
        return false;
    }

    estimateRefreshTimer_.stop();
    if (estimateRequestId_ != 0 || estimateWatcher_.isRunning()) {
        estimateRequestId_ = 0;
        estimateRevision_ = 0;
        estimateWatcher_.cancel();
        estimatePending_ = true;
    }

    exportRequestId_ = ++exportRequestCounter_;
    notifications.progress(tr("Surface export: preparing data"), kSurfaceExportTag, -1);

    if (!requestSurfaceExportSnapshot(exportRequestId_, 0)) {
        exportRequestId_ = 0;
        SurfaceExportResult result;
        result.status = SurfaceExportStatus::kNoData;
        finishSurfaceExport(result);
        return false;
    }
    return true;
}

void IsobathsViewControlMenuController::cancelSurfaceExport()
{
    if (!exportRunning_) {
        return;
    }

    if (exportWatcher_.isRunning()) {
        if (exportCancel_) {
            exportCancel_->store(true);
        }
        return;
    }

    exportRequestId_ = 0;
    SurfaceExportResult result;
    result.status = SurfaceExportStatus::kCanceled;
    finishSurfaceExport(result);
}

void IsobathsViewControlMenuController::requestSurfaceExportEstimate()
{
    if (!dataProcessorPtr_ || exportRunning_) {
        return;
    }

    if (!processingIdle_ || estimateRequestId_ != 0 || estimateWatcher_.isRunning()) {
        estimatePending_ = true;
        return;
    }

    estimatePending_ = false;
    estimateRequestId_ = ++exportRequestCounter_;
    const quint64 knownRevision = bottomTrackData().size() == estimateTrackSize_ ? estimateRevision_ : 0;
    if (!requestSurfaceExportSnapshot(estimateRequestId_, knownRevision)) {
        estimateRequestId_ = 0;
    }
}

double IsobathsViewControlMenuController::surfaceExportNodeStep(int quality) const
{
    return SurfaceGridExport::nodeStepMeters(SurfaceGridExport::zoomFromQuality(quality));
}

bool IsobathsViewControlMenuController::surfaceExportRunning() const
{
    return exportRunning_;
}

QVariantMap IsobathsViewControlMenuController::surfaceExportEstimate() const
{
    return exportEstimate_;
}

QVariantMap IsobathsViewControlMenuController::surfaceExportLastResult() const
{
    return exportLastResult_;
}

void IsobathsViewControlMenuController::onSurfaceExportSnapshotReady(int requestId, SurfaceExportSnapshotPtr snapshot)
{
    if (requestId != 0 && requestId == exportRequestId_) {
        exportRequestId_ = 0;
        if (snapshot) {
            snapshot->track = bottomTrackData();
        }
        exportWatcher_.setFuture(QtConcurrent::run(&SurfaceGridExport::writeFile, snapshot, exportParams_));
        return;
    }

    if (requestId == 0 || requestId != estimateRequestId_) {
        return;
    }

    estimateRequestId_ = 0;
    if (exportRunning_) {
        estimatePending_ = true;
        return;
    }
    if (!estimateActive_) {
        estimatePending_ = false;
        return;
    }

    const QVector<QVector3D> track = bottomTrackData();
    if (snapshot && snapshot->unchanged) {
        if (estimatePending_ || track.size() != estimateTrackSize_) {
            requestSurfaceExportEstimate();
        }
        return;
    }

    if (snapshot) {
        snapshot->track = track;
    }
    estimateRevision_ = snapshot ? snapshot->revision : 0;
    estimateTrackSize_ = track.size();
    estimateWatcher_.setFuture(QtConcurrent::run(&SurfaceGridExport::countNodes, snapshot, SurfaceGridExport::kEstimateZoom));
}

bool IsobathsViewControlMenuController::surfaceExportEstimateActive() const
{
    return estimateActive_;
}

void IsobathsViewControlMenuController::setSurfaceExportEstimateActive(bool state)
{
    if (estimateActive_ == state) {
        return;
    }

    estimateActive_ = state;
    if (estimateActive_) {
        requestSurfaceExportEstimate();
    }
    else {
        estimateRefreshTimer_.stop();
        estimatePending_ = false;
    }
    emit surfaceExportEstimateActiveChanged();
}

void IsobathsViewControlMenuController::onProcessingIdleChanged(bool idle)
{
    processingIdle_ = idle;
    if (!idle || !estimateActive_ || exportRunning_) {
        return;
    }

    if (estimatePending_) {
        requestSurfaceExportEstimate();
    }
    else if (!estimateRefreshTimer_.isActive()) {
        estimateRefreshTimer_.start();
    }
}

void IsobathsViewControlMenuController::onDataProcessorDestroyed()
{
    dataProcessorPtr_ = nullptr;
    estimateRefreshTimer_.stop();
    estimateRequestId_ = 0;

    if (exportRunning_ && exportRequestId_ != 0) {
        exportRequestId_ = 0;
        SurfaceExportResult result;
        result.status = SurfaceExportStatus::kCanceled;
        finishSurfaceExport(result);
    }
}

void IsobathsViewControlMenuController::onSurfaceExportProgress(int value)
{
    const int minimum = exportWatcher_.progressMinimum();
    const int maximum = exportWatcher_.progressMaximum();
    if (maximum <= minimum) {
        return;
    }

    const int percent = std::clamp((value - minimum) * 100 / (maximum - minimum), 0, 100);
    if (percent == exportPercent_) {
        return;
    }

    exportPercent_ = percent;
    notifications.progress(tr("Surface export: %1%").arg(percent), kSurfaceExportTag, percent);
}

void IsobathsViewControlMenuController::onSurfaceExportFinished()
{
    const QFuture<SurfaceExportResult> future = exportWatcher_.future();

    SurfaceExportResult result;
    if (future.resultCount() > 0) {
        result = future.result();
    }
    else {
        result.status = SurfaceExportStatus::kCanceled;
    }

    finishSurfaceExport(result);
}

void IsobathsViewControlMenuController::onSurfaceExportEstimateFinished()
{
    const QFuture<SurfaceExportResult> future = estimateWatcher_.future();

    const bool counted = estimateRevision_ != 0 && future.resultCount() > 0
                         && future.result().status != SurfaceExportStatus::kCanceled;
    if (counted) {
        const SurfaceExportResult result = future.result();
        const double nodeArea = result.nodeStepM * result.nodeStepM;
        QVariantMap estimate;
        estimate["ready"] = true;
        estimate["triangulationArea"] = static_cast<double>(result.triangulationNodes) * nodeArea;
        estimate["extrapolationArea"] = static_cast<double>(result.extrapolationNodes) * nodeArea;
        if (estimate != exportEstimate_) {
            exportEstimate_ = estimate;
            emit surfaceExportEstimateChanged();
        }
    }
    else {
        estimateRevision_ = 0;
    }

    if (estimatePending_ && estimateActive_) {
        requestSurfaceExportEstimate();
    }
}

void IsobathsViewControlMenuController::setSurfaceExportRunning(bool state)
{
    if (exportRunning_ == state) {
        return;
    }

    exportRunning_ = state;
    emit surfaceExportRunningChanged();
}

void IsobathsViewControlMenuController::finishSurfaceExport(const SurfaceExportResult& result)
{
    notifications.dismiss(kSurfaceExportTag);

    const QString path = exportParams_.path;
    const qint64 points = result.triangulationNodes + result.extrapolationNodes;
    QString status;
    switch (result.status) {
    case SurfaceExportStatus::kOk:
        status = QStringLiteral("ok");
        notifications.info(tr("Surface exported (%1 points):\n%2").arg(points).arg(QDir::toNativeSeparators(path)), path);
        break;
    case SurfaceExportStatus::kCanceled:
        status = QStringLiteral("canceled");
        notifications.info(tr("Surface export canceled"));
        break;
    case SurfaceExportStatus::kNoData:
        status = QStringLiteral("noData");
        notifications.warning(tr("Surface export: no surface data"));
        break;
    case SurfaceExportStatus::kNoGeoReference:
        status = QStringLiteral("noGeoReference");
        notifications.warning(tr("Surface export: XYZ needs geographic coordinates, and this data has no GNSS reference"));
        break;
    case SurfaceExportStatus::kFileError:
        status = QStringLiteral("fileError");
        notifications.warning(tr("Surface export failed: %1").arg(result.fileError));
        break;
    }

    QVariantMap last;
    last["status"] = status;
    last["path"] = path;
    last["zoom"] = exportParams_.zoom;
    last["format"] = exportParams_.format == SurfaceExportFormat::kXyz ? QStringLiteral("xyz") : QStringLiteral("csv");
    last["nodeStep"] = result.nodeStepM;
    last["triangulationNodes"] = result.triangulationNodes;
    last["extrapolationNodes"] = result.extrapolationNodes;
    exportLastResult_ = last;
    emit surfaceExportLastResultChanged();

    exportCancel_.reset();
    setSurfaceExportRunning(false);

    if (estimateActive_) {
        requestSurfaceExportEstimate();
    }
}

bool IsobathsViewControlMenuController::requestSurfaceExportSnapshot(int requestId, quint64 knownRevision)
{
    DataProcessor* dataProcessor = dataProcessorPtr_;
    return dataProcessor && QMetaObject::invokeMethod(dataProcessor, [dataProcessor, requestId, knownRevision]() {
        dataProcessor->requestSurfaceExportSnapshot(requestId, knownRevision);
    }, Qt::QueuedConnection);
}

QVector<QVector3D> IsobathsViewControlMenuController::bottomTrackData() const
{
    if (!graphicsSceneViewPtr_) {
        return {};
    }
    const auto bottomTrack = graphicsSceneViewPtr_->bottomTrack();
    return bottomTrack ? bottomTrack->cdata() : QVector<QVector3D>();
}

void IsobathsViewControlMenuController::resetSurfaceExportEstimate()
{
    estimateRevision_ = 0;
    estimateTrackSize_ = -1;
    estimateRequestId_ = 0;
    estimateWatcher_.cancel();

    if (!exportEstimate_.isEmpty()) {
        exportEstimate_.clear();
        emit surfaceExportEstimateChanged();
    }

    if (estimateActive_) {
        estimateRefreshTimer_.start();
    }
}
