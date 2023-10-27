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

void Scene3dToolBarController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void Scene3dToolBarController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::scene3dToolBar);
}
