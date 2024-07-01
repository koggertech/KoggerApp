#ifndef SCENE3DTOOLBARCONTROLLER_H
#define SCENE3DTOOLBARCONTROLLER_H

#include "qmlcomponentcontroller.h"

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

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);
protected:
    virtual void findComponent() override;
private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};

#endif // SCENE3DTOOLBARCONTROLLER_H
