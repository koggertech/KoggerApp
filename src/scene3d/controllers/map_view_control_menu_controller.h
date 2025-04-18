#pragma once

#include "qml_component_controller.h"


class MapView;
class GraphicsScene3dView;
class MapViewControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(MapView* mapView READ getMapViewPtr CONSTANT)

public:
    explicit MapViewControlMenuController(QObject *parent = nullptr);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onVisibilityChanged(bool state);
    Q_INVOKABLE void onUpdateClicked();

Q_SIGNALS:

protected:
    virtual void findComponent() override;

private:
    void tryInitPendingLambda();

    /*data*/
    MapView* getMapViewPtr() const;
    GraphicsScene3dView* graphicsSceneViewPtr_;
    std::function<void()> pendingLambda_;
    bool visibility_;
};
