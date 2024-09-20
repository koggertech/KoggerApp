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

    m_graphicsSceneView->getSideScanViewPtr()->setUseLinearFilter(state);
}

void SideScanViewControlMenuController::onClearSideScanViewButtonClicked()
{
    qDebug() << "onClearSideScanViewButtonClicked";

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->clear();
}

void SideScanViewControlMenuController::onReinitGlobalMeshSideScanViewButtonClicked(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
{
    qDebug() << "onReinitGlobalMeshSideScanViewButtonClicked:" << tileSidePixelSize << tileHeightMatrixRatio << tileResolution;

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->resetTileSettings(tileSidePixelSize, tileHeightMatrixRatio, tileResolution);
}

void SideScanViewControlMenuController::onGenerateGridContourSideScanViewButtonClicked(bool state)
{
    qDebug() << "onGenerateGridContourSideScanViewButtonClicked:" << state;

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->setGenerateGridContour(state);
}

void SideScanViewControlMenuController::onUpdateSideScanViewButtonClicked(bool state)
{
    qDebug() << "onUpdateSideScanViewButtonClicked:" << state;

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->setCalcStateSideScanView(state);
}

void SideScanViewControlMenuController::onTrackLastEpochSideScanViewButtonClicked(bool state)
{
    qDebug() << "onTrackLastEpochSideScanViewButtonClicked:" << state;

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->setTrackLastEpoch(state);
}

void SideScanViewControlMenuController::onThemeSideScanViewButtonClicked(int val)
{
    qDebug() << "onThemeSideScanViewButtonClicked:" << val;

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->setColorTableThemeById(val + 1);
}

void SideScanViewControlMenuController::onSetLevelSideScanViewClicked(float lowLevel, float highLevel)
{
    qDebug() << "onSetLevelSideScanViewClicked:" << lowLevel << highLevel;

    if (!m_graphicsSceneView) {
        return;
    }

    m_graphicsSceneView->getSideScanViewPtr()->setColorTableLevels(lowLevel, highLevel);
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
