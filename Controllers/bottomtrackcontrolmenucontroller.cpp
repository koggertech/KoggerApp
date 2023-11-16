#include "bottomtrackcontrolmenucontroller.h"
#include <graphicsscene3dview.h>
#include <bottomtrack.h>
#include <maxpointsfilter.h>
#include <nearestpointfilter.h>
#include <bottomtrackprovider.h>
#include <QmlObjectNames.h>

BottomTrackControlMenuController::BottomTrackControlMenuController(QObject* parent)
: QmlComponentController(parent)
{}

BottomTrackControlMenuController::~BottomTrackControlMenuController()
{}

void BottomTrackControlMenuController::onVisibilityCheckBoxCheckedChanged(bool checked)
{
    if(!m_graphicsSceneView)
        return;

    auto bottomTrack = m_graphicsSceneView->bottomTrack();

    QMetaObject::invokeMethod(reinterpret_cast <QObject*>(bottomTrack.get()), "setVisible", Q_ARG(bool, checked));
}

void BottomTrackControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void BottomTrackControlMenuController::setBottomTrackProvider(std::shared_ptr<BottomTrackProvider> provider)
{
    m_bottomTrackProvider = provider;
}

BottomTrack *BottomTrackControlMenuController::bottomTrack() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->bottomTrack().get();
}

void BottomTrackControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::bottomTrackControlMenu);
}
