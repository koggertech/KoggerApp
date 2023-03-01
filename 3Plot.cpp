#include "3Plot.h"

#include <QtGui/QOpenGLFramebufferObject>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>
#include <QRandomGenerator>

#include <iostream>


static const char* pOverlappingTrackVertexShader = "in vec3 position;\n"
                                                   "uniform mat4 matrix;\n"
                                                   "out vec4 color;\n"
                                                   "void main()\n"
                                                   "{\n"
                                                   " color = vec4(1.0f, 0.0f, 1.0f, 1.0f);\n"
                                                   " gl_Position = matrix * vec4(position, 1.0);\n"
                                                   "}\n";

static const char* pOverlapingGridVertexShader = "in vec3 position;\n"
                                                 "uniform mat4 matrix;\n"
                                                 "out vec4 color;\n"
                                                 "void main()\n"
                                                 "{\n"
                                                 " color = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n"
                                                 " gl_Position = matrix * vec4(position, 1.0);\n"
                                                 "}\n";

// TODO - вынести шейдер в файл и реализовать загрузку
const char *pHeightMappedVertexShader = "in vec3 position;\n"
                                        "uniform mat4 matrix;\n"
                                        "out vec4 color;\n"
                                        "uniform float max_z;\n"
                                        "uniform float min_z;\n"
                                        "vec3 getColor(float v,float vmin, float vmax)\n"
                                        "{\n"
                                        "   vec3 c = vec3(1.0f, 1.0f, 1.0f);\n"
                                        "   float dv;\n"
                                        "\n"
                                        "   if (v < vmin)\n"
                                        "      v = vmin;\n"
                                        "   if (v > vmax)\n"
                                        "      v = vmax;\n"
                                        "   dv = vmax - vmin;\n"
                                        "\n"
                                        "   if (v < (vmin + 0.25f * dv)) {\n"
                                        "      c.r = 0.0f;\n"
                                        "      c.g = 4.0f * (v - vmin) / dv;\n"
                                        "   } else if (v < (vmin + 0.5f * dv)) {\n"
                                        "      c.r = 0.0f;\n"
                                        "      c.b = 1.0f + 4.0f * (vmin + 0.25f * dv - v) / dv;\n"
                                        "   } else if (v < (vmin + 0.75f * dv)) {\n"
                                        "      c.r = 4.0f * (v - vmin - 0.5f * dv) / dv;\n"
                                        "      c.b = 0.0f;\n"
                                        "   } else {\n"
                                        "      c.g = 1.0f + 4.0f * (vmin + 0.75f * dv - v) / dv;\n"
                                        "      c.b = 0.0f;\n"
                                        "   }\n"
                                        "\n"
                                        "   return c;\n"
                                        "}\n"
                                        "void main()\n"
                                        "{\n"
                                        "   float norm_z = (position.z - min_z) / (abs(max_z - min_z));\n"
                                        "   color = vec4(getColor(norm_z, 0.0f, 1.0f), 1.0f);\n"
                                        "	gl_Position = matrix * vec4(position, 1.0);\n"
                                        "}\n";

const char *pFragmentShader = "in vec4 color;\n"
                              "void main()\n"
                              "{\n"
                              "   gl_FragColor = color;\n"
                              "}\n";

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

    void setModel(const ModelPointer pModel){
        scene.setModel(pModel);
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
    }

    Scene3D scene;
};

QQuickFramebufferObject::Renderer *FboInSGRenderer::createRenderer() const
{
    auto renderer = new FboRenderer();

    renderer->setModel(mpModel);

    return renderer;
}

#include <QPainter>
#include <QPaintEngine>
#include <qmath.h>
#include <QtOpenGL/QtOpenGL>
Scene3D::Scene3D()
: mpHeightMappedProgram(new QOpenGLShaderProgram)
, mpOverlappedTrackProgram(new QOpenGLShaderProgram)
, mpOverlappedGridProgram(new QOpenGLShaderProgram)
{
}

Scene3D::~Scene3D()
{
    if (mpHeightMappedProgram)
        delete mpHeightMappedProgram;

    if (mpOverlappedTrackProgram)
        delete mpOverlappedTrackProgram;

    if (mpOverlappedGridProgram)
        delete mpOverlappedGridProgram;
}

void Scene3D::setModel(const ModelPointer pModel)
{
    mpModel = pModel;

    connect(mpModel.get(), SIGNAL(stateChanged()), this, SLOT(modelStateChanged()));

    if (!pModel) return;

    mBottomTrack = *mpModel->bottomTrack();
    mTriangles   = *mpModel->triangles();
}

void Scene3D::modelStateChanged()
{
    mBottomTrack = *mpModel->bottomTrack();
    mTriangles = *mpModel->triangles();
    mTriangleGrid = *mpModel->triangleGrid();
    mQuads = *mpModel->quads();
    mGrid = *mpModel->grid();

    mMaxZ = mpModel->objectMaximumZ();
    mMinZ = mpModel->objectMinimumZ();

    qDebug() << "Maximum Z: " << mMaxZ << "\n";
    qDebug() << "Minimum Z: " << mMinZ << "\n";
}

void Scene3D::paintScene()
{
    if (mpModel->bottomTrackVisible()){
        displayGPSTrack();
    }

    if (mpModel->surfaceVisible()) {

        displayBottomSurface();
    }

    if (mpModel->surfaceGridVisible()){
        displayBottomSurfaceGrid();
    }
}

void Scene3D::displayGPSTrack() {

    QOpenGLShaderProgram* pProgram = mpHeightMappedProgram;

    if (mpModel->surfaceGridVisible() ||
        mpModel->surfaceVisible()){
        pProgram = mpOverlappedTrackProgram;
    }

    pProgram->bind();
    int posLoc = pProgram->attributeLocation("position");

    pProgram->enableAttributeArray(posLoc);
    pProgram->setAttributeArray(posLoc, mBottomTrack.constData());
    glLineWidth(4.0);
    glDrawArrays(GL_LINE_STRIP, 0, mBottomTrack.size());
    pProgram->disableAttributeArray(posLoc);
    pProgram->release();
    glLineWidth(1.0);
}

void Scene3D::displayBottomSurface() {

    QOpenGLShaderProgram* pProgram = mpHeightMappedProgram;

    pProgram->bind();

    int posLoc = pProgram->attributeLocation("position");

    pProgram->enableAttributeArray(posLoc);

    if (mpModel->displayedStage() == DISPLAYED_STAGE_TIN){
        pProgram->setAttributeArray(posLoc, mTriangles.constData());
        glDrawArrays(GL_TRIANGLES, 0, mTriangles.size());
    }else{
        pProgram->setAttributeArray(posLoc, mQuads.constData());
        glDrawArrays(GL_QUADS, 0, mQuads.size());
    }

    pProgram->disableAttributeArray(posLoc);
    pProgram->release();
}

void Scene3D::displayBottomSurfaceGrid()
{
    QOpenGLShaderProgram* pProgram = mpHeightMappedProgram;

    if (mpModel->surfaceVisible()){
        pProgram = mpOverlappedGridProgram;
    }

    pProgram->bind();

    int posLoc = pProgram->attributeLocation("position");

    pProgram->enableAttributeArray(posLoc);

    if (mpModel->displayedStage() == DISPLAYED_STAGE_TIN){
        pProgram->setAttributeArray(posLoc, mTriangleGrid.constData());
        glDrawArrays(GL_LINES, 0, mTriangleGrid.size());
    }else{
        pProgram->setAttributeArray(posLoc, mGrid.constData());
        glDrawArrays(GL_LINES, 0, mGrid.size());
    }

    pProgram->disableAttributeArray(posLoc);
    pProgram->release();
}

void FboInSGRenderer::setModel(const ModelPointer pModel)
{
    mpModel = pModel;
}


void Scene3D::initialize()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);;
    glClearColor(0.1f, 0.1f, 0.2f, 0.0f);

    program1.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, pHeightMappedVertexShader);
    program1.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, pFragmentShader);
    program1.link();

    // Загрузка шейдеров в программу для случая, когда отсутствует наложение объектов на сцене
    mpHeightMappedProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, pHeightMappedVertexShader);
    mpHeightMappedProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, pFragmentShader);
    mpHeightMappedProgram->link();

    // Загрузка шейдеров в программу для случая, когда присутствует наложение трека на сцене
    mpOverlappedTrackProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, pOverlappingTrackVertexShader);
    mpOverlappedTrackProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, pFragmentShader);
    mpOverlappedTrackProgram->link();

    // Загрузка шейдеров в программу для случая, когда присутствует наложение сетки на сцене
    mpOverlappedGridProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, pOverlapingGridVertexShader);
    mpOverlappedGridProgram->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, pFragmentShader);
    mpOverlappedGridProgram->link();

    m_fScale = 1;
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

    // Настройка шейдерной программы
    mpHeightMappedProgram->bind();

    int maxZLoc = mpHeightMappedProgram->uniformLocation("max_z");
    int minZLoc = mpHeightMappedProgram->uniformLocation("min_z");
    int matrixLoc = mpHeightMappedProgram->uniformLocation("matrix");

    program1.setUniformValue(maxZLoc, mMaxZ);
    program1.setUniformValue(minZLoc, mMinZ);

    mpHeightMappedProgram->setUniformValue(matrixLoc, mProjection*mView*mModel);
    mpHeightMappedProgram->release();

    // Настройка шейдерной программы
    mpOverlappedTrackProgram->bind();
    QMatrix4x4 model = mModel;
    model.translate(QVector3D(0.0f, 0.0f, 0.08f));
    matrixLoc = mpOverlappedTrackProgram->uniformLocation("matrix");
    mpOverlappedTrackProgram->setUniformValue(matrixLoc, mProjection*mView*mModel);
    mpOverlappedTrackProgram->release();

    // Настройка шейдерной программы
    mpOverlappedGridProgram->bind();
    matrixLoc = mpOverlappedGridProgram->uniformLocation("matrix");
    // Приподнимаем сетку над поверхностью для корректной видимости
    model = mModel;
    model.translate(QVector3D(0.0f, 0.0f, 0.04f));
    mpOverlappedGridProgram->setUniformValue(matrixLoc, mProjection*mView*model);
    mpOverlappedGridProgram->release();

    paintScene();

    glDisable(GL_DEPTH_TEST);
//    glDisable(GL_CULL_FACE);
}
