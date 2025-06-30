#include "bottom_track_control_menu_controller.h"
#include "scene3d_view.h"
#include "bottom_track.h"
#include "qml_object_names.h"


BottomTrackControlMenuController::BottomTrackControlMenuController(QObject* parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      pendingLambda_(nullptr),
      visibility_(false)
{}

BottomTrackControlMenuController::~BottomTrackControlMenuController()
{}

void BottomTrackControlMenuController::onVisibilityCheckBoxCheckedChanged(bool checked)
{
    visibility_ = checked;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->bottomTrack()->setVisible(checked);
    }
    else {
        tryInitPendingLambda();
    }
}

void BottomTrackControlMenuController::onSurfaceUpdated()
{
    if (!graphicsSceneViewPtr_)
        return;

    auto bottomTrack = graphicsSceneViewPtr_->bottomTrack();

    QMetaObject::invokeMethod(reinterpret_cast<BottomTrack*>(bottomTrack.get()), "surfaceUpdated");
}

void BottomTrackControlMenuController::onSurfaceStateChanged(bool state)
{
    if (!graphicsSceneViewPtr_)
        return;

    auto bottomTrack = graphicsSceneViewPtr_->bottomTrack();

    QMetaObject::invokeMethod(reinterpret_cast<BottomTrack*>(bottomTrack.get()), "surfaceStateChanged", Q_ARG(bool, state));
}

void BottomTrackControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

BottomTrack *BottomTrackControlMenuController::bottomTrack() const
{
    if(!graphicsSceneViewPtr_)
        return nullptr;

    return graphicsSceneViewPtr_->bottomTrack().get();
}

void BottomTrackControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (graphicsSceneViewPtr_) {
                if (auto bTPtr = graphicsSceneViewPtr_->bottomTrack(); bTPtr) {
                    bTPtr->setVisible(visibility_);
                }
            }
        };
    }
}

void BottomTrackControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::bottomTrackControlMenu());
}
