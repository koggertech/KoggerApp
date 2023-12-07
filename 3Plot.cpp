#include "3Plot.h"
#include <bottomtrack.h>
#include <surface.h>

#include <QOpenGLFramebufferObject>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>
#include <QRandomGenerator>

#include <iostream>

#include <polygonobject.h>

class FboRenderer : public QQuickFramebufferObject::Renderer
{
public:
    FboRenderer()
    {
        scene.initialize();
    }

    void render() override {
        scene.render();
        update();
    }

    //void setSceneObjectsListModel(std::shared_ptr <SceneObjectsListModel> sceneObjectsListModel){
    //    //scene.setSceneObjectsListModel(sceneObjectsListModel);
    //}

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    void synchronize(QQuickFramebufferObject *item) override {
        FboInSGRenderer *fbitem = static_cast<FboInSGRenderer *>(item);
        //scene.setLines(fbitem->lines());

        scene.scale(fbitem->scaleDelta());
        //scene.size(fbitem->size());
        scene.mouse(fbitem->mouse());
        scene.rotationFlag(fbitem->isRotation());
        //scene.setRightMouseButtonPressed(fbitem->isRightMouseButtonPressed());
        //scene.setMousePos(fbitem->mousePos());
        //scene.setSize(QSize(fbitem->width(), fbitem->height()));
    }

    Scene3D scene;
};

QQuickFramebufferObject::Renderer *FboInSGRenderer::createRenderer() const
{
    auto renderer = new FboRenderer();

    return renderer;
}

#include <QPainter>
#include <QPaintEngine>
#include <qmath.h>
#include <QtOpenGL/QtOpenGL>
#include <memory>

Scene3D::Scene3D()
{
}

Scene3D::~Scene3D()
{
}

void Scene3D::paintScene()
{
}

void Scene3D::initialize()
{
}

void Scene3D::render()
{
    glDepthMask(true);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glFrontFace(GL_CW);
//    glCullFace(GL_FRONT);
//    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);


    qreal zNear = 1, zFar = 5000.0, fov = m_fScale;

    mProjection = QMatrix4x4();
    mProjection.perspective(fov, _size.x()/_size.y(), zNear, zFar);

    if((_lastMouse.x() != _mouse.x() || _lastMouse.y() != _mouse.y()) && _mouse.x() >= 0 && _lastMouse.x() >= 0) {
        if(_isRotation) {
            _rotAngle += (_lastMouse - _mouse)*0.002;
            if(_rotAngle[1] > 1.570796f ) {
                _rotAngle[1] = 1.570796f;
            } else if(_rotAngle[1] < 0) {
                _rotAngle[1] = 0;
            }
        } else {
            QVector3D vm = QVector3D(-(_lastMouse.x() - _mouse.x()), (_lastMouse.y() - _mouse.y()), 0)*(m_fScale*0.02);

            _posCenter[0] += (vm[1]*cosf(-_rotAngle.x())*cosf(_rotAngle.y()) - vm[0]*sinf(-_rotAngle.x()));
            _posCenter[1] += (vm[1]*sinf(-_rotAngle.x())*cosf(_rotAngle.y()) + vm[0]*cosf(-_rotAngle.x()));
            _posCenter[2] += -vm[1]*sinf(_rotAngle.y())*sinf(_rotAngle.y());
        }
    }



    mView = QMatrix4x4();
    float r = -500.0;

    QVector3D cf;
    cf[0] = -sinf(_rotAngle.y())*cosf(-_rotAngle.x())*r;
    cf[1] = -sinf(_rotAngle.y())*sinf(-_rotAngle.x())*r;
    cf[2] = -cosf(_rotAngle.y())*r;

    QVector3D cu;
    cu[0] = cosf(_rotAngle.y())*cosf(-_rotAngle.x());
    cu[1] = cosf(_rotAngle.y())*sinf(-_rotAngle.x());
    cu[2] = -sinf(_rotAngle.y());

    mView.lookAt(cf + _posCenter, _posCenter, cu.normalized());

    mModel = QMatrix4x4();
    mModel.scale(1.0, 1.0, _modelScaleZ);

    if(_mouse.x() >= 0) {
        QVector3D zero(0, 0, 0);
        zero = zero.project(mView, mProjection, QRect(0, 0, _size.x(), _size.y()));
        QVector3D screenCoordinates(_mouse.x(), _mouse.y(), zero.z());
        QVector3D screenCoordinates0(_mouse.x(), _mouse.y(), 0);

        QVector3D unprj = screenCoordinates.unproject(mView, mProjection, QRect(0, 0, _size.x(), _size.y()));
//        QVector3D unprj0 = screenCoordinates0.unproject(mView, mProjection, QRect(0, 0, _size.x(), _size.y()));
//        QVector3D dir = (-unprj + unprj0);
        QVector3D ray = (-unprj + cf + _posCenter);

        float t = QVector3D::dotProduct(unprj - QVector3D(0,0,0), QVector3D(0,0,1))/QVector3D::dotProduct(ray, QVector3D(0,0,1));
        QVector3D p = unprj - t*ray;

//        _testPonts.append(p);
    }

    _lastMouse[0] = _mouse.x();
    _lastMouse[1] = _mouse.y();

    //auto pProgram = mBottomTrackDisplayedObject.shaderProgram();

    //// Настройка шейдерной программы
    //if (!pProgram || !pProgram->bind())
    //    return;


    //pProgram->release();

    //// Настройка шейдерной программы
    //mpOverlappedTrackProgram->bind();
    //QMatrix4x4 model = mModel;
    //model.translate(QVector3D(0.0f, 0.0f, 0.08f));
    //matrixLoc = mpOverlappedTrackProgram->uniformLocation("matrix");
    //mpOverlappedTrackProgram->setUniformValue(matrixLoc, mProjection*mView*mModel);
    //mpOverlappedTrackProgram->release();

    //// Настройка шейдерной программы
    //mpOverlappedGridProgram->bind();
    //matrixLoc = mpOverlappedGridProgram->uniformLocation("matrix");
    //// Приподнимаем сетку над поверхностью для корректной видимости
    //model = mModel;
    //model.translate(QVector3D(0.0f, 0.0f, 0.04f));
    //mpOverlappedGridProgram->setUniformValue(matrixLoc, mProjection*mView*model);
    //mpOverlappedGridProgram->release();

    paintScene();



    glDisable(GL_DEPTH_TEST);
//    glDisable(GL_CULL_FACE);
}
