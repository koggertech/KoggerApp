#include "3Plot.h"

#include <QtGui/QOpenGLFramebufferObject>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>
#include <QRandomGenerator>

#include <iostream>

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
Scene3D::Scene3D() {
}

Scene3D::~Scene3D() {
}

void Scene3D::setModel(const ModelPointer pModel)
{
    mpModel = pModel;

    connect(mpModel.get(), SIGNAL(stateChanged()), this, SLOT(modelStateChanged()));

    if (!pModel) return;

    mBottomTrack = *mpModel->bottomTrack();
    mTriangles   = *mpModel->triangles();

    // Определяем минимальную и максимальную высоту (координата Z)
    //findHeightDimensions();
}



void Scene3D::modelStateChanged()
{
    mBottomTrack = *mpModel->bottomTrack();
    mTriangles = *mpModel->triangles();
    mQuads = *mpModel->quads();
    mGrid = *mpModel->grid();

    mMaxZ = mpModel->objectMaximumZ();
    mMinZ = mpModel->objectMinimumZ();
}

void Scene3D::paintScene()
{
    int maxZLoc = program1.uniformLocation("max_z");
    program1.setUniformValue(maxZLoc, mMaxZ);

    int minZLoc = program1.uniformLocation("min_z");
    program1.setUniformValue(minZLoc, mMinZ);

    int isGridLoc = program1.uniformLocation("isGrid");
    program1.setUniformValue(isGridLoc, false);

    if (mpModel->displayedObjectType() == OBJECT_BOTTOM_TRACK){
        displayGPSTrack();
    }

    if (mpModel->displayedObjectType() == OBJECT_SURFACE_POLY_GRID) {

        displayBottomSurface();

        // Устанавливаем признак отрисовки сетки
        program1.setUniformValue(isGridLoc, true);
        displayBottomSurfaceGrid();
    }

    if (mpModel->displayedObjectType() == OBJECT_SURFACE_POLY) {
        displayBottomSurface();
    }

    if (mpModel->displayedObjectType() == OBJECT_SURFACE_GRID) {
        displayBottomSurfaceGrid();
    }

}

void Scene3D::displayGPSTrack() {

    program1.enableAttributeArray(vertexAttr1);
    program1.setAttributeArray(vertexAttr1, mBottomTrack.constData());
    glLineWidth(2.0);

    glDrawArrays(GL_LINE_STRIP, 0, mBottomTrack.size());

    program1.disableAttributeArray(vertexAttr1);
}

void Scene3D::displayBottomSurface() {

    program1.enableAttributeArray(vertexAttr3);
    program1.setAttributeArray(vertexAttr3, mQuads.constData());
    glPolygonOffset(-1,1);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glDrawArrays(GL_QUADS, 0, mQuads.size());
    program1.disableAttributeArray(vertexAttr3);
}

void Scene3D::displayBottomSurfaceGrid()
{
    // Приподнимаем сетку над поверхностью для нормальной видимости
    QMatrix4x4 model = mModel;
    model.translate(QVector3D(0.0f, 0.0f, 0.04f));

    program1.setUniformValue(matrixUniform1, mProjection*mView*model);
    program1.enableAttributeArray(vertexAttr4);
    program1.setAttributeArray(vertexAttr4, mGrid.constData());
    glDrawArrays(GL_LINES, 0, mGrid.size());
    program1.disableAttributeArray(vertexAttr4);

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

    // TODO - вынести шейдер в файл и реализовать загрузку
    const char *vsrc1 = "in vec3 position;\n"
                        "uniform mat4 matrix;\n"
                        "uniform bool isGrid;\n"
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
                        "   if (!isGrid)\n"
                        "       color = vec4(getColor(norm_z, 0.0f, 1.0f), 1.0f);\n"
                        "   else\n"
                        "       color = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n"
                        "	gl_Position = matrix * vec4(position, 1.0);\n"
                        "}\n";

    const char *fsrc1 = "in vec4 color;\n"
                        "void main()\n"
                        "{\n"
                        "   gl_FragColor = color;\n"
                        "}\n";

    program1.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vsrc1);
    program1.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fsrc1);
    program1.link();

    vertexAttr1 = program1.attributeLocation("position");
    vertexAttr2 = program1.attributeLocation("position");
    vertexAttr3 = program1.attributeLocation("position");
    vertexAttr4 = program1.attributeLocation("position");

    matrixUniform1 = program1.uniformLocation("matrix");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

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

    program1.bind();
    program1.setUniformValue(matrixUniform1, mProjection*mView*mModel);
    paintScene();
    program1.release();

    glDisable(GL_DEPTH_TEST);
//    glDisable(GL_CULL_FACE);
}
