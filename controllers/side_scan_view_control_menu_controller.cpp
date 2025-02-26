#include "side_scan_view_control_menu_controller.h"
#include "graphicsscene3dview.h"
#include "side_scan_view.h"
#include "core.h"


SideScanViewControlMenuController::SideScanViewControlMenuController(QObject *parent) :
    QmlComponentController(parent),
    m_graphicsSceneView(nullptr),
    corePtr_(nullptr),
    pendingLambda_(nullptr),
    visibility_(false),
    usingFilter_(false),
    gridVisible_(false),
    measLineVisible_(false),
    //tileSidePixelSize_(256),
    //tileHeightMatrixRatio_(16),
    //tileResolution_(1.0f/10.0f),
    generateGridContour_(false),
    updateState_(false),
    trackLastEpoch_(false),
    themeId_(0),
    lowLevel_(10.0f),
    highLevel_(90.0f),
    lAngleOffset_(0.0f),
    rAngleOffset_(0.0f)
{

}

void SideScanViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    m_graphicsSceneView = sceneView;

    tryClearMakeConnections();

    if (pendingLambda_) {
        pendingLambda_();
        pendingLambda_ = nullptr;
    }
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
        m_graphicsSceneView->bottomTrack()->setSideScanVisibleState(state);
    }
    else {
        visibility_ = state;
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onUseFilterChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setUseLinearFilter(state);
    }
    else {
        usingFilter_ = state;
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onGridVisibleChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setTileGridVisible(state);
    }
    else {
        gridVisible_ = state;
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onMeasLineVisibleChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setMeasLineVisible(state);
    }
    else {
        measLineVisible_ = state;
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onClearClicked()
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->clear(false);
    }
}

void SideScanViewControlMenuController::onGlobalMeshChanged(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->resetTileSettings(tileSidePixelSize, tileHeightMatrixRatio, tileResolution);
    }
    //else {
    //    tileSidePixelSize_ = tileSidePixelSize;
    //    tileHeightMatrixRatio_ = tileHeightMatrixRatio;
    //    tileResolution_ = tileResolution;
    //    tryInitPendingLambda();
    //}
}

void SideScanViewControlMenuController::onGenerateGridContourChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setGenerateGridContour(state);
    }
    else {
        generateGridContour_ = state;
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onUpdateStateChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->setCalcStateSideScanView(state);
    }
    else {
        updateState_ = state;
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onTrackLastEpochChanged(bool state)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setTrackLastEpoch(state);
    }
    else {
        trackLastEpoch_ = state;
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onThemeChanged(int val)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setColorTableThemeById(val + 1);
    }
    else {
        themeId_ = val + 1;
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onLevelChanged(float lowLevel, float highLevel)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setColorTableLevels(lowLevel, highLevel);
    }
    else {
        lowLevel_ = lowLevel;
        highLevel_ = highLevel;
        tryInitPendingLambda();
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
        m_graphicsSceneView->interpolateDatasetEpochs(true);
        m_graphicsSceneView->bottomTrack()->sideScanUpdated();
        m_graphicsSceneView->getSideScanViewPtr()->startUpdateDataInThread(0, 0);
    }
}

void SideScanViewControlMenuController::onSetLAngleOffset(float val)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setLAngleOffset(val);
    }
    else {
        lAngleOffset_ = val;
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onSetRAngleOffset(float val)
{
    if (m_graphicsSceneView) {
        m_graphicsSceneView->getSideScanViewPtr()->setRAngleOffset(val);
    }
    else {
        rAngleOffset_ = val;
        tryInitPendingLambda();
    }
}

SideScanView* SideScanViewControlMenuController::getSideScanViewPtr() const
{
    if (m_graphicsSceneView) {
        return m_graphicsSceneView->getSideScanViewPtr().get();
    }
    return nullptr;
}

void SideScanViewControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this](){
            if (m_graphicsSceneView) {
                m_graphicsSceneView->setCalcStateSideScanView(updateState_);
                if (auto sideScanPtr = m_graphicsSceneView->getSideScanViewPtr(); sideScanPtr) {
                    sideScanPtr->setVisible(visibility_);
                    sideScanPtr->setUseLinearFilter(usingFilter_);
                    sideScanPtr->setTileGridVisible(gridVisible_);
                    sideScanPtr->setMeasLineVisible(measLineVisible_);
                    //sideScanPtr->resetTileSettings(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                    sideScanPtr->setGenerateGridContour(generateGridContour_);
                    sideScanPtr->setTrackLastEpoch(trackLastEpoch_);
                    sideScanPtr->setColorTableThemeById(themeId_);
                    sideScanPtr->setColorTableLevels(lowLevel_, highLevel_);
                    sideScanPtr->setLAngleOffset(lAngleOffset_);
                    sideScanPtr->setRAngleOffset(rAngleOffset_);
                }
            }
        };
    }
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
        connections_.append(QObject::connect(m_graphicsSceneView->getSideScanViewPtr().get(), &SideScanView::sendUpdatedWorkMode, corePtr_, &Core::setSideScanWorkMode, Qt::QueuedConnection));
    }
}
