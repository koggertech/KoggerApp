#include "surfacecontrolmenucontroller.h"
#include <QmlObjectNames.h>
#include <graphicsscene3dview.h>
#include <surface.h>
#include <bottomtrack.h>

SurfaceControlMenuController::SurfaceControlMenuController(QObject *parent)
    : QmlComponentController(parent)
{
    QObject::connect(&m_surfaceProcessor, &SurfaceProcessor::taskFinished, [this](SurfaceProcessor::Result result){
        if(!m_graphicsSceneView || !m_graphicsSceneView->scene())
            return;

        QMetaObject::invokeMethod(m_graphicsSceneView->scene()->surface().get(),
                                  "setPrimitiveType",
                                  Q_ARG(int, result.primitiveType));

        QMetaObject::invokeMethod(m_graphicsSceneView->scene()->surface().get(),
                                  "setData",
                                  Q_ARG(QVector<QVector3D>, result.data));
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

void SurfaceControlMenuController::onSurfaceVisibilityCheckBoxCheckedChanged(bool checked)
{
    if(!m_graphicsSceneView || !m_graphicsSceneView->scene())
        return;

    auto surface = m_graphicsSceneView->scene()->surface();

    if(!surface)
        return;

    QMetaObject::invokeMethod(surface.get(), "setVisible", Q_ARG(bool, checked));
}

void SurfaceControlMenuController::onSurfaceContourVisibilityCheckBoxCheckedChanged(bool checked)
{
    auto surf = surface();

    if(!surf)
        return;

    QMetaObject::invokeMethod(surf->contour(), "setVisible", Q_ARG(bool, checked));
}

void SurfaceControlMenuController::onContourColorDialogAccepted(QColor color)
{
    auto surf = surface();

    if(!surf)
        return;

    QMetaObject::invokeMethod(surf->contour(), "setColor", Q_ARG(QColor, color));
}

void SurfaceControlMenuController::onSurfaceGridVisibilityCheckBoxCheckedChanged(bool checked)
{
    auto surf = surface();

    if(!surf)
        return;

    QMetaObject::invokeMethod(surf->grid(), "setVisible", Q_ARG(bool, checked));
}

void SurfaceControlMenuController::onGridColorDialogAccepted(QColor color)
{
    auto surf = surface();

    if(!surf)
        return;

    QMetaObject::invokeMethod(surf->grid(), "setColor", Q_ARG(QColor, color));
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

    if(!m_graphicsSceneView || !m_graphicsSceneView->scene())
        return;

    auto bottomTrack = m_graphicsSceneView->scene()->bottomTrack();
    auto surface     = m_graphicsSceneView->scene()->surface();

    SurfaceProcessor::Task task;

    task.needSmoothing = gridInterpEnabled;
    task.cellSize      = cellSize;
    task.source        = bottomTrack->cdata();
    task.bounds        = surface->boundingBox();

    m_surfaceProcessor.startInThread(task);
}

Surface *SurfaceControlMenuController::surface() const
{
    if(!m_graphicsSceneView || !m_graphicsSceneView->scene())
        return nullptr;

    return m_graphicsSceneView->scene()->surface().get();
}
