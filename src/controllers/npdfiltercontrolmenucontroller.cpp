#include "npdfiltercontrolmenucontroller.h"
#include <QmlObjectNames.h>
#include <graphicsscene3dview.h>
#include <bottomtrack.h>
#include <nearestpointfilter.h>

NpdFilterControlMenuController::NpdFilterControlMenuController(GraphicsScene3dView *sceneView, QObject *parent)
: QmlComponentController(parent)
, m_graphicsSceneView(sceneView)
{}

NpdFilterControlMenuController::~NpdFilterControlMenuController()
{}

void NpdFilterControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    if(m_graphicsSceneView == sceneView)
        return;

    m_graphicsSceneView = sceneView;
}

void NpdFilterControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::npdFilterControlMenu());
}
