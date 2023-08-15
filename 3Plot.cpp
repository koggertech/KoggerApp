#include "3Plot.h"
#include <pointset.h>
#include <bottomtrack.h>
#include <surface.h>

#include <QtGui/QOpenGLFramebufferObject>

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

    void setController(std::shared_ptr <SceneController> controller){
        scene.setController(controller);
    }

    void setSceneObjectsListModel(std::shared_ptr <SceneObjectsListModel> sceneObjectsListModel){
        scene.setSceneObjectsListModel(sceneObjectsListModel);
    }

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
        scene.size(fbitem->size());
        scene.mouse(fbitem->mouse());
        scene.rotationFlag(fbitem->isRotation());
        scene.setRightMouseButtonPressed(fbitem->isRightMouseButtonPressed());
        scene.setMousePos(fbitem->mousePos());
        scene.setSize(QSize(fbitem->width(), fbitem->height()));
    }

    Scene3D scene;
};

QQuickFramebufferObject::Renderer *FboInSGRenderer::createRenderer() const
{
    auto renderer = new FboRenderer();

    renderer->setController(mpSceneController);
    renderer->setSceneObjectsListModel(mpSceneObjectsListModel);

    return renderer;
}

#include <QPainter>
#include <QPaintEngine>
#include <qmath.h>
#include <QtOpenGL/QtOpenGL>
#include <memory>

Scene3D::Scene3D()
:mpStaticColorShaderProgram(new QOpenGLShaderProgram)
,mpHeightColorShaderProgram(new QOpenGLShaderProgram)
{
    mShaderProgramMap["height"] = mpHeightColorShaderProgram.get();
    mShaderProgramMap["static"] = mpStaticColorShaderProgram.get();
}

Scene3D::~Scene3D()
{
}

void Scene3D::setController(std::shared_ptr <SceneController> controller)
{
    mpController = controller;
}

void Scene3D::setSceneObjectsListModel(std::shared_ptr <SceneObjectsListModel> sceneObjectsListModel)
{
    mpSceneObjectsListModel = sceneObjectsListModel;
}

void Scene3D::paintScene()
{
    displayBottomTrackObjects();

    displaySurfaceObjects();

    //displaySurfaceContours();

    //displaySurfaceGrids();

    //displayObjectBounds();

    //displayPointSetObjects();

    //displayPolygonObjects();
}

void Scene3D::displayBottomTrackObjects()
{
    if(!mpSceneObjectsListModel)
        return;

    auto objects = mpSceneObjectsListModel->data();

    for(const auto& object : objects)
    {
        if(object->type() == SceneObject::SceneObjectType::BottomTrack)
            object->draw(this, mModel*mProjection*mView, mShaderProgramMap);
    }
}

void Scene3D::displaySurfaceObjects()
{
    if(!mpSceneObjectsListModel)
        return;

    auto objects = mpSceneObjectsListModel->data();

    for(const auto& object : objects)
    {
        if(object->type() == SceneObject::SceneObjectType::Surface)
            object->draw(this, mModel*mProjection*mView, mShaderProgramMap);
    }
}

void Scene3D::displaySurfaceContours()
{
    if(!mpSceneObjectsListModel)
        return;

    auto objects = mpSceneObjectsListModel->dataByType(SceneObject::SceneObjectType::Surface);

    for(const auto& object : objects)
    {
        auto surface = dynamic_cast <Surface*>(object.get());

        if (!surface)
            continue;

        if (!surface->contour()->isVisible())
            continue;

        auto pProgram = mpStaticColorShaderProgram.get();

        if (!pProgram->bind()){
            qCritical() << "Error binding shader program.";
            return;
        }

        int posLoc    = pProgram->attributeLocation("position");
        int matrixLoc = pProgram->uniformLocation("matrix");
        int colorLoc  = pProgram->uniformLocation("color");

        pProgram->setUniformValue(colorLoc, surface->contour()->color4d());
        pProgram->setUniformValue(matrixLoc, mProjection*mView*mModel);
        pProgram->enableAttributeArray(posLoc);
        pProgram->setAttributeArray(posLoc, surface->contour()->cdata().constData());
        glLineWidth(4.0);
        glDrawArrays(surface->contour()->primitiveType(), 0, surface->contour()->cdata().size());
        pProgram->disableAttributeArray(posLoc);
        pProgram->release();
        glLineWidth(1.0);

        pProgram->release();
    }
}

void Scene3D::displaySurfaceGrids()
{
    if(!mpSceneObjectsListModel)
        return;

    auto objects = mpSceneObjectsListModel->dataByType(SceneObject::SceneObjectType::Surface);

    for(const auto& object : objects)
    {
        auto surface = dynamic_cast <Surface*>(object.get());

        if (!surface)
            continue;

        if (!surface->grid()->isVisible())
            continue;

        auto pProgram = mpStaticColorShaderProgram.get();

        if (!pProgram->bind()){
            qCritical() << "Error binding shader program.";
            return;
        }

        int posLoc    = pProgram->attributeLocation("position");
        int matrixLoc = pProgram->uniformLocation("matrix");
        int colorLoc  = pProgram->uniformLocation("color");

        pProgram->setUniformValue(colorLoc, surface->grid()->color4d());
        pProgram->setUniformValue(matrixLoc, mProjection*mView*mModel);
        pProgram->enableAttributeArray(posLoc);
        pProgram->setAttributeArray(posLoc, surface->grid()->cdata().constData());
        glLineWidth(4.0);
        glDrawArrays(surface->grid()->primitiveType(), 0, surface->grid()->cdata().size());
        pProgram->disableAttributeArray(posLoc);
        pProgram->release();
        glLineWidth(1.0);

        pProgram->release();
    }
}

void Scene3D::displayObjectBounds()
{
    //if(!mpSceneObjectsListModel)
    //    return;

    //auto objects = mpSceneObjectsListModel->data();

    //for(const auto& object : objects)
    //{
    //    if (!object)
    //        continue;

    //    auto pProgram = mpStaticColorShaderProgram.get();

    //    if (!pProgram->bind()){
    //        qCritical() << "Error binding shader program.";
    //        return;
    //    }

    //    auto bounds = std::unique_ptr <VertexObject>(new VertexObject(GL_LINES));


    //    int posLoc    = pProgram->attributeLocation("position");
    //    int matrixLoc = pProgram->uniformLocation("matrix");
    //    int colorLoc  = pProgram->uniformLocation("color");

    //    pProgram->setUniformValue(colorLoc, QVector4D(1.0f, 1.0f, 1.0f, 0.0f));
    //    pProgram->setUniformValue(matrixLoc, mProjection*mView*mModel);
    //    pProgram->enableAttributeArray(posLoc);
    //    pProgram->setAttributeArray(posLoc, bounds->cdata().constData());
    //    glLineWidth(4.0);
    //    glDrawArrays(bounds->primitiveType(), 0, bounds->cdata().size());
    //    pProgram->disableAttributeArray(posLoc);
    //    pProgram->release();
    //    glLineWidth(1.0);

    //    pProgram->release();
    //}
}

void Scene3D::displayPointSetObjects()
{
    if(!mpSceneObjectsListModel)
        return;

    auto objects = mpSceneObjectsListModel->dataByType(SceneObject::SceneObjectType::PointSet);

    for(const auto& object : objects)
    {
        auto pointSet = dynamic_cast <PointSet*>(object.get());

        if (!pointSet)
            continue;

        if (!pointSet->isVisible())
            continue;

        auto pProgram = mpStaticColorShaderProgram.get();

        if (!pProgram->bind()){
            qCritical() << "Error binding shader program.";
            return;
        }

        int posLoc    = pProgram->attributeLocation("position");
        int matrixLoc = pProgram->uniformLocation("matrix");
        int colorLoc  = pProgram->uniformLocation("color");


        pProgram->setUniformValue(colorLoc, pointSet->color4d());
        pProgram->setUniformValue(matrixLoc, mProjection*mView*mModel);
        pProgram->enableAttributeArray(posLoc);
        pProgram->setAttributeArray(posLoc, pointSet->cdata().constData());
        glLineWidth(4.0);
        glPointSize(pointSet->lineWidth());
        glDrawArrays(pointSet->primitiveType(), 0, pointSet->cdata().size());
        pProgram->disableAttributeArray(posLoc);
        pProgram->release();
        glLineWidth(1.0);
        glPointSize(1.0);

        pProgram->release();
    }
}

void Scene3D::displayPolygonObjects()
{
    if(!mpSceneObjectsListModel)
        return;

    auto objects = mpSceneObjectsListModel->dataByType(SceneObject::SceneObjectType::Polygon);

    for(const auto& object : objects)
    {
        auto polygon = dynamic_cast <PolygonObject*>(object.get());

        if (!polygon)
            continue;

        if (!polygon->isVisible())
            continue;

        auto pProgram = mpStaticColorShaderProgram.get();

        if (!pProgram->bind()){
            qCritical() << "Error binding shader program.";
            return;
        }

        int posLoc    = pProgram->attributeLocation("position");
        int matrixLoc = pProgram->uniformLocation("matrix");
        int colorLoc  = pProgram->uniformLocation("color");

        pProgram->setUniformValue(colorLoc, polygon->color4d());
        pProgram->setUniformValue(matrixLoc, mProjection*mView*mModel);
        pProgram->enableAttributeArray(posLoc);
        pProgram->setAttributeArray(posLoc, polygon->cdata().constData());
        glLineWidth(4.0);
        glPointSize(polygon->lineWidth());
        glDrawArrays(polygon->primitiveType(), 0, polygon->cdata().size());
        pProgram->disableAttributeArray(posLoc);
        pProgram->release();
        glLineWidth(polygon->lineWidth());
        glPointSize(1.0);

        pProgram->release();
    }
}

void FboInSGRenderer::setController(std::shared_ptr<SceneController> controller)
{
    mpSceneController = controller;
}

void FboInSGRenderer::setSceneObjectsListModel(std::shared_ptr<SceneObjectsListModel> sceneObjectsListModel)
{
    mpSceneObjectsListModel = sceneObjectsListModel;
}

void Scene3D::initialize()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);;
    glClearColor(0.1f, 0.1f, 0.2f, 0.0f);

    // ---------
    bool success = mpStaticColorShaderProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base.vsh");

    if (!success) qCritical() << "Error adding vertex shader from source file.";

    success = mpStaticColorShaderProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/staticcolor.fsh");

    if (!success) qCritical() << "Error adding fragment shader from source file.";

    success = mpStaticColorShaderProgram->link();

    if (!success) qCritical() << "Error linking shaders in shader program.";

    // --------

    success = mpHeightColorShaderProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base.vsh");

    if (!success) qCritical() << "Error adding vertex shader from source file.";

    success = mpHeightColorShaderProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/heightcolor.fsh");

    if (!success) qCritical() << "Error adding fragment shader from source file.";

    success = mpHeightColorShaderProgram->link();

    if (!success) qCritical() << "Error linking shaders in shader program.";
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

    _lastMouse[0] = _mouse.x();
    _lastMouse[1] = _mouse.y();

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
