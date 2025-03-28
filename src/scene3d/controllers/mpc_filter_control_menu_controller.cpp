#include "mpc_filter_control_menu_controller.h"
#include "qml_object_names.h"
#include "scene3d_view.h"
#include "bottom_track.h"
#include "max_points_filter.h"

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
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::mpcFilterControlMenu());
}
