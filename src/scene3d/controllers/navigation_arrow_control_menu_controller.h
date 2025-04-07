#pragma once

#include "qml_component_controller.h"


class NavigationArrow;
class GraphicsScene3dView;
class NavigationArrowControlMenuController : public QmlComponentController
{
    Q_OBJECT
public:
    explicit NavigationArrowControlMenuController(QObject* parent = nullptr);
    virtual ~NavigationArrowControlMenuController();

    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    virtual void findComponent() override final;

private:
    /*methods*/
    void tryInitPendingLambda();

    /*data*/
    GraphicsScene3dView* graphicsSceneView_;
    std::function<void()> pendingLambda_;
    bool isVisible_;
};
