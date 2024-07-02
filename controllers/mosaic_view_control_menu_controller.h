#pragma once

#include <QColor>
#include <QThread>
#include "qmlcomponentcontroller.h"
#include "mosaic_view.h"


class MosaicView;
class GraphicsScene3dView;
class MosaicViewControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(MosaicView* mosaicView READ getMosaicViewPtr CONSTANT)

public:
    explicit MosaicViewControlMenuController(QObject *parent = nullptr);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);
    Q_INVOKABLE void onMosaicViewVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onUpdateMosaicViewButtonClicked();

Q_SIGNALS:

protected:
    virtual void findComponent() override;

private:
    MosaicView* getMosaicViewPtr() const;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};
