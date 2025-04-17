#pragma once

#include <functional>
#include <QThread>

#include "qml_component_controller.h"
#include "isobaths_processor.h"


class Isobaths;
class GraphicsScene3dView;
class IsobathsControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(Isobaths* isobaths READ isobaths CONSTANT)

public:
    explicit IsobathsControlMenuController(QObject* parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onIsobathsVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onUpdateIsobathsButtonClicked();
    Q_INVOKABLE void onSetStepSizeIsobaths(float val);

Q_SIGNALS:
    void isobathsProcessorTaskStarted();
    void isobathsProcessorTaskFinished();

protected:
    virtual void findComponent() override;

private:
    Isobaths* isobaths() const;
    void tryInitPendingLambda();

    GraphicsScene3dView* graphicsSceneViewPtr_ = nullptr;
    IsobathsProcessor isobathsProcessor_;
    QThread thread_;
    std::function<void()> pendingLambda_;
    bool visibility_;
    float stepSize_;
};
