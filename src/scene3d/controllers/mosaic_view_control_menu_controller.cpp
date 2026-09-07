#include "mosaic_view_control_menu_controller.h"

#include "scene3d_view.h"
#include "data_processor.h"
#include "draw_utils.h"
#include <QColor>
#include <QVariantMap>

namespace {

constexpr int kStatusPollMs = 700;
constexpr int kStatusCollapsedPollMs = 3000;
constexpr int kStatusProbeWindow = 256;

} // namespace


MosaicViewControlMenuController::MosaicViewControlMenuController(QObject *parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      dataProcessorPtr_(nullptr),
      pendingLambda_(nullptr),
      visibility_(false),
      usingFilter_(true),
      gridVisible_(false),
      measLineVisible_(true),
      resolution_(10.0f), // pixPerMeters
      updateState_(false),
      themeId_(0),
      lowLevel_(10.0f),
      highLevel_(90.0f),
      lAngleOffset_(0.0f),
      rAngleOffset_(0.0f),
      statusMonitorEnabled_(false),
      statusRequestPending_(false),
      statusDetailedPolling_(false)
{
    statusTimer_.setInterval(kStatusPollMs);
    QObject::connect(&statusTimer_, &QTimer::timeout, this, &MosaicViewControlMenuController::refreshPipelineStatus);
}

void MosaicViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void MosaicViewControlMenuController::setDataProcessorPtr(DataProcessor *dataProcessorPtr)
{
    dataProcessorPtr_ = dataProcessorPtr;

    if (dataProcessorPtr_) {
        QObject::connect(dataProcessorPtr_, &DataProcessor::mosaicStats,
                         this, &MosaicViewControlMenuController::onMosaicStats,
                         Qt::UniqueConnection);
    }
}

QVariantMap MosaicViewControlMenuController::pipelineStatus() const
{
    return pipelineStatus_;
}

bool MosaicViewControlMenuController::statusMonitorEnabled() const
{
    return statusMonitorEnabled_;
}

void MosaicViewControlMenuController::setStatusMonitorEnabled(bool state)
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

bool MosaicViewControlMenuController::statusDetailedPolling() const
{
    return statusDetailedPolling_;
}

void MosaicViewControlMenuController::setStatusDetailedPolling(bool state)
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

void MosaicViewControlMenuController::refreshPipelineStatus()
{
    if (!dataProcessorPtr_ || statusRequestPending_) {
        return;
    }

    statusRequestPending_ = true;
    QMetaObject::invokeMethod(dataProcessorPtr_, "requestMosaicStats", Qt::QueuedConnection,
                              Q_ARG(int, kStatusProbeWindow));
}

void MosaicViewControlMenuController::onMosaicStats(const QVariantMap& stats)
{
    statusRequestPending_ = false;

    if (!statusMonitorEnabled_) {
        return;
    }

    QVariantMap next = stats;
    next["mosaicRequested"] = visibility_;
    next["probeWindow"]     = kStatusProbeWindow;
    next["levelLow"]        = lowLevel_;
    next["levelHigh"]       = highLevel_;
    next["gridVisible"]     = gridVisible_;
    next["measLineVisible"] = measLineVisible_;

    if (graphicsSceneViewPtr_) {
        if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
            next["renderTiles"]      = surfacePtr->getRenderTilesCount();
            next["renderMosaicOn"]   = surfacePtr->getMVisible();
            next["renderIsobathsOn"] = surfacePtr->getIVisible();
        }
    }

    if (next == pipelineStatus_) {
        return;
    }

    pipelineStatus_ = next;

    emit pipelineStatusChanged();
}

void MosaicViewControlMenuController::onVisibilityChanged(bool state)
{
    visibility_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setMVisible(visibility_);
        //graphicsSceneViewPtr_->getIsobathsViewPtr()->setMVisible(visibility_);

        if (state) {
                //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kSurface));
                //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kMosaic));
                //QMetaObject::invokeMethod(dataProcessorPtr_, "onMosaicUpdated", Qt::QueuedConnection);
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onUseFilterChanged(bool state)
{
    usingFilter_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onGridVisibleChanged(bool state)
{
    gridVisible_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onMeasLineVisibleChanged(bool state)
{
    measLineVisible_ = state;

    if (graphicsSceneViewPtr_) {
        if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
            surfacePtr->setTraceVisible(measLineVisible_);
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onClearClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->clear();
    }
}

void MosaicViewControlMenuController::onUpdateStateChanged(bool state)
{
    updateState_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->setIsUpdateMosaic(updateState_);

        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateMosaic", Qt::QueuedConnection, Q_ARG(bool, updateState_));
        }
        if (updateState_) {
            graphicsSceneViewPtr_->onCameraMoved();
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onThemeChanged(int val)
{
    themeId_ = val + 1;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicTheme", Qt::QueuedConnection, Q_ARG(int, themeId_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

QVariantList MosaicViewControlMenuController::themeStops(int index) const
{
    mosaic::PlotColorTable table;
    table.setTheme(index + 1);   // combo index → theme id (matches onThemeChanged)
    const QVector<QRgb> ramp = table.getColorTable();
    QVariantList stops;
    const int rampSize = ramp.size();
    if (rampSize <= 0)
        return stops;

    const int samples = 8;
    for (int s = 0; s < samples; ++s) {
        const double pos = (samples > 1) ? static_cast<double>(s) / (samples - 1) : 0.0;
        const int rampIndex = qBound(0, qRound(pos * (rampSize - 1)), rampSize - 1);
        QVariantMap stop;
        stop["pos"] = pos;
        stop["color"] = QColor(ramp[rampIndex]).name();
        stops.append(stop);
    }
    return stops;
}

void MosaicViewControlMenuController::onLevelChanged(float lowLevel, float highLevel)
{
    lowLevel_ = lowLevel;
    highLevel_ = highLevel;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicLevels", Qt::QueuedConnection, Q_ARG(float, lowLevel_), Q_ARG(float, highLevel_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onUpdateClicked()
{
    //if (graphicsSceneViewPtr_) {
    //    if (dataProcessorPtr_) {
    //        //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kSurface));
    //        QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kMosaic));
    //        QMetaObject::invokeMethod(dataProcessorPtr_, "onBottomTrackAdded", Qt::QueuedConnection,
    //                                  Q_ARG(QVector<int>, graphicsSceneViewPtr_->bottomTrack()->getAllIndxs()),
    //                                  Q_ARG(bool, true));
    //    }
    //}
}

void MosaicViewControlMenuController::onSetLAngleOffset(float val)
{
    lAngleOffset_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicLAngleOffset", Qt::QueuedConnection, Q_ARG(float, lAngleOffset_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onSetRAngleOffset(float val)
{
    rAngleOffset_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicRAngleOffset", Qt::QueuedConnection, Q_ARG(float, rAngleOffset_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onSetResolution(float val)
{
    resolution_ = val;

    return;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicTileResolution", Qt::QueuedConnection, Q_ARG(float, resolution_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this](){
            if (graphicsSceneViewPtr_) {
                graphicsSceneViewPtr_->setIsUpdateMosaic(updateState_);

                if (dataProcessorPtr_) {
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateMosaic",              Qt::QueuedConnection, Q_ARG(bool,  updateState_));
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicTheme",               Qt::QueuedConnection, Q_ARG(int,   themeId_));
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicLevels",              Qt::QueuedConnection, Q_ARG(float, lowLevel_), Q_ARG(float, highLevel_));
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicLAngleOffset",        Qt::QueuedConnection, Q_ARG(float, lAngleOffset_));
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicRAngleOffset",        Qt::QueuedConnection, Q_ARG(float, rAngleOffset_));
                   //QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicTileResolution",      Qt::QueuedConnection, Q_ARG(float, resolution_));
                }

                if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
                    surfacePtr->setMVisible(visibility_);
                    surfacePtr->setTraceVisible(measLineVisible_);
                }
                // if (auto isobathsPtr = graphicsSceneViewPtr_->getIsobathsViewPtr(); isobathsPtr) {
                //     isobathsPtr->setMVisible(visibility_);
                // }
                if (updateState_) {
                    graphicsSceneViewPtr_->onCameraMoved();
                }
            }
        };
    }
}

void MosaicViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("mosaicViewControlMenu");
}
