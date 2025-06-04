#pragma once

#include <functional>
#include <QThread>

#include "qml_component_controller.h"
#include "surface_view_processor.h"


class GraphicsScene3dView;
class SurfaceViewControlMenuController : public QmlComponentController
{
    Q_OBJECT

public:
    explicit SurfaceViewControlMenuController(QObject* parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onSurfaceViewVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onUpdateSurfaceViewButtonClicked();

Q_SIGNALS:
    void surfaceViewProcessorTaskStarted();
    void surfaceViewProcessorTaskFinished();

protected:
    virtual void findComponent() override;

private:
    void tryInitPendingLambda();

    GraphicsScene3dView* graphicsSceneViewPtr_;
    SurfaceViewProcessor surfaceViewProcessor_;
    QThread thread_;
    std::function<void()> pendingLambda_;
    bool visibility_;
};
