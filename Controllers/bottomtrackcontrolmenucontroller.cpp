#include "bottomtrackcontrolmenucontroller.h"
#include <graphicsscene3dview.h>
#include <bottomtrack.h>
#include <maxpointsfilter.h>
#include <nearestpointfilter.h>
#include <QmlObjectNames.h>
#include <plotcash.h>

BottomTrackControlMenuController::BottomTrackControlMenuController(QObject* parent)
: QmlComponentController(parent)
    , m_channelListModel(new QStandardItemModel)
{
    auto roleNames = m_channelListModel->roleNames();
    roleNames.insert(Qt::UserRole + 1, "ChannelId");

    m_channelListModel->setItemRoleNames(roleNames);
}

BottomTrackControlMenuController::~BottomTrackControlMenuController()
{}

void BottomTrackControlMenuController::onVisibilityCheckBoxCheckedChanged(bool checked)
{
    if(!m_graphicsSceneView)
        return;

    auto bottomTrack = m_graphicsSceneView->bottomTrack();

    QMetaObject::invokeMethod(reinterpret_cast <QObject*>(bottomTrack.get()), "setVisible", Q_ARG(bool, checked));
}

//void BottomTrackControlMenuController::onRestoreBottomTrackButtonClicked()
//{
//    if(!m_graphicsSceneView)
//        return;
//
//    auto dataset = m_graphicsSceneView->dataset();
//
//    Q_EMIT dataset->dataUpdate();
//
//    // if(!dataset)
//    //     return;
//
//    // m_graphicsSceneView->bottomTrack()->setData(dataset->bottomTrack(), GL_LINE_STRIP);
//}

void BottomTrackControlMenuController::onVisibleChannelComboBoxIndexChanged(int index)
{
    if(!m_graphicsSceneView)
        return;

    auto channelId = m_channelListModel->data(m_channelListModel->index(index,0),Qt::UserRole+1);

    m_graphicsSceneView->bottomTrack()->setVisibleChannel(channelId.toInt());
}

void BottomTrackControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;

    if(!m_graphicsSceneView)
        return;

    QObject::connect(m_graphicsSceneView->bottomTrack().get(), &BottomTrack::epochListChanged,
                     this,                                     &BottomTrackControlMenuController::updateChannelListModel);
}

BottomTrack *BottomTrackControlMenuController::bottomTrack() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->bottomTrack().get();
}

QStandardItemModel* BottomTrackControlMenuController::channelListModel() const
{
    return m_channelListModel.get();
}

void BottomTrackControlMenuController::updateChannelListModel()
{
    m_channelListModel->clear();

    if(!m_graphicsSceneView)
        return;

    auto channels = m_graphicsSceneView->bottomTrack()->channels();

    for(const auto& channel : qAsConst(channels)){
        auto item = new QStandardItem(QString("%1").arg(channel.channel));
        item->setData(channel.channel, Qt::UserRole + 1);
        m_channelListModel->invisibleRootItem()->setChild(m_channelListModel->rowCount(), item);
    }
}

void BottomTrackControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::bottomTrackControlMenu);
}
