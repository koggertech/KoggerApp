#include "scene3dcontrolmenucontroller.h"
#include <graphicsscene3dview.h>

Scene3DControlMenuController::Scene3DControlMenuController(QObject *parent)
    : QmlComponentController(parent)
{}

void Scene3DControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void Scene3DControlMenuController::onVerticalScaleSliderValueChanged(float value)
{
    if(!m_graphicsSceneView)
        return;

    m_graphicsSceneView->setVerticalScale(value);
}

void Scene3DControlMenuController::findComponent()
{

}

float Scene3DControlMenuController::verticalScale() const
{
    if(!m_graphicsSceneView)
        return 1.0f;

    return m_graphicsSceneView->verticalScale();
}
