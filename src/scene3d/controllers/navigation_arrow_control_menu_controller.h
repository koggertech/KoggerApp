#pragma once

#include "qml_component_controller.h"


class NavigationArrow;
class GraphicsScene3dView;
class NavigationArrowControlMenuController : public QmlComponentController
{
    Q_OBJECT
public:
    explicit NavigationArrowControlMenuController(QObject* parent = nullptr);
    ~NavigationArrowControlMenuController() override;

    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onSizeSpinBoxValueChanged(int size);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    void findComponent() final;

private:
    /*methods*/
    void tryInitPendingLambda();

    /*data*/
    GraphicsScene3dView* graphicsSceneView_;
    std::function<void()> pendingLambda_;
    bool isVisible_;
    int size_;
};
