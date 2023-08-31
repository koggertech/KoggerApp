#include "scenecontroller.h"
#include "raycastpickerfactory.h"

SceneController::SceneController()
{}

SceneController::~SceneController()
{

}

void SceneController::cursorPosChanged(const QVector3D& pos,
                                       const QMatrix4x4& view,
                                       const QMatrix4x4& model,
                                       const QMatrix4x4& projection)
{
    mView = view;
    mModel = model;
    mProjection = projection;
    mMousePos = pos;

    //if (mpSceneModel->pickingMethod() != PICKING_METHOD_NONE){
    //    updateObjectsPicker();
    //    return;
    //}

    updateSceneTransform();
}

void SceneController::viewportRectChanged(const QRect &rect)
{
    mViewportRect = rect;
}

void SceneController::updateObjectsPicker()
{
    auto origin = QVector3D(mMousePos.toVector2D(), -1.0f).unproject(mModel * mView, mProjection, mViewportRect);
    auto end = QVector3D(mMousePos.toVector2D(), 1.0f).unproject(mModel * mView, mProjection, mViewportRect);
    //auto dir = (end - origin).normalized();
    auto dir = end - origin;

    auto pickerFactory = std::make_shared <RayCastPickerFactory>(origin, dir);

    std::shared_ptr <AbstractPicker> pPicker = nullptr;

    pPicker = pickerFactory->createPointPicker();

    //mpSceneModel->setObjectsPicker(pPicker);
}

void SceneController::updateSceneTransform()
{

}
