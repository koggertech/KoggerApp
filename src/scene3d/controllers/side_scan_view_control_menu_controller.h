#pragma once

#include <QList>
#include "qml_component_controller.h"

class Core;
class SideScanView;
class GraphicsScene3dView;
class SideScanViewControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(SideScanView* sideScanView READ getSideScanViewPtr CONSTANT)

public:
    explicit SideScanViewControlMenuController(QObject *parent = nullptr);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);
    void setCorePtr(Core* corePtr);

    Q_INVOKABLE void onVisibilityChanged(bool state);
    Q_INVOKABLE void onUseFilterChanged(bool state);
    Q_INVOKABLE void onGridVisibleChanged(bool state);
    Q_INVOKABLE void onMeasLineVisibleChanged(bool state);
    Q_INVOKABLE void onClearClicked();
    Q_INVOKABLE void onGlobalMeshChanged(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution);
    Q_INVOKABLE void onGenerateGridContourChanged(bool state);
    Q_INVOKABLE void onUpdateStateChanged(bool state);
    Q_INVOKABLE void onThemeChanged(int val);
    Q_INVOKABLE void onLevelChanged(float lowLevel, float highLevel);
    Q_INVOKABLE void onUpdateClicked();
    Q_INVOKABLE void onSetLAngleOffset(float val);
    Q_INVOKABLE void onSetRAngleOffset(float val);


Q_SIGNALS:

protected:
    virtual void findComponent() override;

private:
    void tryClearMakeConnections();
    SideScanView* getSideScanViewPtr() const;
    void tryInitPendingLambda();

    /*data*/
    GraphicsScene3dView* graphicsSceneViewPtr_;
    Core* corePtr_;
    QList<QMetaObject::Connection> connections_;
    std::function<void()> pendingLambda_;
    bool visibility_;
    bool usingFilter_;
    bool gridVisible_;
    bool measLineVisible_;
    //int tileSidePixelSize_;
    //int tileHeightMatrixRatio_;
    //float tileResolution_;
    bool generateGridContour_;
    bool updateState_;
    int themeId_;
    float lowLevel_;
    float highLevel_;
    float lAngleOffset_;
    float rAngleOffset_;
};
