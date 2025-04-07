#ifndef MPCFILTERCONTROLMENUCONTROLLER_H
#define MPCFILTERCONTROLMENUCONTROLLER_H

#include <memory>

#include "qml_component_controller.h"

class GraphicsScene3dView;
class BottomTrackProvider;
class MpcFilterControlMenuController : public QmlComponentController
{
    Q_OBJECT
public:
    explicit MpcFilterControlMenuController(GraphicsScene3dView *sceneView = nullptr, QObject *parent = nullptr);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    virtual void findComponent() override;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};

#endif // MPCFILTERCONTROLMENUCONTROLLER_H
