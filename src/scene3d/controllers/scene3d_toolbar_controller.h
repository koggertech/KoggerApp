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
};

#endif // SCENE3DTOOLBARCONTROLLER_H
