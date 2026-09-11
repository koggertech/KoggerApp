#include "isobaths_view_control_menu_controller.h"
#include "scene3d_view.h"
#include "isobaths_defs.h"
#include <cmath>
#include <limits>
#include <QColor>
#include <QVariantMap>

namespace {

constexpr int kStatusPollMs = 700;
constexpr int kStatusCollapsedPollMs = 3000;

} // namespace


IsobathsViewControlMenuController::IsobathsViewControlMenuController(QObject* parent)
    : QmlComponentController(parent),
    graphicsSceneViewPtr_(nullptr),
    dataProcessorPtr_(nullptr),
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
        QObject::connect(dataProcessorPtr_, &DataProcessor::surfaceProcessingCleared,
                         this, &IsobathsViewControlMenuController::onSurfaceCleared,
                         Qt::UniqueConnection);
        QObject::connect(dataProcessorPtr_, &DataProcessor::allProcessingCleared,
                         this, &IsobathsViewControlMenuController::onSurfaceCleared,
                         Qt::UniqueConnection);
    }
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
    surfaceLineStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceIsobathsStepSize", Qt::QueuedConnection, Q_ARG(float, surfaceLineStepSize_));
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
    edgeLimit_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceEdgeLimit", Qt::QueuedConnection, Q_ARG(int, edgeLimit_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onSetExtraWidth(int val)
{
    extraWidth_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setExtraWidth", Qt::QueuedConnection, Q_ARG(int, extraWidth_));
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
