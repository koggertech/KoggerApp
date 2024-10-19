#pragma once

#include <QList>
#include "qmlcomponentcontroller.h"

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
    Q_INVOKABLE void onTrackLastEpochChanged(bool state);
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
    GraphicsScene3dView* m_graphicsSceneView;
    Core* corePtr_;
    QList<QMetaObject::Connection> connections_;
};
