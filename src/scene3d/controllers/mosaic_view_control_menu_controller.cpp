#include "mosaic_view_control_menu_controller.h"

#include "scene3d_view.h"
#include "data_processor.h"


MosaicViewControlMenuController::MosaicViewControlMenuController(QObject *parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      dataProcessorPtr_(nullptr),
      pendingLambda_(nullptr),
      visibility_(false),
      usingFilter_(true),
      gridVisible_(false),
      measLineVisible_(false),
      resolution_(10.0f), // pixPerMeters
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

void MosaicViewControlMenuController::onVisibilityChanged(bool state)
{
    visibility_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setMVisible(visibility_);
        graphicsSceneViewPtr_->getIsobathsViewPtr()->setMVisible(visibility_);

        if (state) {
                //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kSurface));
                //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kMosaic));
                //QMetaObject::invokeMethod(dataProcessorPtr_, "onMosaicUpdated", Qt::QueuedConnection);
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onUseFilterChanged(bool state)
{
    usingFilter_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onGridVisibleChanged(bool state)
{
    gridVisible_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onMeasLineVisibleChanged(bool state)
{
    measLineVisible_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onClearClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->clear();
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
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicTheme", Qt::QueuedConnection, Q_ARG(int, themeId_));
        }
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
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicLevels", Qt::QueuedConnection, Q_ARG(float, lowLevel_), Q_ARG(float, highLevel_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onUpdateClicked()
{
    //if (graphicsSceneViewPtr_) {
    //    if (dataProcessorPtr_) {
    //        //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kSurface));
    //        QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kMosaic));
    //        QMetaObject::invokeMethod(dataProcessorPtr_, "onBottomTrackAdded", Qt::QueuedConnection,
    //                                  Q_ARG(QVector<int>, graphicsSceneViewPtr_->bottomTrack()->getAllIndxs()),
    //                                  Q_ARG(bool, true));
    //    }
    //}
}

void MosaicViewControlMenuController::onSetLAngleOffset(float val)
{
    lAngleOffset_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicLAngleOffset", Qt::QueuedConnection, Q_ARG(float, lAngleOffset_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onSetRAngleOffset(float val)
{
    rAngleOffset_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicRAngleOffset", Qt::QueuedConnection, Q_ARG(float, rAngleOffset_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::onSetResolution(float val)
{
    resolution_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicTileResolution", Qt::QueuedConnection, Q_ARG(float, resolution_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void MosaicViewControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this](){
            if (graphicsSceneViewPtr_) {
                if (dataProcessorPtr_) {
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateMosaic",              Qt::QueuedConnection, Q_ARG(bool,  updateState_));
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicTheme",               Qt::QueuedConnection, Q_ARG(int,   themeId_));
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicLevels",              Qt::QueuedConnection, Q_ARG(float, lowLevel_), Q_ARG(float, highLevel_));
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicLAngleOffset",        Qt::QueuedConnection, Q_ARG(float, lAngleOffset_));
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicRAngleOffset",        Qt::QueuedConnection, Q_ARG(float, rAngleOffset_));
                   QMetaObject::invokeMethod(dataProcessorPtr_, "setMosaicTileResolution",      Qt::QueuedConnection, Q_ARG(float, resolution_));
                }

                if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
                    surfacePtr->setMVisible(visibility_);
                }
                if (auto isobathsPtr = graphicsSceneViewPtr_->getIsobathsViewPtr(); isobathsPtr) {
                    isobathsPtr->setMVisible(visibility_);
                }
            }
        };
    }
}

void MosaicViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("mosaicViewControlMenu");
}
