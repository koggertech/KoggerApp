#include "bottomtrackcontrolmenucontroller.h"
#include "scene3d_view.h"
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

void BottomTrackControlMenuController::onVisibleChannelComboBoxIndexChanged(int index)
{
    if(!m_graphicsSceneView)
        return;

    if(m_channelList.isEmpty())
        return;

    auto channelId = QString(m_channelList.at(index)).toInt();

    m_graphicsSceneView->bottomTrack()->setVisibleChannel(channelId);
}

void BottomTrackControlMenuController::onSurfaceUpdated()
{
    if (!m_graphicsSceneView)
        return;

    auto bottomTrack = m_graphicsSceneView->bottomTrack();

    QMetaObject::invokeMethod(reinterpret_cast<BottomTrack*>(bottomTrack.get()), "surfaceUpdated");
}

void BottomTrackControlMenuController::onSurfaceStateChanged(bool state)
{
    if (!m_graphicsSceneView)
        return;

    auto bottomTrack = m_graphicsSceneView->bottomTrack();

    QMetaObject::invokeMethod(reinterpret_cast<BottomTrack*>(bottomTrack.get()), "surfaceStateChanged", Q_ARG(bool, state));
}

void BottomTrackControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;

    if (!m_graphicsSceneView)
        return;

    QObject::connect(m_graphicsSceneView->bottomTrack().get(), &BottomTrack::epochListChanged,
                     this,                                     &BottomTrackControlMenuController::updateChannelList);
}

BottomTrack *BottomTrackControlMenuController::bottomTrack() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->bottomTrack().get();
}

QStringList BottomTrackControlMenuController::channelList() const
{
    return m_channelList;
}

int BottomTrackControlMenuController::visibleChannelIndex() const
{
    if(!m_graphicsSceneView)
        return -1;

    auto ch = m_graphicsSceneView->bottomTrack()->visibleChannel();

    return m_channelList.indexOf(QString::number(ch.channel));
}

void BottomTrackControlMenuController::updateChannelList()
{
    m_channelList.clear();

    if(!m_graphicsSceneView)
        return;

    auto channels = m_graphicsSceneView->bottomTrack()->channels();

    for(const auto& channel : qAsConst(channels))
        m_channelList << QString("%1").arg(channel.channel);

    Q_EMIT channelListUpdated();
}

void BottomTrackControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::bottomTrackControlMenu());
}
