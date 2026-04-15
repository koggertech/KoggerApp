#ifndef NPDFILTERCONTROLMENUCONTROLLER_H
#define NPDFILTERCONTROLMENUCONTROLLER_H


#include "qml_component_controller.h"

class GraphicsScene3dView;
class BottomTrackProvider;
class NpdFilterControlMenuController : public QmlComponentController
{
    Q_OBJECT
public:
    NpdFilterControlMenuController(GraphicsScene3dView *sceneView = nullptr, QObject *parent = nullptr);

    ~NpdFilterControlMenuController() override;
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    void findComponent() override;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};

#endif // NPDFILTERCONTROLMENUCONTROLLER_H
