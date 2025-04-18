#include "scene3d_toolbar_controller.h"
#include "scene3d_view.h"
#include "qml_object_names.h"


Scene3dToolBarController::Scene3dToolBarController(QObject *parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      pendingLambda_(nullptr),
      isVertexEditingMode_(false)
{}

void Scene3dToolBarController::onFitAllInViewButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->fitAllInView();
    }
}

void Scene3dToolBarController::onSetCameraIsometricViewButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->setIsometricView();
    }
}

void Scene3dToolBarController::onSetCameraMapViewButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->setMapView();
    }
}

void Scene3dToolBarController::onBottomTrackVertexEditingModeButtonChecked(bool checked)
{
    isVertexEditingMode_ = checked;

    if (graphicsSceneViewPtr_) {
        if (isVertexEditingMode_) {
            graphicsSceneViewPtr_->setBottomTrackVertexSelectionMode();
        }
        else {
            graphicsSceneViewPtr_->setIdleMode();
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void Scene3dToolBarController::onCancelZoomButtonClicked()
{
    graphicsSceneViewPtr_->setCancelZoomView();
}

void Scene3dToolBarController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
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
            if (graphicsSceneViewPtr_) {
                if (isVertexEditingMode_) {
                    graphicsSceneViewPtr_->setBottomTrackVertexSelectionMode();
                }
                else {
                    graphicsSceneViewPtr_->setIdleMode();
                }
            }
        };
    }
}
