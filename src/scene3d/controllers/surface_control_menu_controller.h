#ifndef SURFACECONTROLMENUCONTROLLER_H
#define SURFACECONTROLMENUCONTROLLER_H

#include <QColor>
#include <QThread>

#include "qml_component_controller.h"
#include "surface_processor.h"

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
    void tryInitPendingLambda();

private:
    std::shared_ptr<AbstractEntityDataFilter> inputDataFilter_;
    GraphicsScene3dView* graphicsSceneViewPtr_;
    SurfaceProcessor surfaceProcessor_;
    QThread thread_;
    std::function<void()> pendingLambda_;
    bool visibility_;
    bool gridVisibility_;
    bool contourVisibility_;
};

#endif // SURFACECONTROLMENUCONTROLLER_H
