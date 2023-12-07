#ifndef NPDFILTERCONTROLMENUCONTROLLER_H
#define NPDFILTERCONTROLMENUCONTROLLER_H

#include <memory>

#include "qmlcomponentcontroller.h"

class GraphicsScene3dView;
class BottomTrackProvider;
class NpdFilterControlMenuController : public QmlComponentController
{
    Q_OBJECT
public:
    NpdFilterControlMenuController(GraphicsScene3dView *sceneView = nullptr, QObject *parent = nullptr);

    virtual ~NpdFilterControlMenuController();

    Q_INVOKABLE void onDistanceSpinBoxValueChanged(qreal value);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    void setBottomTrackProvider(std::shared_ptr <BottomTrackProvider> provider);

protected:
    virtual void findComponent() override;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
    std::shared_ptr <BottomTrackProvider> m_bottomTrackProvider;
};

#endif // NPDFILTERCONTROLMENUCONTROLLER_H
