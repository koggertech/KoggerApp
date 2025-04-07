#include "npd_filter_control_menu_controller.h"
#include "qml_object_names.h"
#include "scene3d_view.h"
#include "bottom_track.h"
#include "nearest_point_filter.h"

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
