#include "isobaths_view_control_menu_controller.h"
#include "scene3d_view.h"


IsobathsViewControlMenuController::IsobathsViewControlMenuController(QObject* parent)
    : QmlComponentController(parent),
    graphicsSceneViewPtr_(nullptr),
    dataProcessorPtr_(nullptr),
    pendingLambda_(nullptr),
    surfaceLineStepSize_(3.0f),
    themeId_(0),
    labelStepSize_(100),
    edgeLimit_(70),
    extraWidth_(10),
    visibility_(false),
    edgesVisible_(false),
    trianglesVisible_(false),
    debugModeView_(false),
    processState_(true)
{
    qRegisterMetaType<DataProcessorType>("DataProcessorType");
}

void IsobathsViewControlMenuController::setGraphicsSceneView(GraphicsScene3dView* sceneView)
{
    graphicsSceneViewPtr_ = sceneView;

    if (graphicsSceneViewPtr_) {
        if (pendingLambda_) {
            pendingLambda_();
            pendingLambda_ = nullptr;
        }
    }
}

void IsobathsViewControlMenuController::setDataProcessorPtr(DataProcessor *dataProcessorPtr)
{
    dataProcessorPtr_ = dataProcessorPtr;
}

void IsobathsViewControlMenuController::findComponent()
{
    m_component = m_engine->findChild<QObject*>("activeObjectParamsMenuLoader");
}

void IsobathsViewControlMenuController::tryInitPendingLambda()
{
    if (!pendingLambda_) {
        pendingLambda_ = [this] () -> void {
            if (graphicsSceneViewPtr_) {
                graphicsSceneViewPtr_->setIsUpdateSurface(processState_);

                if (dataProcessorPtr_) {
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateSurface",                Qt::QueuedConnection, Q_ARG(bool,  processState_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateIsobaths",               Qt::QueuedConnection, Q_ARG(bool,  processState_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceIsobathsStepSize",      Qt::QueuedConnection, Q_ARG(float, surfaceLineStepSize_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsLabelStepSize",        Qt::QueuedConnection, Q_ARG(float, labelStepSize_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceColorTableThemeById",   Qt::QueuedConnection, Q_ARG(int,   themeId_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceEdgeLimit",             Qt::QueuedConnection, Q_ARG(int,   edgeLimit_));
                    QMetaObject::invokeMethod(dataProcessorPtr_, "setExtraWidth",                   Qt::QueuedConnection, Q_ARG(int,   extraWidth_));
                }

                if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
                    surfacePtr->setIVisible(visibility_);
                    surfacePtr->setIsobathsLabelStepSize(labelStepSize_);
                }
                //if (auto isobathsViewPtr = graphicsSceneViewPtr_->getIsobathsViewPtr(); isobathsViewPtr) {
                //    isobathsViewPtr->setVisible(visibility_);
                //}
            }
        };
    }
}

void IsobathsViewControlMenuController::onIsobathsVisibilityCheckBoxCheckedChanged(bool checked)
{
    visibility_ = checked;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->getSurfaceViewPtr()->setIVisible(checked);
        //graphicsSceneViewPtr_->getIsobathsViewPtr()->setVisible(checked);

        if (visibility_) {
            if (dataProcessorPtr_) {
                if (checked) {
                    //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kSurface));
                    //QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType, DataProcessorType::kIsobaths));
                    //QMetaObject::invokeMethod(dataProcessorPtr_, "onIsobathsUpdated", Qt::QueuedConnection);
                }
            }
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onUpdateIsobathsButtonClicked()
{
    //if (graphicsSceneViewPtr_) {
    //    if (dataProcessorPtr_) {
    //        QMetaObject::invokeMethod(dataProcessorPtr_, "onBottomTrackAdded", Qt::QueuedConnection,
    //                                  Q_ARG(QVector<int>, graphicsSceneViewPtr_->bottomTrack()->getAllIndxs()),
    //                                  Q_ARG(bool, false));
    //    }
    //}
}

void IsobathsViewControlMenuController::onTrianglesVisible(bool state)
{
    trianglesVisible_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onEdgesVisible(bool state)
{
    edgesVisible_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onSetSurfaceLineStepSize(float val)
{
    surfaceLineStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceIsobathsStepSize", Qt::QueuedConnection, Q_ARG(float, surfaceLineStepSize_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onSetLabelStepSize(int val)
{
    labelStepSize_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setIsobathsLabelStepSize", Qt::QueuedConnection, Q_ARG(int , labelStepSize_));
        }
        if (auto surfacePtr = graphicsSceneViewPtr_->getSurfaceViewPtr(); surfacePtr) {
            surfacePtr->setIsobathsLabelStepSize(labelStepSize_);
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onThemeChanged(int val)
{
    themeId_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceColorTableThemeById", Qt::QueuedConnection, Q_ARG(int, themeId_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onDebugModeView(bool state)
{
    debugModeView_ = state;

    if (graphicsSceneViewPtr_) {
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onProcessStateChanged(bool state)
{
    processState_ = state;

    if (graphicsSceneViewPtr_) {
        graphicsSceneViewPtr_->setIsUpdateSurface(processState_);

        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateSurface",  Qt::QueuedConnection, Q_ARG(bool, processState_));
            QMetaObject::invokeMethod(dataProcessorPtr_, "setUpdateIsobaths", Qt::QueuedConnection, Q_ARG(bool, processState_));
        }

        if (processState_) {
            graphicsSceneViewPtr_->onCameraMoved();
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onResetIsobathsButtonClicked()
{
    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "clearProcessing", Qt::QueuedConnection, Q_ARG(DataProcessorType , DataProcessorType::kIsobaths));
        }

        //graphicsSceneViewPtr_->getIsobathsViewPtr()->clear();
    }
}

void IsobathsViewControlMenuController::onEdgeLimitChanged(int val)
{
    edgeLimit_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setSurfaceEdgeLimit", Qt::QueuedConnection, Q_ARG(int, edgeLimit_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}

void IsobathsViewControlMenuController::onSetExtraWidth(int val)
{
    extraWidth_ = val;

    if (graphicsSceneViewPtr_) {
        if (dataProcessorPtr_) {
            QMetaObject::invokeMethod(dataProcessorPtr_, "setExtraWidth", Qt::QueuedConnection, Q_ARG(int, extraWidth_));
        }
    }
    else {
        tryInitPendingLambda();
    }
}
