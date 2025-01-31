#include "surfacecontrolmenucontroller.h"
#include <QmlObjectNames.h>
#include <graphicsscene3dview.h>
#include <surface.h>
#include <bottomtrack.h>
#include <nearestpointfilter.h>
#include <maxpointsfilter.h>

SurfaceControlMenuController::SurfaceControlMenuController(QObject *parent)
    : QmlComponentController(parent)
{
    QObject::connect(&m_surfaceProcessor, &SurfaceProcessor::taskStarted,
                     this               , &SurfaceControlMenuController::surfaceProcessorTaskStarted);

    QObject::connect(&m_surfaceProcessor, &SurfaceProcessor::taskFinished,
                     this,                [this](SurfaceProcessor::Result result) {
                                              if (!m_graphicsSceneView)
                                                  return;

                                              //QVector<QVector3D> data;
                                              //for(const auto& v : qAsConst(result.data))
                                              //    data.append({v.x(), v.z(), v.y()});

                                              QMetaObject::invokeMethod(m_graphicsSceneView->surface().get(),
                                                                        "setData",
                                                                        Qt::QueuedConnection,
                                                                        Q_ARG(QVector<QVector3D>, result.data),
                                                                        Q_ARG(int, result.primitiveType));

                                              m_graphicsSceneView->surface()->setProcessingTask(m_surfaceProcessor.ctask());

                                              if (!result.data.empty()) {
                                                  m_graphicsSceneView->bottomTrack()->surfaceUpdated();
                                              }
                                              Q_EMIT surfaceProcessorTaskFinished();
                                          });
}

void SurfaceControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void SurfaceControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader");
}

Surface *SurfaceControlMenuController::surface() const
{
    if(!m_graphicsSceneView)
        return nullptr;

    return m_graphicsSceneView->surface().get();
}

AbstractEntityDataFilter *SurfaceControlMenuController::inputDataFilter() const
{
    if (!m_graphicsSceneView)
        return nullptr;

    auto task = m_graphicsSceneView->surface()->processingTask(); // ?!

    return nullptr;
}

void SurfaceControlMenuController::onSurfaceVisibilityCheckBoxCheckedChanged(bool checked)
{
    if (!m_graphicsSceneView)
        return;

    m_graphicsSceneView->surface()->setVisible(checked);
}

void SurfaceControlMenuController::onSurfaceContourVisibilityCheckBoxCheckedChanged(bool checked)
{
    if (!m_graphicsSceneView)
        return;

    m_graphicsSceneView->surface()->contour()->setVisible(checked);
}

void SurfaceControlMenuController::onContourColorDialogAccepted(QColor color)
{
    m_graphicsSceneView->surface()->contour()->setColor(color);
}

void SurfaceControlMenuController::onSurfaceGridVisibilityCheckBoxCheckedChanged(bool checked)
{
    if (!m_graphicsSceneView)
        return;

    m_graphicsSceneView->surface()->grid()->setVisible(checked);
}

void SurfaceControlMenuController::onGridColorDialogAccepted(QColor color)
{
    if (!m_graphicsSceneView)
        return;

    m_graphicsSceneView->surface()->grid()->setColor(color);
}

void SurfaceControlMenuController::onGridInterpolationCheckBoxCheckedChanged(bool checked)
{
    Q_UNUSED(checked)
}

void SurfaceControlMenuController::onFilterTypeComboBoxIndexChanged(int index)
{
    Q_UNUSED(index);

    if (!m_graphicsSceneView)
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
    if (!m_graphicsSceneView) {
        qDebug().noquote() << "m_graphicsSceneView is nullptr!";
        return;
    }

    if (m_surfaceProcessor.isBusy()) {
        qDebug().noquote() << "Surface processor is busy!";
        return;
    }

    m_graphicsSceneView->surface()->saveVerticesToFile(path);
}

void SurfaceControlMenuController::onUpdateSurfaceButtonClicked(int triangleEdgeLengthLimitSpinBox,
                                                                int gridCellSizeSpinBox,
                                                                int decimationCountSpinBox,
                                                                int decimationDistanceSpinBox)
{
    if (!m_graphicsSceneView) {
        qDebug().noquote() << "m_graphicsSceneView is nullptr!";
        return;
    }

    if (m_surfaceProcessor.isBusy()) {
        qDebug().noquote() << "Surface processor is busy!";
        return;
    }

    SurfaceProcessorTask task;
    task.setGridInterpEnabled(gridCellSizeSpinBox == -1 ? false : true);
    task.setInterpGridCellSize(gridCellSizeSpinBox);
    task.setBottomTrack(m_graphicsSceneView->bottomTrack());
    task.setEdgeLengthLimit(triangleEdgeLengthLimitSpinBox);
    std::shared_ptr<AbstractEntityDataFilter> filter;
    if (decimationCountSpinBox != -1) {
        filter = std::make_shared<MaxPointsFilter>(decimationCountSpinBox);
    }
    else if (decimationDistanceSpinBox != -1) {
        filter = std::make_shared<NearestPointFilter>(static_cast<float>(decimationDistanceSpinBox));
    }
    task.setBottomTrackDataFilter(filter);

    m_surfaceProcessor.startInThread(task);
}
