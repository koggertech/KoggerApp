#include "mpcfiltercontrolmenucontroller.h"
#include <QmlObjectNames.h>
#include <graphicsscene3dview.h>
#include <bottomtrack.h>
#include <maxpointsfilter.h>

MpcFilterControlMenuController::MpcFilterControlMenuController(GraphicsScene3dView *sceneView, QObject *parent)
    : QmlComponentController(parent)
    , m_graphicsSceneView(sceneView)
{}


void MpcFilterControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    if(m_graphicsSceneView == sceneView)
        return;

    m_graphicsSceneView = sceneView;
}

void MpcFilterControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::mpcFilterControlMenu);
}
