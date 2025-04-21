#pragma once

#include <functional>
#include <QThread>

#include "qml_component_controller.h"
#include "isobaths_processor.h"


class GraphicsScene3dView;
class IsobathsControlMenuController : public QmlComponentController
{
    Q_OBJECT

public:
    explicit IsobathsControlMenuController(QObject* parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onIsobathsVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onUpdateIsobathsButtonClicked();
    Q_INVOKABLE void onSetSurfaceStepSizeIsobaths(float val);
    Q_INVOKABLE void onSetLineStepSizeIsobaths(float val);

Q_SIGNALS:
    void isobathsProcessorTaskStarted();
    void isobathsProcessorTaskFinished();

protected:
    virtual void findComponent() override;

private:
    void tryInitPendingLambda();

    GraphicsScene3dView* graphicsSceneViewPtr_;
    IsobathsProcessor isobathsProcessor_;
    QThread thread_;
    std::function<void()> pendingLambda_;
    bool visibility_;
    float surfaceStepSize_;
    float lineStepSize_;
};
