#include "surface_control_menu_controller.h"
#include "qml_object_names.h"
#include "scene3d_view.h"
#include "surface.h"
#include "bottom_track.h"
#include "nearest_point_filter.h"
#include "max_points_filter.h"


SurfaceControlMenuController::SurfaceControlMenuController(QObject *parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      pendingLambda_(nullptr),
      visibility_(false),
      gridVisibility_(false),
      contourVisibility_(false)
{
    QObject::connect(&surfaceProcessor_, &SurfaceProcessor::taskStarted,
                     this,               &SurfaceControlMenuController::surfaceProcessorTaskStarted);

    QObject::connect(&surfaceProcessor_, &SurfaceProcessor::taskFinished,
                     this,                [this](SurfaceProcessor::Result result) {
                                              if (!graphicsSceneViewPtr_)
                                                  return;

                                              //QVector<QVector3D> data;
                                              //for(const auto& v : qAsConst(result.data))
                                              //    data.append({v.x(), v.z(), v.y()});

                                              QMetaObject::invokeMethod(graphicsSceneViewPtr_->surface().get(),
                                                                        "setData",
                                                                        Qt::QueuedConnection,
                                                                        Q_ARG(QVector<QVector3D>, result.data),
                                                                        Q_ARG(int, result.primitiveType));

                                              graphicsSceneViewPtr_->surface()->setProcessingTask(surfaceProcessor_.ctask());

                                              if (!result.data.empty()) {
                                                  graphicsSceneViewPtr_->bottomTrack()->surfaceUpdated();
                                              }

                                              Q_EMIT surfaceProcessorTaskFinished();
                                          });
}

void SurfaceControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void SurfaceControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader");
}

Surface *SurfaceControlMenuController::surface() const
{
    if(!graphicsSceneViewPtr_)
        return nullptr;

    return graphicsSceneViewPtr_->surface().get();
}

AbstractEntityDataFilter *SurfaceControlMenuController::inputDataFilter() const
{
    if (!graphicsSceneViewPtr_)
        return nullptr;

    auto task = graphicsSceneViewPtr_->surface()->processingTask(); // ?!

    return nullptr;
}

void SurfaceControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (graphicsSceneViewPtr_) {
                if (auto surfacePtr = graphicsSceneViewPtr_->surface(); surfacePtr) {
                    surfacePtr->setVisible(visibility_);
                    surfacePtr->grid()->setVisible(gridVisibility_);
                    surfacePtr->contour()->setVisible(contourVisibility_);
                }
            }
        };
    }
}

void SurfaceControlMenuController::onSurfaceVisibilityCheckBoxCheckedChanged(bool checked)
{
    visibility_ = checked;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->surface()->setVisible(checked);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceControlMenuController::onSurfaceContourVisibilityCheckBoxCheckedChanged(bool checked)
{
    contourVisibility_ = checked;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->surface()->contour()->setVisible(contourVisibility_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceControlMenuController::onContourColorDialogAccepted(QColor color)
{
    graphicsSceneViewPtr_->surface()->contour()->setColor(color);
}

void SurfaceControlMenuController::onSurfaceGridVisibilityCheckBoxCheckedChanged(bool checked)
{
    gridVisibility_ = checked;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->surface()->grid()->setVisible(gridVisibility_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceControlMenuController::onGridColorDialogAccepted(QColor color)
{
    if (!graphicsSceneViewPtr_)
        return;

    graphicsSceneViewPtr_->surface()->grid()->setColor(color);
}

void SurfaceControlMenuController::onGridInterpolationCheckBoxCheckedChanged(bool checked)
{
    Q_UNUSED(checked)
}

void SurfaceControlMenuController::onFilterTypeComboBoxIndexChanged(int index)
{
    Q_UNUSED(index);

    if (!graphicsSceneViewPtr_)
        return;

    auto menu = m_component->findChild<QObject*>(QmlObjectNames::surfaceControlMenu());

    if(!menu)
        return;

    auto filterMenuLoader = menu->findChild<QObject*>("filterParamsLoader");

    if(!filterMenuLoader)
        return;
}

void SurfaceControlMenuController::onExportToCSVButtonClicked(const QString& path)
{
    if (!graphicsSceneViewPtr_) {
        qDebug().noquote() << "m_graphicsSceneView is nullptr!";
        return;
    }

    if (surfaceProcessor_.isBusy()) {
        qDebug().noquote() << "Surface processor is busy!";
        return;
    }

    graphicsSceneViewPtr_->surface()->saveVerticesToFile(path);
}

void SurfaceControlMenuController::onUpdateSurfaceButtonClicked(int triangleEdgeLengthLimitSpinBox,
                                                                int gridCellSizeSpinBox,
                                                                int decimationCountSpinBox,
                                                                int decimationDistanceSpinBox)
{
    if (!graphicsSceneViewPtr_) {
        qDebug().noquote() << "m_graphicsSceneView is nullptr!";
        return;
    }

    if (surfaceProcessor_.isBusy()) {
        qDebug().noquote() << "Surface processor is busy!";
        return;
    }

    SurfaceProcessorTask task;
    task.setGridInterpEnabled(gridCellSizeSpinBox == -1 ? false : true);
    task.setInterpGridCellSize(gridCellSizeSpinBox);
    task.setBottomTrack(graphicsSceneViewPtr_->bottomTrack());
    task.setEdgeLengthLimit(triangleEdgeLengthLimitSpinBox);
    std::shared_ptr<AbstractEntityDataFilter> filter;
    if (decimationCountSpinBox != -1) {
        filter = std::make_shared<MaxPointsFilter>(decimationCountSpinBox);
    }
    else if (decimationDistanceSpinBox != -1) {
        filter = std::make_shared<NearestPointFilter>(static_cast<float>(decimationDistanceSpinBox));
    }
    task.setBottomTrackDataFilter(filter);

    surfaceProcessor_.startInThread(task);
}
