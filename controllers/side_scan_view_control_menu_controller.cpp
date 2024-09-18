#include "side_scan_view_control_menu_controller.h"
#include "graphicsscene3dview.h"


SideScanViewControlMenuController::SideScanViewControlMenuController(QObject *parent) :
    QmlComponentController(parent)
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
    qDebug() << "onUseFilterMosaicViewButtonClicked: " << state;

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->setUseLinearFilterForTileTexture(state);
}

void SideScanViewControlMenuController::onClearSideScanViewButtonClicked()
{
    qDebug() << "onClearSideScanViewButtonClicked";

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->clear();
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

    m_graphicsSceneView->getSideScanViewPtr()->setTileGridVisible(state);
}

SideScanView* SideScanViewControlMenuController::getSideScanViewPtr() const
{
    if (!m_graphicsSceneView) {
        return nullptr;
    }

    return m_graphicsSceneView->getSideScanViewPtr().get();
}
