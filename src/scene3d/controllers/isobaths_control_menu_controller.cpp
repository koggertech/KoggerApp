#include "isobaths_control_menu_controller.h"
#include "scene3d_view.h"


IsobathsControlMenuController::IsobathsControlMenuController(QObject* parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      pendingLambda_(nullptr),
      visibility_(false)
{}

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
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateIsobaths", Qt::QueuedConnection, Q_ARG(bool, processState_));
                }

                if (auto isobathsPtr = graphicsSceneViewPtr_->getIsobathsPtr(); isobathsPtr) {
                    isobathsPtr->setVisible(visibility_);
                    isobathsPtr->setColorTableThemeById(themeId_);
                    isobathsPtr->setSurfaceStepSize(surfaceLineStepSize_);
                    isobathsPtr->setLineStepSize(surfaceLineStepSize_);
                    isobathsPtr->onEdgesVisible(edgesVisible_);
                    isobathsPtr->onTrianglesVisible(trianglesVisible_);
                    isobathsPtr->setLabelStepSize(labelStepSize_);
                    isobathsPtr->setDebugMode(debugModeView_);
                    isobathsPtr->onProcessStateChanged(processState_);
                    isobathsPtr->setEdgeLimit(edgeLimit_);
                    isobathsPtr->setHandleXCall(handleXCall_);
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
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onUpdateIsobathsButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->updateIsobathsForAllData();
    }
}

void IsobathsControlMenuController::onTrianglesVisible(bool state)
{
    trianglesVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->onTrianglesVisible(trianglesVisible_);
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onEdgesVisible(bool state)
{
    edgesVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->onEdgesVisible(edgesVisible_ );
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onSetSurfaceLineStepSize(float val)
{
    surfaceLineStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setSurfaceStepSize(surfaceLineStepSize_);
        graphicsSceneViewPtr_->getIsobathsPtr()->setLineStepSize(surfaceLineStepSize_);
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onSetLabelStepSize(int val)
{
    labelStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setLabelStepSize(labelStepSize_);
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onThemeChanged(int val)
{
    themeId_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setColorTableThemeById(themeId_);
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

        graphicsSceneViewPtr_->getIsobathsPtr()->onProcessStateChanged(processState_);
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onResetIsobathsButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->clear();
    }
}

void IsobathsControlMenuController::onEdgeLimitChanged(int val)
{
    edgeLimit_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setEdgeLimit(edgeLimit_);
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onHandleXCallChanged(int val)
{
    handleXCall_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setHandleXCall(handleXCall_);
    }
    else {
        tryInitPendingLambda();
    }
}
