#ifndef SCENECONTROLLER_H
#define SCENECONTROLLER_H

#include <QVector2D>
#include <QRect>

#include <Q3DSceneModel.h>
#include <raycastpickerfactory.h>

class SceneController
{
public:
    SceneController(std::shared_ptr <Q3DSceneModel> model);

    void setModel(std::shared_ptr <Q3DSceneModel> model);
    void cursorPosChanged(const QVector3D& pos,
                          const QMatrix4x4& view,
                          const QMatrix4x4& model,
                          const QMatrix4x4& projection);

    void viewportRectChanged(const QRect& rect);

private:

    void updateObjectsPicker();
    void updateSceneTransform();

private:

    std::shared_ptr <Q3DSceneModel> mpSceneModel;

    QMatrix4x4 mView;
    QMatrix4x4 mModel;
    QMatrix4x4 mProjection;
    QVector3D mMousePos;
    QRect mViewportRect;
};

#endif // SCENECONTROLLER_H
