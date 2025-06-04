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

        // force update
        //if (checked) {
        //    onUpdateSurfaceViewButtonClicked();
        //}
    }
    else {
        tryInitPendingLambda();
    }
}

void SurfaceViewControlMenuController::onUpdateSurfaceViewButtonClicked()
{
    qDebug() << "   onUpdateSurfaceViewButtonClicked";

    if (!graphicsSceneViewPtr_) {
        qDebug() << "graphicsSceneViewPtr_ is nullptr!";
        return;
    }

    if (surfaceViewProcessor_.isBusy()) {
        qDebug() << "Isobaths processor is busy!";
        return;
    }

    auto* sur = graphicsSceneViewPtr_->surface().get();
    auto* iso = graphicsSceneViewPtr_->getIsobathsPtr().get();

    if (!sur || !iso) {
        qDebug() << "sur or iso is nullptr";
        return;
    }

    //// duplitate surface data for isobaths

    //// recalc
}
