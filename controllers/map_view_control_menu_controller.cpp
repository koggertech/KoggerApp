#include "map_view_control_menu_controller.h"
#include "graphicsscene3dview.h"
#include "map_view.h"


MapViewControlMenuController::MapViewControlMenuController(QObject *parent) :
    QmlComponentController(parent),
    m_graphicsSceneView(nullptr),
    pendingLambda_(nullptr),
    visibility_(false)
{ }

void MapViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;

    if (pendingLambda_) {
        pendingLambda_();
        pendingLambda_ = nullptr;
    }
}

void MapViewControlMenuController::onVisibilityChanged(bool state)
{
    visibility_ = state;

    if (m_graphicsSceneView) {
        m_graphicsSceneView->getMapViewPtr()->setVisible(visibility_);
        if (state) {
            m_graphicsSceneView->updateMapView();
        }
    }
    else {
        tryInitPendingLambda();
    }
}
void MapViewControlMenuController::onUpdateClicked()
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getMapViewPtr()->update();
    }
    else {
        tryInitPendingLambda();
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

void MapViewControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (m_graphicsSceneView) {
                if (auto mapPtr = m_graphicsSceneView->getMapViewPtr(); mapPtr) {
                    mapPtr->setVisible(visibility_);
                    mapPtr->update();
                }
            }
        };
    }
}
