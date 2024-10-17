#include "side_scan_view_control_menu_controller.h"
#include "graphicsscene3dview.h"
#include "core.h"


SideScanViewControlMenuController::SideScanViewControlMenuController(QObject *parent) :
    QmlComponentController(parent),
    m_graphicsSceneView(nullptr),
    corePtr_(nullptr)
{

}

void SideScanViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;

    tryClearMakeConnections();
}

void SideScanViewControlMenuController::setCorePtr(Core* corePtr)
{
    corePtr_ = corePtr;

    tryClearMakeConnections();
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

void SideScanViewControlMenuController::onUpdateClicked()
{
    if (m_graphicsSceneView) {
#ifdef SEPARATE_READING
        if (corePtr_) {
            if (!corePtr_->getTryOpenedfilePath().isEmpty()) {
                return;
            }
        }
        else {
            return;
        }
#endif
        m_graphicsSceneView->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kPerformance);
        m_graphicsSceneView->interpolateDatasetEpochs();
        m_graphicsSceneView->getSideScanViewPtr()->startUpdateDataInThread(0, 0);
    }
}

void SideScanViewControlMenuController::onSetLAngleOffset(float val)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setLAngleOffset(val);
    }
}

void SideScanViewControlMenuController::onSetRAngleOffset(float val)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setRAngleOffset(val);
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

void SideScanViewControlMenuController::tryClearMakeConnections()
{
    for (auto& itm : connections_) {
        disconnect(itm);
    }
    connections_.clear();

    if (m_graphicsSceneView && corePtr_) {
        connections_.append(QObject::connect(m_graphicsSceneView->getSideScanViewPtr().get(), &SideScanView::sendStartedInThread, corePtr_, &Core::setIsMosaicUpdatingInThread, Qt::QueuedConnection));
    }
}
