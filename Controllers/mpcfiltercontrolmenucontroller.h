#ifndef MPCFILTERCONTROLMENUCONTROLLER_H
#define MPCFILTERCONTROLMENUCONTROLLER_H

#include <memory>

#include "qmlcomponentcontroller.h"

class GraphicsScene3dView;
class BottomTrackProvider;
class MpcFilterControlMenuController : public QmlComponentController
{
    Q_OBJECT
public:
    explicit MpcFilterControlMenuController(GraphicsScene3dView *sceneView = nullptr, QObject *parent = nullptr);

    Q_INVOKABLE void onPointsCountSpinBoxValueChanged(qreal value);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    void setBottomTrackProvider(std::shared_ptr <BottomTrackProvider> provider);

protected:
    virtual void findComponent() override;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
    std::shared_ptr <BottomTrackProvider> m_bottomTrackProvider;
};

#endif // MPCFILTERCONTROLMENUCONTROLLER_H
