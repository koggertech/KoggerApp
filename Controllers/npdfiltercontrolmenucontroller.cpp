#include "npdfiltercontrolmenucontroller.h"
#include <QmlObjectNames.h>
#include <graphicsscene3dview.h>
#include <bottomtrack.h>
#include <nearestpointfilter.h>
#include <bottomtrackprovider.h>

NpdFilterControlMenuController::NpdFilterControlMenuController(GraphicsScene3dView *sceneView, QObject *parent)
: QmlComponentController(parent)
, m_graphicsSceneView(sceneView)
{}

NpdFilterControlMenuController::~NpdFilterControlMenuController()
{}

void NpdFilterControlMenuController::onDistanceSpinBoxValueChanged(qreal value)
{
    if(!m_graphicsSceneView)
        return;

    auto bottomTrack = m_graphicsSceneView->bottomTrack();

    if(!bottomTrack)
        return;

    auto npdFilter = qobject_cast <NearestPointFilter*>(bottomTrack->filter());

    if(!npdFilter)
        return;

    QMetaObject::invokeMethod(npdFilter, "setDistance", Q_ARG(float, value));

    if(!m_bottomTrackProvider)
        return;

    QMetaObject::invokeMethod(bottomTrack.get(),
                              "setData",
                              Q_ARG(QVector<QVector3D>, m_bottomTrackProvider->getBottomTrack()),
                              Q_ARG(int, GL_LINE_STRIP));
}

void NpdFilterControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    if(m_graphicsSceneView == sceneView)
        return;

    m_graphicsSceneView = sceneView;
}

void NpdFilterControlMenuController::setBottomTrackProvider(std::shared_ptr<BottomTrackProvider> provider)
{
    m_bottomTrackProvider = provider;
}

void NpdFilterControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::npdFilterControlMenu);
}
