#ifndef NPDFILTERCONTROLMENUCONTROLLER_H
#define NPDFILTERCONTROLMENUCONTROLLER_H

#include <memory>

#include "qml_component_controller.h"

class GraphicsScene3dView;
class BottomTrackProvider;
class NpdFilterControlMenuController : public QmlComponentController
{
    Q_OBJECT
public:
    NpdFilterControlMenuController(GraphicsScene3dView *sceneView = nullptr, QObject *parent = nullptr);

    virtual ~NpdFilterControlMenuController();
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    virtual void findComponent() override;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};

#endif // NPDFILTERCONTROLMENUCONTROLLER_H
