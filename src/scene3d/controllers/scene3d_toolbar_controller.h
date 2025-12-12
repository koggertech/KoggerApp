#ifndef SCENE3DTOOLBARCONTROLLER_H
#define SCENE3DTOOLBARCONTROLLER_H

#include "qml_component_controller.h"
#include "data_processor.h"


class GraphicsScene3dView;
class Scene3dToolBarController : public QmlComponentController
{
    Q_OBJECT
public:
    explicit Scene3dToolBarController(QObject *parent = nullptr);
    Q_INVOKABLE void onFitAllInViewButtonClicked();
    Q_INVOKABLE void onSetCameraIsometricViewButtonClicked();
    Q_INVOKABLE void onSetCameraMapViewButtonClicked();
    Q_INVOKABLE void onBottomTrackVertexEditingModeButtonChecked(bool checked);
    Q_INVOKABLE void onCancelZoomButtonClicked();
    Q_INVOKABLE void onTrackLastDataCheckButtonCheckedChanged(bool state);
    Q_INVOKABLE void onUpdateBottomTrackCheckButtonCheckedChanged(bool state);
    Q_INVOKABLE void onGridVisibilityCheckedChanged(bool state);
    Q_INVOKABLE void onUseAngleLocationButtonChanged(bool state);
    Q_INVOKABLE void onNavigatorLocationButtonChanged(bool state);
    Q_INVOKABLE void onIsNorthLocationButtonChanged(bool state);
    Q_INVOKABLE void onCompassButtonChanged(bool state);
    Q_INVOKABLE void onCompassPosChanged(int pos);
    Q_INVOKABLE void onCompassSizeChanged(int size);
    Q_INVOKABLE void onPlaneGridTypeChanged(bool val);
    Q_INVOKABLE void onPlaneGridCircleGridSizeChanged(int val);
    Q_INVOKABLE void onPlaneGridCircleGridStepChanged(int val);
    Q_INVOKABLE void onPlaneGridCircleGridAngleChanged(int val);
    Q_INVOKABLE void onPlaneGridCircleGridLabelsChanged(bool state);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);
    void setDataProcessorPtr(DataProcessor* dataProcessorPtr);

protected:
    virtual void findComponent() override;

private:
    void tryInitPendingLambda();

    DataProcessor* dataProcessorPtr_ = nullptr;
    GraphicsScene3dView* graphicsScene3dViewPtr_;
    std::function<void()> pendingLambda_;
    bool isVertexEditingMode_;
    bool trackLastData_;
    bool updateBottomTrack_;
    bool gridVisibility_;
    bool useAngleLocation_;
    bool navigatorViewLocation_;
    bool isNorth_;
    bool compass_;
    int  compassPos_;
    int  compassSize_;
    bool planeGridType_;
    int planeGridCircleSize_;
    int planeGridCircleStep_;
    int planeGridCircleAngle_;
    bool planeGridCircleLabels_;
};

#endif // SCENE3DTOOLBARCONTROLLER_H
