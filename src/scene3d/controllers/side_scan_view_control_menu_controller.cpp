#include "side_scan_view_control_menu_controller.h"
#include "scene3d_view.h"
#include "side_scan_view.h"
#include "core.h"


SideScanViewControlMenuController::SideScanViewControlMenuController(QObject *parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
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
      themeId_(0),
      lowLevel_(10.0f),
      highLevel_(90.0f),
      lAngleOffset_(0.0f),
      rAngleOffset_(0.0f)
{

}

void SideScanViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
{
    graphicsSceneViewPtr_ = sceneView;
    tryClearMakeConnections();

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void SideScanViewControlMenuController::setCorePtr(Core* corePtr)
{
    corePtr_ = corePtr;

    tryClearMakeConnections();
}

void SideScanViewControlMenuController::onVisibilityChanged(bool state)
{
    visibility_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->setVisible(visibility_);
        graphicsSceneViewPtr_->bottomTrack()->setSideScanVisibleState(visibility_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onUseFilterChanged(bool state)
{
    usingFilter_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->setUseLinearFilter(usingFilter_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onGridVisibleChanged(bool state)
{
    gridVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->setTileGridVisible(gridVisible_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onMeasLineVisibleChanged(bool state)
{
    measLineVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->setMeasLineVisible(measLineVisible_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onClearClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->clear(false);
    }
}

void SideScanViewControlMenuController::onGlobalMeshChanged(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->resetTileSettings(tileSidePixelSize, tileHeightMatrixRatio, tileResolution);
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
    generateGridContour_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->setGenerateGridContour(generateGridContour_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onUpdateStateChanged(bool state)
{
    updateState_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->setUpdateMosaic(updateState_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onThemeChanged(int val)
{
    themeId_ = val + 1;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->setColorTableThemeById(themeId_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onLevelChanged(float lowLevel, float highLevel)
{
    lowLevel_ = lowLevel;
    highLevel_ = highLevel;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->setColorTableLevels(lowLevel_, highLevel_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onUpdateClicked()
{
    if (graphicsSceneViewPtr_) {
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
        graphicsSceneViewPtr_->getSideScanViewPtr()->setWorkMode(SideScanView::Mode::kPerformance);
        graphicsSceneViewPtr_->interpolateDatasetEpochs(true);
        graphicsSceneViewPtr_->bottomTrack()->sideScanUpdated();
        graphicsSceneViewPtr_->getSideScanViewPtr()->startUpdateDataInThread(0, 0);
    }
}

void SideScanViewControlMenuController::onSetLAngleOffset(float val)
{
    lAngleOffset_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->setLAngleOffset(lAngleOffset_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SideScanViewControlMenuController::onSetRAngleOffset(float val)
{
    rAngleOffset_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSideScanViewPtr()->setRAngleOffset(rAngleOffset_);
    }
    else {
        tryInitPendingLambda();
    }
}

SideScanView* SideScanViewControlMenuController::getSideScanViewPtr() const
{
    if (graphicsSceneViewPtr_) {
        return graphicsSceneViewPtr_->getSideScanViewPtr().get();
    }

    return nullptr;
}

void SideScanViewControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this](){
            if (graphicsSceneViewPtr_) {
                graphicsSceneViewPtr_->setUpdateMosaic(updateState_);
                if (auto sideScanPtr = graphicsSceneViewPtr_->getSideScanViewPtr(); sideScanPtr) {
                    sideScanPtr->setVisible(visibility_);
                    sideScanPtr->setUseLinearFilter(usingFilter_);
                    sideScanPtr->setTileGridVisible(gridVisible_);
                    sideScanPtr->setMeasLineVisible(measLineVisible_);
                    //sideScanPtr->resetTileSettings(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                    sideScanPtr->setGenerateGridContour(generateGridContour_);
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

    if (graphicsSceneViewPtr_ && corePtr_) {
        connections_.append(QObject::connect(graphicsSceneViewPtr_->getSideScanViewPtr().get(), &SideScanView::sendStartedInThread, corePtr_, &Core::setIsMosaicUpdatingInThread, Qt::QueuedConnection));
        connections_.append(QObject::connect(graphicsSceneViewPtr_->getSideScanViewPtr().get(), &SideScanView::sendUpdatedWorkMode, corePtr_, &Core::setSideScanWorkMode, Qt::QueuedConnection));
    }
}
