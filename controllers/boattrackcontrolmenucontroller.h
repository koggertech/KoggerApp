#pragma once

#include <qmlcomponentcontroller.h>


class BoatTrack;
class GraphicsScene3dView;

#ifndef OPAQUE_BoatTrack
#define OPAQUE_BoatTrack
Q_DECLARE_OPAQUE_POINTER(BoatTrack*)
#endif

class BoatTrackControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(BoatTrack* boatTrack READ boatTrack CONSTANT)

public:
    explicit BoatTrackControlMenuController(QObject* parent = nullptr);
    virtual ~BoatTrackControlMenuController();

    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    virtual void findComponent() override final;

private:
    BoatTrack* boatTrack() const;

private Q_SLOTS:

Q_SIGNALS:

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};
