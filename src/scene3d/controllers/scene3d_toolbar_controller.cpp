#include "scene3d_toolbar_controller.h"
#include "scene3d_view.h"
#include "qml_object_names.h"


Scene3dToolBarController::Scene3dToolBarController(QObject *parent)
    : QmlComponentController(parent),
      graphicsScene3dViewPtr_(nullptr),
      pendingLambda_(nullptr),
      isVertexEditingMode_(false),
      trackLastData_(false),
      updateBottomTrack_(false)
{}

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
        graphicsScene3dViewPtr_->setUpdateBottomTrack(updateBottomTrack_);
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsScene3dViewPtr_ = sceneView;

    if (graphicsScene3dViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
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
                graphicsScene3dViewPtr_->setUpdateBottomTrack(updateBottomTrack_);
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
