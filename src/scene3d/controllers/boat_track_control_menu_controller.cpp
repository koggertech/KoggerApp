#include "boat_track_control_menu_controller.h"

#include "scene3d_view.h"
#include "boat_track.h"


BoatTrackControlMenuController::BoatTrackControlMenuController(QObject* parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      pendingLambda_(nullptr),
      visibility_(false)
{

}

BoatTrackControlMenuController::~BoatTrackControlMenuController()
{

}

void BoatTrackControlMenuController::onVisibilityCheckBoxCheckedChanged(bool checked)
{
    visibility_ = checked;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->boatTrack()->setVisible(checked);
    }
    else {
        tryInitPendingLambda();
    }
}

void BoatTrackControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

BoatTrack *BoatTrackControlMenuController::boatTrack() const
{
    if (graphicsSceneViewPtr_) {
        return graphicsSceneViewPtr_->boatTrack().get();
    }

    return nullptr;
}

void BoatTrackControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (graphicsSceneViewPtr_) {
                if (auto bTPtr = graphicsSceneViewPtr_->boatTrack(); bTPtr) {
                    bTPtr->setVisible(visibility_);
                }
            }
        };
    }
}

void BoatTrackControlMenuController::findComponent()
{
    // m_component = m_engine->findChild<QObject*>(QmlObjectNames::boatTrackControlMenu()); TODO: need to create control menu?
}
