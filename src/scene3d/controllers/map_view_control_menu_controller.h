#pragma once

#include "qml_component_controller.h"
#include "map_view.h"


class GraphicsScene3dView;
class MapViewControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(MapView* mapView READ getMapViewPtr CONSTANT)

public:
    explicit MapViewControlMenuController(QObject *parent = nullptr);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onVisibilityChanged(bool state);
    bool visibility() const { return visibility_; }
    void setForcedVisible(bool forced);
    Q_INVOKABLE void onUpdateClicked();

Q_SIGNALS:

protected:
    void findComponent() override;

private:
    void tryInitPendingLambda();

    /*data*/
    MapView* getMapViewPtr() const;
    GraphicsScene3dView* graphicsSceneViewPtr_;
    void applyVisibility();

    std::function<void()> pendingLambda_;
    bool visibility_;
    bool forcedVisible_ = false;
};
