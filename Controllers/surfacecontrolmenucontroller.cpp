#include "surfacecontrolmenucontroller.h"
#include <QmlObjectNames.h>
#include <graphicsscene3dview.h>
#include <surface.h>
#include <bottomtrack.h>

SurfaceControlMenuController::SurfaceControlMenuController(QObject *parent)
    : QmlComponentController(parent)
{
    QObject::connect(&m_surfaceProcessor, &SurfaceProcessor::taskFinished, [this](SurfaceProcessor::Result result){
        if(!m_graphicsSceneView)
            return;

        QMetaObject::invokeMethod(m_graphicsSceneView->surface().get(),
                                  "setData",
                                  Q_ARG(QVector<QVector3D>, result.data),
                                  Q_ARG(int, result.primitiveType));
    });
}

void SurfaceControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void SurfaceControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>(QmlObjectNames::surfaceControlMenu);
}

Surface *SurfaceControlMenuController::surface() const
{
    return m_graphicsSceneView->surface().get();
}

void SurfaceControlMenuController::onSurfaceVisibilityCheckBoxCheckedChanged(bool checked)
{
    if(!m_graphicsSceneView)
        return;

    m_graphicsSceneView->surface()->setVisible(checked);
}

void SurfaceControlMenuController::onSurfaceContourVisibilityCheckBoxCheckedChanged(bool checked)
{
    m_graphicsSceneView->surface()->contour()->setVisible(checked);
}

void SurfaceControlMenuController::onContourColorDialogAccepted(QColor color)
{
    m_graphicsSceneView->surface()->contour()->setColor(color);
}

void SurfaceControlMenuController::onSurfaceGridVisibilityCheckBoxCheckedChanged(bool checked)
{
    m_graphicsSceneView->surface()->grid()->setVisible(checked);
}

void SurfaceControlMenuController::onGridColorDialogAccepted(QColor color)
{
    m_graphicsSceneView->surface()->grid()->setColor(color);
}

void SurfaceControlMenuController::onGridInterpolationCheckBoxCheckedChanged(bool checked)
{
    Q_UNUSED(checked)
}

void SurfaceControlMenuController::onUpdateSurfaceButtonClicked(bool gridInterpEnabled, qreal cellSize)
{
    if(m_surfaceProcessor.isBusy()){
        qDebug().noquote() << "Surface processor is busy!";
        return;
    }

    if(!m_graphicsSceneView)
        return;

    auto bottomTrack = m_graphicsSceneView->bottomTrack();
    auto surface     = m_graphicsSceneView->surface();

    SurfaceProcessor::Task task;

    task.needSmoothing = gridInterpEnabled;
    task.cellSize      = cellSize;
    task.source        = bottomTrack->cdata();
    task.bounds        = surface->bounds();

    m_surfaceProcessor.startInThread(task);
}
