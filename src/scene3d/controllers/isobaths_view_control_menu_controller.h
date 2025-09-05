#pragma once

#include <functional>
#include <QThread>

#include "qml_component_controller.h"
#include "data_processor.h"


class GraphicsScene3dView;
class IsobathsViewControlMenuController : public QmlComponentController
{
    Q_OBJECT

public:
    explicit IsobathsViewControlMenuController(QObject* parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);
    void setDataProcessorPtr(DataProcessor *dataProcessorPtr);

    Q_INVOKABLE void onIsobathsVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onUpdateIsobathsButtonClicked();
    Q_INVOKABLE void onTrianglesVisible(bool state);
    Q_INVOKABLE void onEdgesVisible(bool state);
    Q_INVOKABLE void onSetSurfaceLineStepSize(float val);
    Q_INVOKABLE void onSetLabelStepSize(int val);
    Q_INVOKABLE void onThemeChanged(int val);
    Q_INVOKABLE void onDebugModeView(bool state);
    Q_INVOKABLE void onProcessStateChanged(bool state);
    Q_INVOKABLE void onResetIsobathsButtonClicked();
    Q_INVOKABLE void onEdgeLimitChanged(int val);
    Q_INVOKABLE void onSetExtraWidth(int val);

protected:
    virtual void findComponent() override;

private:
    void tryInitPendingLambda();

private:
    GraphicsScene3dView* graphicsSceneViewPtr_;
    DataProcessor* dataProcessorPtr_;
    std::function<void()> pendingLambda_;
    QThread thread_;
    float surfaceLineStepSize_;
    int themeId_;
    int labelStepSize_;
    int edgeLimit_;
    int extraWidth_;
    bool visibility_;
    bool edgesVisible_;
    bool trianglesVisible_;
    bool debugModeView_;
    bool processState_;
};
