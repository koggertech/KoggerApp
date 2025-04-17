#include "isobaths_control_menu_controller.h"
#include "qml_object_names.h"
#include "scene3d_view.h"
#include "isobaths.h"


IsobathsControlMenuController::IsobathsControlMenuController(QObject* parent) :
    QmlComponentController(parent)
{
    QObject::connect(&isobathsProcessor_, &IsobathsProcessor::taskStarted, this, &IsobathsControlMenuController::isobathsProcessorTaskStarted);

    QObject::connect(&isobathsProcessor_, &IsobathsProcessor::taskFinished,
                     this,                [this](IsobathsProcessor::IsobathProcessorResult result) {
                                              if (!graphicsSceneViewPtr_) {
                                                  return;
                                              }

                                              if (auto* isobathsPtr = graphicsSceneViewPtr_->getIsobathsPtr().get(); isobathsPtr) {
                                                  QMetaObject::invokeMethod(isobathsPtr,
                                                                            "setData",
                                                                            Qt::QueuedConnection,
                                                                            Q_ARG(QVector<QVector3D>, result.data),
                                                                            Q_ARG(int, 1) /*gl primitive type*/);
                                                  isobathsPtr->setProcessingTask(isobathsProcessor_.ctask());
                                              }

                                              Q_EMIT isobathsProcessorTaskFinished();
                                          });
}

void IsobathsControlMenuController::setGraphicsSceneView(GraphicsScene3dView* sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (pendingLambda_) {
        pendingLambda_();
        pendingLambda_ = nullptr;
    }
}

void IsobathsControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader");
}

Isobaths* IsobathsControlMenuController::isobaths() const
{
    if (graphicsSceneViewPtr_) {
        return graphicsSceneViewPtr_->getIsobathsPtr().get();
    }

    return nullptr;
}

void IsobathsControlMenuController::tryInitPendingLambda()
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

void IsobathsControlMenuController::onIsobathsVisibilityCheckBoxCheckedChanged(bool checked)
{
    qDebug() << "   onIsobathsVisibilityCheckBoxCheckedChanged" << checked;

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
    qDebug() << "   onUpdateIsobathsButtonClicked";

    if (!graphicsSceneViewPtr_) {
        qDebug().noquote() << "graphicsSceneViewPtr_ is nullptr!";
        return;
    }

    if (isobathsProcessor_.isBusy()) {
        qDebug().noquote() << "Isobaths processor is busy!";
        return;
    }

    IsobathsProcessorTask task;

    isobathsProcessor_.startInThread(task);
}
