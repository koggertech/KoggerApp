#pragma once

#include <functional>
#include <QThread>

#include "qml_component_controller.h"


class GraphicsScene3dView;
class SurfaceViewControlMenuController : public QmlComponentController
{
    Q_OBJECT

public:
    explicit SurfaceViewControlMenuController(QObject* parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onSurfaceViewVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onUpdateSurfaceViewButtonClicked();
    Q_INVOKABLE void onTrianglesVisible(bool state);
    Q_INVOKABLE void onEdgesVisible(bool state);
    Q_INVOKABLE void onSetSurfaceLineStepSize(float val);
    Q_INVOKABLE void onSetLabelStepSize(int val);
    Q_INVOKABLE void onThemeChanged(int val);
    Q_INVOKABLE void onDebugModeView(bool state);
    Q_INVOKABLE void onProcessStateChanged(bool state);
    Q_INVOKABLE void onResetSurfaceViewButtonClicked();
    Q_INVOKABLE void onEdgeLimitChanged(int val);
    Q_INVOKABLE void onHandleXCallChanged(int val);

protected:
    virtual void findComponent() override;

private:
    void tryInitPendingLambda();

    GraphicsScene3dView* graphicsSceneViewPtr_;
    QThread thread_;
    std::function<void()> pendingLambda_;
    bool visibility_;
    bool edgesVisible_ = true;
    bool trianglesVisible_ = true;
    float surfaceLineStepSize_ = 3.0f;
    int themeId_ = 0;
    int labelStepSize_ = 100;
    bool debugModeView_ = false;
    bool processState_ = true;
    int edgeLimit_ = 20;
    int handleXCall_ = 1;
};
