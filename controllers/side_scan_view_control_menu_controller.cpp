#include "side_scan_view_control_menu_controller.h"
#include "graphicsscene3dview.h"


SideScanViewControlMenuController::SideScanViewControlMenuController(QObject *parent) :
    QmlComponentController(parent),
    usingFilters_(false)
{

}

void SideScanViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void SideScanViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("sideScanViewControlMenu");
}

void SideScanViewControlMenuController::onSideScanViewVisibilityCheckBoxCheckedChanged(bool checked)
{
    qDebug() << "onSideScanViewVisibilityCheckBoxCheckedChanged: " << checked;

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->setVisible(checked);
}

void SideScanViewControlMenuController::onUseFilterMosaicViewButtonClicked(bool state)
{
    usingFilters_ = state;
}

void SideScanViewControlMenuController::onUpdateSideScanViewButtonClicked(bool interpMeasLines, const QString& imagePath)
{
    qDebug() << "onUpdateSideScanViewButtonClicked";

    if (!m_graphicsSceneView) {
        return;
    }

   // m_graphicsSceneView->getSideScanViewPtr()->updateData(interpMeasLines, imagePath); // TODO: temporary broken
    m_graphicsSceneView->setTextureImage(m_graphicsSceneView->getSideScanViewPtr()->getImagePtr(), usingFilters_);
}

void SideScanViewControlMenuController::onClearSideScanViewButtonClicked()
{
    qDebug() << "onClearSideScanViewButtonClicked";

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->clear();
}

void SideScanViewControlMenuController::onScaleSideScanViewSpinBoxValueChanged(int scaleFactor)
{
    qDebug() << "onScaleSideScanViewSpinBoxValueChanged";

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->setScaleFactor(scaleFactor);
}

void SideScanViewControlMenuController::onMeasLineVisibleSideScanViewButtonClicked(bool state)
{
    qDebug() << "onMeasLineVisibleSideScanViewButtonClicked";

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->setMeasLineVisible(state);
}

void SideScanViewControlMenuController::onGridVisibleMosaicViewButtonClicked(bool state)
{
    qDebug() << "onGridVisibleMosaicViewButtonClicked";

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->setGridVisible(state);
}

SideScanView* SideScanViewControlMenuController::getSideScanViewPtr() const
{
    if (!m_graphicsSceneView) {
        return nullptr;
    }

    return m_graphicsSceneView->getSideScanViewPtr().get();
}
