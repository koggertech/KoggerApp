#include "map_view_control_menu_controller.h"
#include "graphicsscene3dview.h"
#include "map_view.h"


MapViewControlMenuController::MapViewControlMenuController(QObject *parent) :
    QmlComponentController(parent),
    m_graphicsSceneView(nullptr)
{ }

void MapViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void MapViewControlMenuController::onVisibilityChanged(bool state)
{
    qDebug() << "MapViewControlMenuController::onVisibilityChanged" << state;

    if (m_graphicsSceneView) {
        m_graphicsSceneView->getMapViewPtr()->setVisible(state);
    }
}
void MapViewControlMenuController::onUpdateClicked()
{
    qDebug() << "MapViewControlMenuController::onUpdateClicked";

    if (m_graphicsSceneView) {
        m_graphicsSceneView->getMapViewPtr()->update();
    }
}

MapView* MapViewControlMenuController::getMapViewPtr() const
{
    if (m_graphicsSceneView) {
        return m_graphicsSceneView->getMapViewPtr().get();
    }
    return nullptr;
}

void MapViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("mapViewControlMenu");
}
