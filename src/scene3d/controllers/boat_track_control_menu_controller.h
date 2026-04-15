#pragma once

#include "qml_component_controller.h"

#include "boat_track.h"


class GraphicsScene3dView;
class BoatTrackControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(BoatTrack* boatTrack READ boatTrack CONSTANT)

public:
    explicit BoatTrackControlMenuController(QObject* parent = nullptr);
    ~BoatTrackControlMenuController() override;

    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    void findComponent() final;

private:
    BoatTrack* boatTrack() const;
    void tryInitPendingLambda();

private Q_SLOTS:

Q_SIGNALS:

private:
    GraphicsScene3dView* graphicsSceneViewPtr_;
    std::function<void()> pendingLambda_;
    bool visibility_;
};
