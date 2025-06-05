#include "surface_view_control_menu_controller.h"
#include "scene3d_view.h"


SurfaceViewControlMenuController::SurfaceViewControlMenuController(QObject* parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      pendingLambda_(nullptr),
      visibility_(false)
{
    QObject::connect(&surfaceViewProcessor_, &SurfaceViewProcessor::taskStarted, this, &SurfaceViewControlMenuController::surfaceViewProcessorTaskStarted);

    QObject::connect(&surfaceViewProcessor_, &SurfaceViewProcessor::taskFinished,
                     this,                [this](SurfaceViewProcessorResult result) {
                                              Q_UNUSED(result)
                                              if (graphicsSceneViewPtr_) {
                                                  if (auto surfaceViewPtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfaceViewPtr) {
                                                      //surfaceViewPtr->setProcessorResult(result);
                                                  }
                                              }
                                              Q_EMIT surfaceViewProcessorTaskFinished();
                                          });
}

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
                if (auto isobathsPtr = graphicsSceneViewPtr_->getIsobathsPtr(); isobathsPtr) {
                    isobathsPtr->setVisible(visibility_);
                }
            }
        };
    }
}

void SurfaceViewControlMenuController::onSurfaceViewVisibilityCheckBoxCheckedChanged(bool checked)
{
    qDebug() << "   onSurfaceViewVisibilityCheckBoxCheckedChanged" << checked;

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
    qDebug() << "   onUpdateSurfaceViewButtonClicked";

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->onAction();
    }
}

void SurfaceViewControlMenuController::onTrianglesVisible(bool state)
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->onTrianglesVisible(state);
    }
}

void SurfaceViewControlMenuController::onEdgesVisible(bool state)
{
    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->onEdgesVisible(state);
    }
}
