#include "mpcfiltercontrolmenucontroller.h"
#include <QmlObjectNames.h>
#include <graphicsscene3dview.h>
#include <bottomtrack.h>
#include <maxpointsfilter.h>
#include <bottomtrackprovider.h>

MpcFilterControlMenuController::MpcFilterControlMenuController(GraphicsScene3dView *sceneView, QObject *parent)
    : QmlComponentController(parent)
    , m_graphicsSceneView(sceneView)
{}

void MpcFilterControlMenuController::onPointsCountSpinBoxValueChanged(qreal value)
{
    if(!m_graphicsSceneView)
        return;

    auto bottomTrack = m_graphicsSceneView->bottomTrack();

    if(!bottomTrack)
        return;

    auto mpcFilter = qobject_cast <MaxPointsFilter*>(bottomTrack->filter());

    if(!mpcFilter)
        return;

    QMetaObject::invokeMethod(mpcFilter, "setDistance", Q_ARG(float, value));

    if(!m_bottomTrackProvider)
        return;

    QMetaObject::invokeMethod(bottomTrack.get(),
                              "setData",
                              Q_ARG(QVector <QVector3D>, m_bottomTrackProvider->getBottomTrack()),
                              Q_ARG(int, GL_LINE_STRIP));
}

void MpcFilterControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    if(m_graphicsSceneView == sceneView)
        return;

    m_graphicsSceneView = sceneView;
}

void MpcFilterControlMenuController::setBottomTrackProvider(std::shared_ptr<BottomTrackProvider> provider)
{
    m_bottomTrackProvider = provider;
}

void MpcFilterControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::mpcFilterControlMenu);
}
