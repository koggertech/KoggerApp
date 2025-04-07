#ifndef SCENE3DCONTROLMENUCONTROLLER_H
#define SCENE3DCONTROLMENUCONTROLLER_H

#include <qml_component_controller.h>

class GraphicsScene3dView;
class Scene3DControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(float verticalScale READ verticalScale CONSTANT)
    Q_PROPERTY(bool sceneBoundingBoxVisible READ sceneBoundingBoxVisible CONSTANT)

public:
    explicit Scene3DControlMenuController(QObject *parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onVerticalScaleSliderValueChanged(float value);
    Q_INVOKABLE void onShowSceneBoundingBoxCheckBoxChecked(bool checked);

protected:
    virtual void findComponent() override;

private:
    float verticalScale() const;
    float sceneBoundingBoxVisible() const;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};

#endif // SCENE3DCONTROLMENUCONTROLLER_H
