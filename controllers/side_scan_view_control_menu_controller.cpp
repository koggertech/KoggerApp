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

void SideScanViewControlMenuController::onUpdateSideScanViewButtonClicked(const QString& imagePath, const QString& heightMatrixPath)
{
    qDebug() << "onUpdateSideScanViewButtonClicked";

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->updateData(imagePath, heightMatrixPath);
}

void SideScanViewControlMenuController::onClearSideScanViewButtonClicked()
{
    qDebug() << "onClearSideScanViewButtonClicked";

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->clear();
}

SideScanView* SideScanViewControlMenuController::getSideScanViewPtr() const
{
    if (!m_graphicsSceneView) {
        return nullptr;
    }

    return m_graphicsSceneView->getSideScanViewPtr().get();
}
