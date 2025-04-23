#include "isobaths_control_menu_controller.h"
#include "scene3d_view.h"


IsobathsControlMenuController::IsobathsControlMenuController(QObject* parent)
    : QmlComponentController(parent),
      graphicsSceneViewPtr_(nullptr),
      pendingLambda_(nullptr),
      visibility_(false),
      surfaceStepSize_(3.0f),
      lineStepSize_(3.0f),
      labelStepSize_(100)
{
    QObject::connect(&isobathsProcessor_, &IsobathsProcessor::taskStarted, this, &IsobathsControlMenuController::isobathsProcessorTaskStarted);

    QObject::connect(&isobathsProcessor_, &IsobathsProcessor::taskFinished,
                     this,                [this](IsobathsProcessorResult result) {
                                              if (graphicsSceneViewPtr_) {
                                                  if (auto isobathsPtr = graphicsSceneViewPtr_->getIsobathsPtr(); isobathsPtr) {
                                                      isobathsPtr->setProcessorResult(result);
                                                  }
                                              }
                                              Q_EMIT isobathsProcessorTaskFinished();
                                          });
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

void IsobathsControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader");
}

void IsobathsControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (graphicsSceneViewPtr_) {
                if (auto isobathsPtr = graphicsSceneViewPtr_->getIsobathsPtr(); isobathsPtr) {
                    isobathsPtr->setVisible(visibility_);
                    isobathsPtr->setSurfaceStepSize(surfaceStepSize_);
                    isobathsPtr->setLineStepSize(lineStepSize_);
                    isobathsPtr->setLabelStepSize(labelStepSize_);
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

        if (checked) {
            onUpdateIsobathsButtonClicked();
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onUpdateIsobathsButtonClicked()
{
    qDebug() << "   onUpdateIsobathsButtonClicked";

    if (!graphicsSceneViewPtr_) {
        qDebug() << "graphicsSceneViewPtr_ is nullptr!";
        return;
    }

    if (isobathsProcessor_.isBusy()) {
        qDebug() << "Isobaths processor is busy!";
        return;
    }

    auto* sur = graphicsSceneViewPtr_->surface().get();
    auto* iso = graphicsSceneViewPtr_->getIsobathsPtr().get();

    if (!sur || !iso) {
        qDebug() << "sur or iso is nullptr";
        return;
    }

    // duplitate surface data for isobaths
    QMetaObject::invokeMethod(iso,
                              "setData",
                              Qt::QueuedConnection,
                              Q_ARG(QVector<QVector3D>, sur->getRawData()),
                              Q_ARG(int, sur->getPrimitiveType()));

    // recalc
    QMetaObject::invokeMethod(this,
                              "onSetSurfaceStepSizeIsobaths",
                              Qt::QueuedConnection,
                              Q_ARG(float, iso->getSurfaceStepSize()));

    QMetaObject::invokeMethod(this,
                              "onSetLineStepSizeIsobaths",
                              Qt::QueuedConnection,
                              Q_ARG(float, iso->getLineStepSize()));
}

void IsobathsControlMenuController::onSetSurfaceStepSizeIsobaths(float val)
{
    qDebug() << "   onSetSurfaceStepSizeIsobaths" << val;

    surfaceStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getIsobathsPtr()->setSurfaceStepSize(val);
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onSetLineStepSizeIsobaths(float val)
{
    qDebug() << "   onSetLineStepSizeIsobaths" << val;

    lineStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        if (auto isobathsPtr = graphicsSceneViewPtr_->getIsobathsPtr(); isobathsPtr) {

            isobathsPtr->setLineStepSize(lineStepSize_);

            if (isobathsProcessor_.isBusy()) {
                qDebug().noquote() << "Isobaths processor is busy!";
                return;
            }

            IsobathsProcessorTask task;
            task.grid = isobathsPtr->getRawData();
            task.gridWidth = isobathsPtr->getGridWidth();
            task.gridHeight = isobathsPtr->getGridHeight();
            task.step = isobathsPtr->getLineStepSize();
            task.labelStep = isobathsPtr->getLabelStepSize();

            isobathsProcessor_.startInThread(task);
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsControlMenuController::onSetLabelStepSizeIsobaths(int val)
{
    qDebug() << "   onSetLabelStepSizeIsobaths" << val;

    labelStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        if (auto isobathsPtr = graphicsSceneViewPtr_->getIsobathsPtr(); isobathsPtr) {

            isobathsPtr->setLabelStepSize(static_cast<float>(labelStepSize_));

            if (isobathsProcessor_.isBusy()) {
                qDebug().noquote() << "Isobaths processor is busy!";
                return;
            }

            IsobathsProcessorTask task;
            task.grid = isobathsPtr->getRawData();
            task.gridWidth = isobathsPtr->getGridWidth();
            task.gridHeight = isobathsPtr->getGridHeight();
            task.step = isobathsPtr->getLineStepSize();
            task.labelStep = isobathsPtr->getLabelStepSize();

            isobathsProcessor_.startInThread(task);
        }
    }
    else {
        tryInitPendingLambda();
    }
}
