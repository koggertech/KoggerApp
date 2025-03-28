#include "boattrackcontrolmenucontroller.h"

#include <memory>
#include "scene3d_view.h"
#include "boattrack.h"


BoatTrackControlMenuController::BoatTrackControlMenuController(QObject* parent)
    : QmlComponentController(parent)
{

}

BoatTrackControlMenuController::~BoatTrackControlMenuController()
{

}

void BoatTrackControlMenuController::onVisibilityCheckBoxCheckedChanged(bool checked)
{
    if (!m_graphicsSceneView) {
        return;
    }

    auto boatTrack = m_graphicsSceneView->boatTrack();
    QMetaObject::invokeMethod(reinterpret_cast<QObject*>(boatTrack.get()), "setVisible", Q_ARG(bool, checked));
}

void BoatTrackControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;

    if (!m_graphicsSceneView)
        return;    
}

BoatTrack *BoatTrackControlMenuController::boatTrack() const
{
    if (!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->boatTrack().get();
}

void BoatTrackControlMenuController::findComponent()
{
    // m_component = m_engine->findChild<QObject*>(QmlObjectNames::boatTrackControlMenu()); TODO: need to create control menu?
}
