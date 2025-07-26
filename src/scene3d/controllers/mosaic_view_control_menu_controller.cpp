#include "mosaic_view_control_menu_controller.h"
#include "scene3d_view.h"
#include "mosaic_view.h"
#include "core.h"


MosaicViewControlMenuController::MosaicViewControlMenuController(QObject *parent)
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

void MosaicViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView *sceneView)
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

void MosaicViewControlMenuController::setDataProcessorPtr(DataProcessor *dataProcessorPtr)
{
    dataProcessorPtr_ = dataProcessorPtr;
}

void MosaicViewControlMenuController::setCorePtr(Core* corePtr)
{
    corePtr_ = corePtr;

    tryClearMakeConnections();
}

void MosaicViewControlMenuController::onVisibilityChanged(bool state)
{
    visibility_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getMosaicViewPtr()->setVisible(visibility_);
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onUseFilterChanged(bool state)
{
    usingFilter_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getMosaicViewPtr()->setUseLinearFilter(usingFilter_);
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onGridVisibleChanged(bool state)
{
    gridVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getMosaicViewPtr()->setTileGridVisible(gridVisible_);
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onMeasLineVisibleChanged(bool state)
{
    measLineVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getMosaicViewPtr()->setMeasLineVisible(measLineVisible_);
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onClearClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getMosaicViewPtr()->clear();
    }
}

void MosaicViewControlMenuController::onGlobalMeshChanged(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution)
{
    if (graphicsSceneViewPtr_) {
        // TODO
        //graphicsSceneViewPtr_->getMosaicViewPtr()->resetTileSettings(tileSidePixelSize, tileHeightMatrixRatio, tileResolution);
    }
    //else {
    //    tileSidePixelSize_ = tileSidePixelSize;
    //    tileHeightMatrixRatio_ = tileHeightMatrixRatio;
    //    tileResolution_ = tileResolution;
    //    tryInitPendingLambda();
    //}
}

void MosaicViewControlMenuController::onGenerateGridContourChanged(bool state)
{
    generateGridContour_ = state;

    if (graphicsSceneViewPtr_) {
        // TODO
        //graphicsSceneViewPtr_->getMosaicViewPtr()->setGenerateGridContour(generateGridContour_);
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onUpdateStateChanged(bool state)
{
    updateState_ = state;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateMosaic", Qt::QueuedConnection, Q_ARG(bool, updateState_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onThemeChanged(int val)
{
    themeId_ = val + 1;

    if (graphicsSceneViewPtr_) {
        // TODO
        //graphicsSceneViewPtr_->getMosaicViewPtr()->setColorTableThemeById(themeId_);
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onLevelChanged(float lowLevel, float highLevel)
{
    lowLevel_ = lowLevel;
    highLevel_ = highLevel;

    if (graphicsSceneViewPtr_) {
        // TODO
        //graphicsSceneViewPtr_->getMosaicViewPtr()->setColorTableLevels(lowLevel_, highLevel_);
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onUpdateClicked()
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
        // TODO
        //graphicsSceneViewPtr_->getMosaicViewPtr()->startUpdateDataInThread(0, 0);
    }
}

void MosaicViewControlMenuController::onSetLAngleOffset(float val)
{
    lAngleOffset_ = val;

    if (graphicsSceneViewPtr_) {
        // TODO
        //graphicsSceneViewPtr_->getMosaicViewPtr()->setLAngleOffset(lAngleOffset_);
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onSetRAngleOffset(float val)
{
    rAngleOffset_ = val;

    if (graphicsSceneViewPtr_) {
        // TODO
        //graphicsSceneViewPtr_->getMosaicViewPtr()->setRAngleOffset(rAngleOffset_);
    }
    else {
        tryInitPendingLambda();
    }
}

MosaicView* MosaicViewControlMenuController::getMosaicViewPtr() const
{
    if (graphicsSceneViewPtr_) {
        return graphicsSceneViewPtr_->getMosaicViewPtr().get();
    }

    return nullptr;
}

void MosaicViewControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this](){
            if (graphicsSceneViewPtr_) {

                if (dataProcessorPtr_) {
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateMosaic", Qt::QueuedConnection, Q_ARG(bool, updateState_));
                }

                if (auto mosaicPtr = graphicsSceneViewPtr_->getMosaicViewPtr(); mosaicPtr) {
                    mosaicPtr->setVisible(visibility_);
                    mosaicPtr->setUseLinearFilter(usingFilter_);
                    mosaicPtr->setTileGridVisible(gridVisible_);
                    mosaicPtr->setMeasLineVisible(measLineVisible_);
                    //mosaicPtr->resetTileSettings(tileSidePixelSize_, tileHeightMatrixRatio_, tileResolution_);
                    // TODO
                    //mosaicPtr->setGenerateGridContour(generateGridContour_);
                    //mosaicPtr->setColorTableThemeById(themeId_);
                    //mosaicPtr->setColorTableLevels(lowLevel_, highLevel_);
                    //mosaicPtr->setLAngleOffset(lAngleOffset_);
                    //mosaicPtr->setRAngleOffset(rAngleOffset_);
                }
            }
        };
    }
}

void MosaicViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("mosaicViewControlMenu");
}

void MosaicViewControlMenuController::tryClearMakeConnections()
{
    for (auto& itm : connections_) {
        disconnect(itm);
    }
    connections_.clear();
}
