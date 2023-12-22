#include "bottomtrackcontrolmenucontroller.h"
#include <graphicsscene3dview.h>
#include <bottomtrack.h>
#include <maxpointsfilter.h>
#include <nearestpointfilter.h>
#include <QmlObjectNames.h>
#include <plotcash.h>

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

void BottomTrackControlMenuController::onRestoreBottomTrackButtonClicked()
{
    if(!m_graphicsSceneView)
        return;

    auto dataset = m_graphicsSceneView->dataset();

    Q_EMIT dataset->dataUpdate();

    // if(!dataset)
    //     return;

    // m_graphicsSceneView->bottomTrack()->setData(dataset->bottomTrack(), GL_LINE_STRIP);
}

void BottomTrackControlMenuController::onVisibleChannelComboBoxIndexChanged(int index)
{
    if(!m_graphicsSceneView)
        return;

    m_graphicsSceneView->bottomTrack()->setVisibleChannel(index);
}

void BottomTrackControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

BottomTrack *BottomTrackControlMenuController::bottomTrack() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->bottomTrack().get();
}

QStringListModel* BottomTrackControlMenuController::channelListModel() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    auto model = m_graphicsSceneView->bottomTrack()->channelListModel();

    if(model.expired())
        return nullptr;

    return model.lock().get();
}

void BottomTrackControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::bottomTrackControlMenu);
}
