#include "navigation_arrow_control_menu_controller.h"

#include "scene3d_view.h"


NavigationArrowControlMenuController::NavigationArrowControlMenuController(QObject* parent) :
    QmlComponentController(parent),
    graphicsSceneView_(nullptr),
    pendingLambda_(nullptr),
    isVisible_(false)
{

}

NavigationArrowControlMenuController::~NavigationArrowControlMenuController()
{

}

void NavigationArrowControlMenuController::onVisibilityCheckBoxCheckedChanged(bool checked)
{
    isVisible_ = checked;

    if (graphicsSceneView_) {
        if (auto nAPtr = graphicsSceneView_->getNavigationArrowPtr(); nAPtr) {
            nAPtr->setVisible(checked);
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void NavigationArrowControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsSceneView_ = sceneView;

    if (graphicsSceneView_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void NavigationArrowControlMenuController::findComponent()
{
    // m_component = m_engine->findChild<QObject*>(QmlObjectNames::NavigationArrowControlMenu());
}

void NavigationArrowControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this](){
            if (graphicsSceneView_) {
                if (auto nAPtr = graphicsSceneView_->getNavigationArrowPtr(); nAPtr) {
                    nAPtr->setVisible(isVisible_);
                }
            }
        };
    }
}
