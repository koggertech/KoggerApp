#include "scene3d_toolbar_controller.h"
#include "scene3d_view.h"
#include "qml_object_names.h"
#include <cmath>


Scene3dToolBarController::Scene3dToolBarController(QObject *parent)
    : QmlComponentController(parent),
      graphicsScene3dViewPtr_(nullptr),
      pendingLambda_(nullptr),
      isVertexEditingMode_(false),
      trackLastData_(false),
      updateBottomTrack_(false),
      gridVisibility_(true),
      useAngleLocation_(false),
      navigatorViewLocation_(false),
      isNorth_(false),
      compass_(true),
      compassPos_(1),
      compassSize_(1),
      shadowsEnabled_(true),
      shadowVectorX_(0.40f),
      shadowVectorY_(0.40f),
      shadowVectorZ_(0.40f),
      shadowIntensity_(1.00f),
      shadowAmbient_(0.35f),
      shadowHighlight_(0.70f),
      planeGridType_(false),
      planeGridCircleSize_(1),
      planeGridCircleStep_(1),
      planeGridCircleAngle_(1),
      planeGridCircleLabels_(true),
      rulerEnabled_(false),
      geoJsonEnabled_(false),
      forceSingleZoomEnabled_(false),
      forceSingleZoomValue_(5),
      syncLoupeVisible_(false),
      syncLoupeSize_(1),
      syncLoupeZoom_(1),
      suppressForceSingleZoomUiCallback_(false)
{}

void Scene3dToolBarController::setQmlEngine(QObject *engine)
{
    QmlComponentController::setQmlEngine(engine);

    syncForceSingleZoomUi(forceSingleZoomEnabled_, forceSingleZoomValue_);
}

void Scene3dToolBarController::onFitAllInViewButtonClicked()
{
    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->fitAllInView();
    }
}

void Scene3dToolBarController::onSetCameraIsometricViewButtonClicked()
{
    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setIsometricView();
    }
}

void Scene3dToolBarController::onSetCameraMapViewButtonClicked()
{
    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setMapView();
    }
}

void Scene3dToolBarController::onBottomTrackVertexEditingModeButtonChecked(bool checked)
{
    isVertexEditingMode_ = checked;

    if (graphicsScene3dViewPtr_) {
        if (isVertexEditingMode_) {
            graphicsScene3dViewPtr_->setBottomTrackVertexSelectionMode();
        }
        else {
            graphicsScene3dViewPtr_->setIdleMode();
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onResetProcessingButtonClicked()
{
    if (dataProcessorPtr_) {
        QMetaObject::invokeMethod(dataProcessorPtr_, "resetProcessingPipeline", Qt::QueuedConnection);
    }

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->clearSurfaceViewRender();
    }
}

void Scene3dToolBarController::onCancelZoomButtonClicked()
{
    graphicsScene3dViewPtr_->setCancelZoomView();
}

void Scene3dToolBarController::onTrackLastDataCheckButtonCheckedChanged(bool state)
{
    trackLastData_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setTrackLastData(trackLastData_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onUpdateBottomTrackCheckButtonCheckedChanged(bool state)
{
    updateBottomTrack_ = state;

    if (graphicsScene3dViewPtr_) {

        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateBottomTrack", Qt::QueuedConnection, Q_ARG(bool, updateBottomTrack_));
        }

    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onGridVisibilityCheckedChanged(bool state)
{
    gridVisibility_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setGridVisibility(gridVisibility_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onUseAngleLocationButtonChanged(bool state)
{
    useAngleLocation_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setUseAngleLocation(useAngleLocation_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onNavigatorLocationButtonChanged(bool state)
{
    navigatorViewLocation_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setNavigatorViewLocation(navigatorViewLocation_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onIsNorthLocationButtonChanged(bool state)
{
    isNorth_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setIsNorth(isNorth_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onCompassButtonChanged(bool state)
{
    compass_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setCompassState(compass_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onCompassPosChanged(int pos)
{
    compassPos_ = pos;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setCompassPos(compassPos_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onCompassSizeChanged(int size)
{
    compassSize_ = size;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setCompassSize(compassSize_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onShadowsEnabledChanged(bool state)
{
    shadowsEnabled_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setShadowsEnabled(shadowsEnabled_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onShadowVectorXChanged(float value)
{
    if (!std::isfinite(value)) {
        return;
    }
    shadowVectorX_ = value;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setShadowVectorX(shadowVectorX_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onShadowVectorYChanged(float value)
{
    if (!std::isfinite(value)) {
        return;
    }
    shadowVectorY_ = value;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setShadowVectorY(shadowVectorY_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onShadowVectorZChanged(float value)
{
    if (!std::isfinite(value)) {
        return;
    }
    shadowVectorZ_ = value;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setShadowVectorZ(shadowVectorZ_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onShadowIntensityChanged(float value)
{
    Q_UNUSED(value);
    constexpr float kFixedShadowIntensity = 1.0f;
    shadowIntensity_ = kFixedShadowIntensity;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setShadowIntensity(kFixedShadowIntensity);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onShadowAmbientChanged(float value)
{
    if (!std::isfinite(value)) {
        return;
    }
    shadowAmbient_ = qBound(0.0f, value, 1.0f);

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setShadowAmbient(shadowAmbient_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onShadowHighlightChanged(float value)
{
    if (!std::isfinite(value)) {
        return;
    }
    shadowHighlight_ = qBound(0.0f, value, 1.0f);

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setShadowHighlight(shadowHighlight_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onPlaneGridTypeChanged(bool val)
{
    planeGridType_ = val;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setPlaneGridType(planeGridType_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onPlaneGridCircleGridSizeChanged(int val)
{
    planeGridCircleSize_ = val;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setPlaneGridCircleSize(planeGridCircleSize_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onPlaneGridCircleGridStepChanged(int val)
{
    planeGridCircleStep_ = val;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setPlaneGridCircleStep(planeGridCircleStep_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onPlaneGridCircleGridAngleChanged(int val)
{
    planeGridCircleAngle_ = val;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setPlaneGridCircleAngle(planeGridCircleAngle_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onPlaneGridCircleGridLabelsChanged(bool state)
{
    planeGridCircleLabels_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setPlaneGridCircleLabels(planeGridCircleLabels_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onRulerModeChanged(bool enabled)
{
    rulerEnabled_ = enabled;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setRulerEnabled(rulerEnabled_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onGeoJsonModeChanged(bool enabled)
{
    geoJsonEnabled_ = enabled;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setGeoJsonEnabled(geoJsonEnabled_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onForceSingleZoomCheckedChanged(bool state)
{
    if (suppressForceSingleZoomUiCallback_) {
        return;
    }

    forceSingleZoomEnabled_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setForceSingleZoomEnabled(forceSingleZoomEnabled_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onForceSingleZoomValueChanged(int zoom)
{
    if (suppressForceSingleZoomUiCallback_) {
        return;
    }

    forceSingleZoomValue_ = zoom;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setForceSingleZoomValue(forceSingleZoomValue_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onSyncLoupeVisibleChanged(bool state)
{
    syncLoupeVisible_ = state;

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setSyncLoupeVisible(syncLoupeVisible_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onSyncLoupeSizeChanged(int size)
{
    syncLoupeSize_ = qBound(1, size, 3);

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setSyncLoupeSize(syncLoupeSize_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onSyncLoupeZoomChanged(int zoom)
{
    syncLoupeZoom_ = qBound(1, zoom, 3);

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setSyncLoupeZoom(syncLoupeZoom_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsScene3dViewPtr_ = sceneView;

    if (graphicsScene3dViewPtr_) {
        connect(graphicsScene3dViewPtr_,
                &GraphicsScene3dView::forceSingleZoomAutoStateChanged,
                this,
                &Scene3dToolBarController::onForceSingleZoomAutoStateChanged,
                Qt::QueuedConnection);

        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void Scene3dToolBarController::setDataProcessorPtr(DataProcessor *dataProcessorPtr)
{
    dataProcessorPtr_ = dataProcessorPtr;
}

void Scene3dToolBarController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::scene3dToolBar());
}

void Scene3dToolBarController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (graphicsScene3dViewPtr_) {
                graphicsScene3dViewPtr_->setTrackLastData(trackLastData_);
                graphicsScene3dViewPtr_->setGridVisibility(gridVisibility_);
                graphicsScene3dViewPtr_->setNavigatorViewLocation(navigatorViewLocation_);
                graphicsScene3dViewPtr_->setUseAngleLocation(useAngleLocation_);
                graphicsScene3dViewPtr_->setIsNorth(isNorth_);
                graphicsScene3dViewPtr_->setCompassState(compass_);
                graphicsScene3dViewPtr_->setCompassPos(compassPos_);
                graphicsScene3dViewPtr_->setCompassSize(compassSize_);
                graphicsScene3dViewPtr_->setShadowsEnabled(shadowsEnabled_);
                graphicsScene3dViewPtr_->setShadowVectorX(shadowVectorX_);
                graphicsScene3dViewPtr_->setShadowVectorY(shadowVectorY_);
                graphicsScene3dViewPtr_->setShadowVectorZ(shadowVectorZ_);
                graphicsScene3dViewPtr_->setShadowIntensity(shadowIntensity_);
                graphicsScene3dViewPtr_->setShadowAmbient(shadowAmbient_);
                graphicsScene3dViewPtr_->setShadowHighlight(shadowHighlight_);
                graphicsScene3dViewPtr_->setPlaneGridType(planeGridType_);
                graphicsScene3dViewPtr_->setPlaneGridCircleSize(planeGridCircleSize_);
                graphicsScene3dViewPtr_->setPlaneGridCircleStep(planeGridCircleStep_);
                graphicsScene3dViewPtr_->setPlaneGridCircleAngle(planeGridCircleAngle_);
                graphicsScene3dViewPtr_->setPlaneGridCircleLabels(planeGridCircleLabels_);
                graphicsScene3dViewPtr_->setRulerEnabled(rulerEnabled_);
                graphicsScene3dViewPtr_->setGeoJsonEnabled(geoJsonEnabled_);
                graphicsScene3dViewPtr_->setForceSingleZoomEnabled(forceSingleZoomEnabled_);
                graphicsScene3dViewPtr_->setForceSingleZoomValue(forceSingleZoomValue_);
                graphicsScene3dViewPtr_->setSyncLoupeVisible(syncLoupeVisible_);
                graphicsScene3dViewPtr_->setSyncLoupeSize(syncLoupeSize_);
                graphicsScene3dViewPtr_->setSyncLoupeZoom(syncLoupeZoom_);

                if (dataProcessorPtr_) {
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateBottomTrack", Qt::QueuedConnection, Q_ARG(bool, updateBottomTrack_));
                }

                if (isVertexEditingMode_) {
                    graphicsScene3dViewPtr_->setBottomTrackVertexSelectionMode();
                }
                else {
                    graphicsScene3dViewPtr_->setIdleMode();
                }
            }
        };
    }
}

void Scene3dToolBarController::onForceSingleZoomAutoStateChanged(bool active)
{
    if (active) {
        forceSingleZoomEnabled_ = true;
        forceSingleZoomValue_ = 5;
    } else {
        forceSingleZoomEnabled_ = false;
    }

    syncForceSingleZoomUi(forceSingleZoomEnabled_, forceSingleZoomValue_);

    if (graphicsScene3dViewPtr_) {
        graphicsScene3dViewPtr_->setForceSingleZoomEnabled(forceSingleZoomEnabled_);
        graphicsScene3dViewPtr_->setForceSingleZoomValue(forceSingleZoomValue_);
        graphicsScene3dViewPtr_->onCameraMoved();
    }
}

void Scene3dToolBarController::syncForceSingleZoomUi(bool enabled, int zoom)
{
    if (!m_engine) {
        return;
    }

    QObject* checkObj = m_engine->findChild<QObject*>(QStringLiteral("forceSingleZoomCheckButton"));
    QObject* spinObj  = m_engine->findChild<QObject*>(QStringLiteral("forceSingleZoomSpinBox"));

    suppressForceSingleZoomUiCallback_ = true;
    if (checkObj) {
        checkObj->setProperty("checked", enabled);
    }
    if (spinObj) {
        spinObj->setProperty("value", zoom);
    }
    suppressForceSingleZoomUiCallback_ = false;
}
