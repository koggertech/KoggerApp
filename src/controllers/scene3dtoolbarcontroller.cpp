#include "scene3dtoolbarcontroller.h"
#include <graphicsscene3dview.h>
#include <QmlObjectNames.h>

Scene3dToolBarController::Scene3dToolBarController(QObject *parent)
    : QmlComponentController(parent)
{}

void Scene3dToolBarController::onFitAllInViewButtonClicked()
{
    m_graphicsSceneView->fitAllInView();
}

void Scene3dToolBarController::onSetCameraIsometricViewButtonClicked()
{
    m_graphicsSceneView->setIsometricView();
}

void Scene3dToolBarController::onSetCameraMapViewButtonClicked() {
    m_graphicsSceneView->setMapView();
}

void Scene3dToolBarController::onBottomTrackVertexEditingModeButtonChecked(bool checked)
{
    if(checked)
        m_graphicsSceneView->setBottomTrackVertexSelectionMode();
    else
        m_graphicsSceneView->setIdleMode();
}

void Scene3dToolBarController::onCancelZoomButtonClicked()
{
    m_graphicsSceneView->setCancelZoomView();
}

void Scene3dToolBarController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void Scene3dToolBarController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::scene3dToolBar());
}
