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
    Q_PROPERTY(AbstractEntityDataFilter* inputDataFilter READ inputDataFilter CONSTANT)

public:
    explicit SurfaceControlMenuController(QObject *parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onSurfaceVisibilityCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onSurfaceContourVisibilityCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onContourColorDialogAccepted(QColor color);

    Q_INVOKABLE void onSurfaceGridVisibilityCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onGridColorDialogAccepted(QColor color);

    Q_INVOKABLE void onGridInterpolationCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onUpdateSurfaceButtonClicked(int triangleEdgeLengthLimitSpinBox,
                                                  int gridCellSizeSpinBox,
                                                  int decimationCountSpinBox,
                                                  int decimationDistanceSpinBox);

    Q_INVOKABLE void onFilterTypeComboBoxIndexChanged(int index);

    Q_INVOKABLE void onExportToCSVButtonClicked(const QString& path);


Q_SIGNALS:
    void surfaceProcessorTaskStarted();
    void surfaceProcessorTaskFinished();

protected:
    virtual void findComponent() override;

private:
    Surface* surface() const;
    AbstractEntityDataFilter* inputDataFilter() const;

private:
    std::shared_ptr<AbstractEntityDataFilter> m_inputDataFilter;
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
    SurfaceProcessor m_surfaceProcessor;
    QThread m_thread;

};

#endif // SURFACECONTROLMENUCONTROLLER_H
