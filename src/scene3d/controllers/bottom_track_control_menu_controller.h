#pragma once

#include "qml_component_controller.h"


class BottomTrack;
class GraphicsScene3dView;
class BottomTrackControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(BottomTrack* bottomTrack READ bottomTrack CONSTANT)

public:
    BottomTrackControlMenuController(QObject* parent = nullptr);
    virtual ~BottomTrackControlMenuController();

    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onSurfaceUpdated();
    Q_INVOKABLE void onSurfaceStateChanged(bool state);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    virtual void findComponent() override;

private:
    BottomTrack* bottomTrack() const;
    void tryInitPendingLambda();

Q_SIGNALS:
    void channelListUpdated();

private:
    GraphicsScene3dView* graphicsSceneViewPtr_ = nullptr;
    std::function<void()> pendingLambda_;
    bool visibility_;
};
