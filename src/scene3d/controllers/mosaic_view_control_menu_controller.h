#pragma once

#include <QList>
#include "qml_component_controller.h"


class DataProcessor;
class GraphicsScene3dView;
class MosaicViewControlMenuController : public QmlComponentController
{
    Q_OBJECT

public:
    explicit MosaicViewControlMenuController(QObject *parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);
    void setDataProcessorPtr(DataProcessor* dataProcessorPtr);

    Q_INVOKABLE void onVisibilityChanged(bool state);
    Q_INVOKABLE void onUseFilterChanged(bool state);
    Q_INVOKABLE void onGridVisibleChanged(bool state);
    Q_INVOKABLE void onMeasLineVisibleChanged(bool state);
    Q_INVOKABLE void onClearClicked();
    Q_INVOKABLE void onUpdateStateChanged(bool state);
    Q_INVOKABLE void onThemeChanged(int val);
    Q_INVOKABLE void onLevelChanged(float lowLevel, float highLevel);
    Q_INVOKABLE void onUpdateClicked();
    Q_INVOKABLE void onSetLAngleOffset(float val);
    Q_INVOKABLE void onSetRAngleOffset(float val);
    Q_INVOKABLE void onSetResolution(float val);

protected:
    virtual void findComponent() override;

private:
    void tryInitPendingLambda();

    /*data*/
    GraphicsScene3dView* graphicsSceneViewPtr_;
    DataProcessor* dataProcessorPtr_;
    std::function<void()> pendingLambda_;
    bool visibility_;
    bool usingFilter_;
    bool gridVisible_;
    bool measLineVisible_;
    float resolution_;
    bool updateState_;
    int themeId_;
    float lowLevel_;
    float highLevel_;
    float lAngleOffset_;
    float rAngleOffset_;
};
