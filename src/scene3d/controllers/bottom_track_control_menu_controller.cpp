#include "bottom_track_control_menu_controller.h"
#include "scene3d_view.h"
#include "bottom_track.h"
#include "qml_object_names.h"
#include "plotcash.h"

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

void BottomTrackControlMenuController::onVisibleChannelComboBoxIndexChanged(int index)
{
    if(!graphicsSceneViewPtr_)
        return;

    if(channelList_.isEmpty())
        return;

    auto channelId = QString(channelList_.at(index)).toInt();

    graphicsSceneViewPtr_->bottomTrack()->setVisibleChannel(channelId);
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

        QObject::connect(graphicsSceneViewPtr_->bottomTrack().get(), &BottomTrack::epochListChanged,
                         this,                                     &BottomTrackControlMenuController::updateChannelList);
    }
}

BottomTrack *BottomTrackControlMenuController::bottomTrack() const
{
    if(!graphicsSceneViewPtr_)
        return nullptr;

    return graphicsSceneViewPtr_->bottomTrack().get();
}

QStringList BottomTrackControlMenuController::channelList() const
{
    return channelList_;
}

int BottomTrackControlMenuController::visibleChannelIndex() const
{
    if(!graphicsSceneViewPtr_)
        return -1;

    auto ch = graphicsSceneViewPtr_->bottomTrack()->visibleChannel();

    return channelList_.indexOf(QString::number(ch.channel));
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

void BottomTrackControlMenuController::updateChannelList()
{
    channelList_.clear();

    if(!graphicsSceneViewPtr_)
        return;

    auto channels = graphicsSceneViewPtr_->bottomTrack()->channels();

    for(const auto& channel : qAsConst(channels))
        channelList_ << QString("%1").arg(channel.channel);

    Q_EMIT channelListUpdated();
}

void BottomTrackControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::bottomTrackControlMenu());
}
