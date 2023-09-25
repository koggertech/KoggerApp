#ifndef SURFACECONTROLMENUCONTROLLER_H
#define SURFACECONTROLMENUCONTROLLER_H

#include <QColor>
#include <QThread>

#include <qmlcomponentcontroller.h>
#include <surfaceprocessor.h>

class Surface;
class GraphicsScene3dView;
class SurfaceControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(Surface* surface READ surface CONSTANT)

public:
    explicit SurfaceControlMenuController(QObject *parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onSurfaceVisibilityCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onSurfaceContourVisibilityCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onContourColorDialogAccepted(QColor color);

    Q_INVOKABLE void onSurfaceGridVisibilityCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onGridColorDialogAccepted(QColor color);

    Q_INVOKABLE void onGridInterpolationCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onUpdateSurfaceButtonClicked(bool gridInterpEnabled, qreal cellSize);

protected:
    virtual void findComponent() override;

private:
    Surface* surface() const;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
    SurfaceProcessor m_surfaceProcessor;
    QThread m_thread;

};

#endif // SURFACECONTROLMENUCONTROLLER_H
