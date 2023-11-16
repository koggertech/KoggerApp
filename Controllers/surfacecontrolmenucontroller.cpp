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

    QObject::connect(&m_surfaceProcessor, &SurfaceProcessor::taskFinished, [this](SurfaceProcessor::Result result){
        if(!m_graphicsSceneView)
            return;

        //QVector<QVector3D> data;
        //for(const auto& v : qAsConst(result.data))
        //    data.append({v.x(), v.z(), v.y()});


        QMetaObject::invokeMethod(m_graphicsSceneView->surface().get(),
                                  "setData",
                                  Q_ARG(QVector<QVector3D>, result.data),
                                  Q_ARG(int, result.primitiveType));
        m_graphicsSceneView->surface()->setProcessingTask(m_surfaceProcessor.ctask());

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
    auto task = m_graphicsSceneView->surface()->processingTask();

    return nullptr;
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

void SurfaceControlMenuController::onFilterTypeComboBoxIndexChanged(int index)
{
    if(!m_graphicsSceneView)
        return;

    auto menu = m_component->findChild<QObject*>(QmlObjectNames::surfaceControlMenu);

    if(!menu)
        return;

    auto filterMenuLoader = menu->findChild<QObject*>("filterParamsLoader");

    if(!filterMenuLoader)
        return;


}

void SurfaceControlMenuController::onUpdateSurfaceButtonClicked()
{
    auto menu = m_component->findChild<QObject*>(QmlObjectNames::surfaceControlMenu);

    if(!menu)
        return;

    auto triangleEdgeLengthSpinBox = menu->findChild<QObject*>("triangleEdgeLengthLimitSpinBox");
    auto gridCellSizeSpinBox = menu->findChild<QObject*>("gridCellSizeSpinBox");
    auto gridInterpSpinBox = menu->findChild<QObject*>("gridInterpolationCheckBox");

    if(m_surfaceProcessor.isBusy()){
        qDebug().noquote() << "Surface processor is busy!";
        return;
    }

    if(!m_graphicsSceneView)
        return;

    SurfaceProcessorTask task;

    task.setGridInterpEnabled(gridInterpSpinBox->property("checked").toBool());
    task.setInterpGridCellSize(gridCellSizeSpinBox->property("value").toInt());
    task.setBottomTrack(m_graphicsSceneView->bottomTrack());
    task.setEdgeLengthLimit(triangleEdgeLengthSpinBox->property("value").toInt());

    std::shared_ptr<AbstractEntityDataFilter> filter;

    auto filterTypeComboBox = menu->findChild<QObject*>("filterTypeCombo");
    auto filterParamsMenuLoader = menu->findChild<QObject*>("filterParamsLoader");

    if(filterTypeComboBox->property("currentIndex") == AbstractEntityDataFilter::FilterType::MaxPointsCount){
        auto mpcFilterMenu = filterParamsMenuLoader->findChild<QObject*>("mpcFilterControlMenu");
        auto pointsCountSpinBox = mpcFilterMenu->findChild<QObject*>("pointsCountSpinBox");
        int v = pointsCountSpinBox->property("value").toInt();
        filter = std::make_shared<MaxPointsFilter>(pointsCountSpinBox->property("value").toInt());
    }

    if(filterTypeComboBox->property("currentIndex") == AbstractEntityDataFilter::FilterType::NearestPointDistance){
        auto npdFilterMenu = filterParamsMenuLoader->findChild<QObject*>("npdFilterControlMenu");
        auto distanceSpinBox = npdFilterMenu->findChild<QObject*>("distanceValueSpinBox");
        filter = std::make_shared<NearestPointFilter>(distanceSpinBox->property("value").toFloat());
    }

    task.setBottomTrackDataFilter(filter);

    m_surfaceProcessor.startInThread(task);
}
