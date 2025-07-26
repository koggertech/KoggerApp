#include "isobaths_control_menu_controller.h"
#include "scene3d_view.h"


IsobathsControlMenuController::IsobathsControlMenuController(QObject* parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      dataProcessorPtr_(nullptr),
      pendingLambda_(nullptr),
      surfaceLineStepSize_(3.0f),
      themeId_(0),
      labelStepSize_(100),
      edgeLimit_(20),
      visibility_(false),
      edgesVisible_(false),
      trianglesVisible_(false),
      debugModeView_(false),
      processState_(true)
{
    qRegisterMetaType<DataProcessorType>("DataProcessorType");
}

void IsobathsControlMenuController::setGraphicsSceneView(GraphicsScene3dView* sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void IsobathsControlMenuController::setDataProcessorPtr(DataProcessor *dataProcessorPtr)
{
    dataProcessorPtr_ = dataProcessorPtr;
}

void IsobathsControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader");
}

void IsobathsControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (graphicsSceneViewPtr_) {
                if (dataProcessorPtr_) {
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateIsobaths",               Qt::QueuedConnection, Q_ARG(bool,  processState_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsSurfaceStepSize",      Qt::QueuedConnection, Q_ARG(float, surfaceLineStepSize_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsLineStepSize",         Qt::QueuedConnection, Q_ARG(float, surfaceLineStepSize_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsLabelStepSize",        Qt::QueuedConnection, Q_ARG(float, labelStepSize_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsColorTableThemeById",  Qt::QueuedConnection, Q_ARG(int,   themeId_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsEdgeLimit",            Qt::QueuedConnection, Q_ARG(int,   edgeLimit_));
                }

                if (auto isobathsPtr = graphicsSceneViewPtr_->getIsobathsPtr(); isobathsPtr) {
                    isobathsPtr->setVisible(visibility_);
                    isobathsPtr->setEdgesVisible(edgesVisible_);
                    isobathsPtr->setTrianglesVisible(trianglesVisible_);
                    isobathsPtr->setDebugMode(debugModeView_);
                }
            }
        };
    }
}

void IsobathsControlMenuController::onIsobathsVisibilityCheckBoxCheckedChanged(bool checked)
{
    visibility_ = checked;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setVisible(checked);

        if (visibility_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "onBottomTrackAdded", Qt::QueuedConnection, Q_ARG(QVector<int>, graphicsSceneViewPtr_->bottomTrack()->getAllIndxs()));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onUpdateIsobathsButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "onBottomTrackAdded", Qt::QueuedConnection, Q_ARG(QVector<int>, graphicsSceneViewPtr_->bottomTrack()->getAllIndxs()));
        }
    }
}

void IsobathsControlMenuController::onTrianglesVisible(bool state)
{
    trianglesVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setTrianglesVisible(trianglesVisible_);
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onEdgesVisible(bool state)
{
    edgesVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setEdgesVisible(edgesVisible_ );
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onSetSurfaceLineStepSize(float val)
{
    surfaceLineStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsSurfaceStepSize", Qt::QueuedConnection, Q_ARG(float, surfaceLineStepSize_));
            QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsLineStepSize",    Qt::QueuedConnection, Q_ARG(float, surfaceLineStepSize_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onSetLabelStepSize(int val)
{
    labelStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsLabelStepSize", Qt::QueuedConnection, Q_ARG(int , labelStepSize_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onThemeChanged(int val)
{
    themeId_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsColorTableThemeById", Qt::QueuedConnection, Q_ARG(int, themeId_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onDebugModeView(bool state)
{
    debugModeView_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setDebugMode(debugModeView_);
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onProcessStateChanged(bool state)
{
    processState_ = state;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateIsobaths", Qt::QueuedConnection, Q_ARG(bool, processState_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onResetIsobathsButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "clear", Qt::QueuedConnection, Q_ARG(DataProcessorType , DataProcessorType::kIsobaths));
        }

        graphicsSceneViewPtr_->getIsobathsPtr()->clear();
    }
}

void IsobathsControlMenuController::onEdgeLimitChanged(int val)
{
    edgeLimit_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsEdgeLimit", Qt::QueuedConnection, Q_ARG(int, edgeLimit_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}
