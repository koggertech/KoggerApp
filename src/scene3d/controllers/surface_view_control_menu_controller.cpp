#include "surface_view_control_menu_controller.h"
#include "scene3d_view.h"


SurfaceViewControlMenuController::SurfaceViewControlMenuController(QObject* parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      pendingLambda_(nullptr),
      visibility_(false)
{}

void SurfaceViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView* sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void SurfaceViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader");
}

void SurfaceViewControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (graphicsSceneViewPtr_) {
                graphicsSceneViewPtr_->setUpdateIsobaths(processState_);
                if (auto surfaceViewPtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfaceViewPtr) {
                    surfaceViewPtr->setVisible(visibility_);
                    surfaceViewPtr->setColorTableThemeById(themeId_);
                    surfaceViewPtr->setSurfaceStepSize(surfaceLineStepSize_);
                    surfaceViewPtr->setLineStepSize(surfaceLineStepSize_);
                    surfaceViewPtr->onEdgesVisible(edgesVisible_);
                    surfaceViewPtr->onTrianglesVisible(trianglesVisible_);
                    surfaceViewPtr->setLabelStepSize(labelStepSize_);
                    surfaceViewPtr->setDebugMode(debugModeView_);
                    surfaceViewPtr->onProcessStateChanged(processState_);
                    surfaceViewPtr->setEdgeLimit(edgeLimit_);
                    surfaceViewPtr->setHandleXCall(handleXCall_);
                }
            }
        };
    }
}

void SurfaceViewControlMenuController::onSurfaceViewVisibilityCheckBoxCheckedChanged(bool checked)
{
    visibility_ = checked;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setVisible(checked);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onUpdateSurfaceViewButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->onAction();
    }
}

void SurfaceViewControlMenuController::onTrianglesVisible(bool state)
{
    trianglesVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->onTrianglesVisible(trianglesVisible_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onEdgesVisible(bool state)
{
    edgesVisible_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->onEdgesVisible(edgesVisible_ );
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onSetSurfaceLineStepSize(float val)
{
    surfaceLineStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setSurfaceStepSize(surfaceLineStepSize_);
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setLineStepSize(surfaceLineStepSize_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onSetLabelStepSize(int val)
{
    labelStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setLabelStepSize(labelStepSize_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onThemeChanged(int val)
{
    themeId_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setColorTableThemeById(themeId_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onDebugModeView(bool state)
{
    debugModeView_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setDebugMode(debugModeView_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onProcessStateChanged(bool state)
{
    processState_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->setUpdateIsobaths(processState_);
        graphicsSceneViewPtr_->getSurfaceViewPtr()->onProcessStateChanged(processState_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onResetSurfaceViewButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->clear();
    }
}

void SurfaceViewControlMenuController::onEdgeLimitChanged(int val)
{
    edgeLimit_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setEdgeLimit(edgeLimit_);
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onHandleXCallChanged(int val)
{
    handleXCall_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setHandleXCall(handleXCall_);
    }
    else {
        tryInitPendingLambda();
    }
}
