#include "map_view_control_menu_controller.h"
#include "scene3d_view.h"
#include "map_view.h"


MapViewControlMenuController::MapViewControlMenuController(QObject *parent) :
    QmlComponentController(parent),
    graphicsSceneViewPtr_(nullptr),
    pendingLambda_(nullptr),
    visibility_(false)
{ }

void MapViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void MapViewControlMenuController::onVisibilityChanged(bool state)
{
    visibility_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getMapViewPtr()->setVisible(visibility_);
        if (visibility_) {
            graphicsSceneViewPtr_->updateMapView();
        }
    }
    else {
        tryInitPendingLambda();
    }
}
void MapViewControlMenuController::onUpdateClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getMapViewPtr()->update();
    }
    else {
        tryInitPendingLambda();
    }
}

MapView* MapViewControlMenuController::getMapViewPtr() const
{
    if (graphicsSceneViewPtr_) {
        return graphicsSceneViewPtr_->getMapViewPtr().get();
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
            if (graphicsSceneViewPtr_) {
                if (auto mapPtr = graphicsSceneViewPtr_->getMapViewPtr(); mapPtr) {
                    mapPtr->setVisible(visibility_);
                    mapPtr->update();
                }
            }
        };
    }
}
