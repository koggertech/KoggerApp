#include "side_scan_view_control_menu_controller.h"
#include "graphicsscene3dview.h"


SideScanViewControlMenuController::SideScanViewControlMenuController(QObject *parent) :
    QmlComponentController(parent),
    m_graphicsSceneView(nullptr)
{

}

void SideScanViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;
}

void SideScanViewControlMenuController::onVisibilityChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setVisible(state);
    }
}

void SideScanViewControlMenuController::onUseFilterChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setUseLinearFilter(state);
    }
}

void SideScanViewControlMenuController::onGridVisibleChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setTileGridVisible(state);
    }
}

void SideScanViewControlMenuController::onMeasLineVisibleChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setMeasLineVisible(state);
    }
}

void SideScanViewControlMenuController::onClearClicked()
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->clear();
    }
}

void SideScanViewControlMenuController::onGlobalMeshChanged(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->resetTileSettings(tileSidePixelSize, tileHeightMatrixRatio, tileResolution);
    }
}

void SideScanViewControlMenuController::onGenerateGridContourChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setGenerateGridContour(state);
    }
}

void SideScanViewControlMenuController::onUpdateStateChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->setCalcStateSideScanView(state);
    }
}

void SideScanViewControlMenuController::onTrackLastEpochChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setTrackLastEpoch(state);
    }
}

void SideScanViewControlMenuController::onThemeChanged(int val)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setColorTableThemeById(val + 1);
    }
}

void SideScanViewControlMenuController::onLevelChanged(float lowLevel, float highLevel)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setColorTableLevels(lowLevel, highLevel);
    }
}

SideScanView* SideScanViewControlMenuController::getSideScanViewPtr() const
{
    if (m_graphicsSceneView) {
        return m_graphicsSceneView->getSideScanViewPtr().get();
    }
    return nullptr;
}

void SideScanViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("sideScanViewControlMenu");
}
