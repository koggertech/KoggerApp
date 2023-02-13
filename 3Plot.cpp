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

    mBottomTrack = *mpModel->bottomTrack();
    mTriangles   = *mpModel->triangles();


}

void Scene3D::modelStateChanged()
{
    mBottomTrack = *mpModel->bottomTrack();
    mTriangles = *mpModel->triangles();
}

void Scene3D::paintScene()
{
    if (mpModel->displayedObjectType() == OBJECT_TYPE_BOTTOM_TRACK){
        displayGPSTrack();
    }

    if (mpModel->displayedObjectType() == OBJECT_TYPE_SURFACE_POLY) {
        displayBottomSurface();
    }

    if (mpModel->displayedObjectType() == OBJECT_TYPE_SURFACE_MESH) {
        displayBottomSurfaceMesh();
    }



//    program1.enableAttributeArray(normalAttr1);
    //program1.enableAttributeArray(vertexAttr1);
    //program1.setAttributeArray(vertexAttr1, vLines.constData());
//    program1.setAttributeArray(normalAttr1, nLines.constData());
    //glLineWidth(2.0);
//    glEnable(GL_PROGRAM_POINT_SIZE);
//    glPointSize(2.0f);
//    glLineWidth()
//

    //glDrawArrays(GL_LINE_STRIP, 0, vLines.size());
//    glDrawArrays(GL_POINTS, 0, vLines.size());
//    program1.disableAttributeArray(normalAttr1);
    //program1.disableAttributeArray(vertexAttr1);



//    program1.enableAttributeArray(vertexAttr2);
//    for(int i = 0; i < vQuads.length(); i++) {
//        program1.setAttributeArray(vertexAttr2, vQuads[i].constData());
//        glDrawArrays(GL_TRIANGLES, 0, vQuads[i].length());
//    }

//    program1.disableAttributeArray(vertexAttr2);


    //program1.enableAttributeArray(vertexAttr3);
    //program1.setAttributeArray(vertexAttr3, vTriangle.constData());
    //glDrawArrays(GL_TRIANGLES, 0, vTriangle.length());
    //program1.disableAttributeArray(vertexAttr3);
}

void FboInSGRenderer::setModel(const ModelPointer pModel)
{
    mpModel = pModel;
}


void Scene3D::initialize()
{
    initializeOpenGLFunctions();

    glClearColor(0.1f, 0.1f, 0.2f, 0.0f);

    const char *vsrc1 =
        "attribute highp vec4 vertex;\n"
        "attribute mediump vec3 normal;\n"
        "uniform mediump mat4 matrix;\n"
        "varying mediump vec4 color;\n"
        "void main(void)\n"

        "{\n"
        "    gl_PointSize = 3.0;\n"
        "    vec3 toLight = normalize(vec3(0.0, 0.3, 1.0));\n"
        "    float angle = max(dot(normal, toLight), 0.0);\n"
        "    vec3 col = vec3(1.0, 1.0, 1.0);\n"
        "    color = vec4(col[0]*((vertex[2]+0.5)*2.1),0.5,col[2]*(-(vertex[2]+0.5)*2.1), 1.0);\n"
        "    color = clamp(color, 0.0, 1.0);\n"
        "    gl_Position = matrix * vertex;\n"
        "}\n";

    const char *fsrc1 =
        "varying mediump vec4 color;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = color;\n"
        "}\n";

    program1.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vsrc1);
    program1.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fsrc1);
    program1.link();

    vertexAttr1 = program1.attributeLocation("vertex");
    vertexAttr2 = program1.attributeLocation("vertex");
    vertexAttr3 = program1.attributeLocation("vertex");
    vertexAttr4 = program1.attributeLocation("vertex");


//    normalAttr1 = program1.attributeLocation("normal");
    matrixUniform1 = program1.uniformLocation("matrix");
    projectionUniform1 = program1.uniformLocation("projection");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    setCameraView(QVector3D(0, 0, -40), QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    m_fScale = 1;
    createGeometry();
}

void Scene3D::displayGPSTrack() {

    program1.enableAttributeArray(vertexAttr1);
    program1.setAttributeArray(vertexAttr1, mBottomTrack.constData());
    glLineWidth(2.0);
    //glColorMask(50.0, 0.0, 200.0, 0.0);
    glDrawArrays(GL_LINE_STRIP, 0, mBottomTrack.size());

    program1.disableAttributeArray(vertexAttr1);
}

//void Scene3D::setLines(QVector<QVector3D> p) {
//    vLines.clear();
//    vLines.append(p);
//    float max_x = -100000000, min_x=100000000, max_y=-100000000, min_y=100000000, max_z=-100000000, min_z=100000000;
//
//    for(int i = 0; i < vLines.size(); i++) {
//        float x = vLines[i].x();
//        float y = vLines[i].y();
//        vLines[i].setZ(vLines[i].z()*1.0f);
//        float z = vLines[i].z();
//    }
//}

void Scene3D::displayBottomSurface() {

    program1.enableAttributeArray(vertexAttr3);
    program1.setAttributeArray(vertexAttr3, mTriangles.constData());
    //glColorMask(50.0, 0.0, 200.0, 0.0);
    glDrawArrays(GL_TRIANGLES, 0, mTriangles.size());
    program1.disableAttributeArray(vertexAttr3);
}

void Scene3D::displayBottomSurfaceMesh()
{
    program1.enableAttributeArray(vertexAttr4);
    program1.setAttributeArray(vertexAttr4, mTriangles.constData());
    //glColorMask(50.0, 0.0, 200.0, 0.0);
    glDrawArrays(GL_LINE_STRIP, 0, mTriangles.size());
    program1.disableAttributeArray(vertexAttr4);
}

//void Scene3D::setLines(QVector<QVector3D> p) {
//    vLines = p;
//    float max_x = -100000000, min_x=100000000, max_y=-100000000, min_y=100000000, max_z=-100000000, min_z=100000000;
//
//    for(int i = 0; i < vLines.size(); i++) {
//        float x = vLines[i].x();
//        float y = vLines[i].y();
//        float z = vLines[i].z();
//
//        if(max_x < x) { max_x = x; }
//        else if (min_x > x) { min_x = x; }
//
//        if(max_y < y) { max_y = y; }
//        else if (min_y > y) { min_y = y; }
//
//        if(max_z < z) { max_z = z; }
//        else if (min_z > z) { min_z = z; }
//    }
//
//    _gridXY.clear();
//    _gridXY.append(QVector3D(min_x, min_y, 0));
//    _gridXY.append(QVector3D(min_x, max_y, 0));
////    _gridXY.append(QVector3D(max_x, min_y, min_z));
////    _gridXY.append(QVector3D(max_x, max_y, min_z));
//
//    _gridXY.append(QVector3D(min_x, min_y, 0));
//    _gridXY.append(QVector3D(max_x, min_y, 0));
////    _gridXY.append(QVector3D(min_x, max_y, min_z));
////    _gridXY.append(QVector3D(max_x, max_y, min_z));
//
//    _gridXY.append(QVector3D(min_x, min_y, min_z));
//    _gridXY.append(QVector3D(min_x, min_y, max_z));
//
////    for(float x_offset = min_x; x_offset <= max_x; x_offset+=0.1) {
////        _gridXY.append(QVector3D(x_offset, min_y, min_z));
////        _gridXY.append(QVector3D(x_offset, max_y, min_z));
////    }
//
//}


QVector3D Scene3D::acrball(QVector2D m) {
    QVector3D arc;
    arc[0] = m[1]/500.f - 1.f;
    arc[1] = m[0]/1000.f - 1.f;
    arc[2] = 0;

//    arc[1] = -arc[1];

    float lsqr = arc.lengthSquared();
    if (lsqr <= 1.0f) {
        arc[2] = sqrtf(1.0f - lsqr);
    } else {
//        arc[2] = 0;
        arc.normalize();
    }
//    arc[2]= - arc[2];

    return arc;
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

    QMatrix4x4 persp;
    persp.perspective(fov, _size.x()/_size.y(), zNear, zFar);

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


//            if(_rotAngle[1] < 1.570796f) {
//                _posCenter[0] += (vm[1]*cosf(-_rotAngle.x())*cosf(_rotAngle.y()) - vm[0]*sinf(-_rotAngle.x()));
//                _posCenter[1] += (vm[1]*sinf(-_rotAngle.x())*cosf(_rotAngle.y()) + vm[0]*cosf(-_rotAngle.x()));
//                _posCenter[2] += -vm[1]*sinf(_rotAngle.y());
//            } else {
//                _posCenter[0] += - vm[0]*sinf(-_rotAngle.x());
//                _posCenter[1] += vm[0]*cosf(-_rotAngle.x());

//            }
        }
    }

    _lastMouse[0] = _mouse.x();
    _lastMouse[1] = _mouse.y();

    QMatrix4x4 view;
    float r = -500.0;

    QVector3D cf;
    cf[0] = -sinf(_rotAngle.y())*cosf(-_rotAngle.x())*r;
    cf[1] = -sinf(_rotAngle.y())*sinf(-_rotAngle.x())*r;
    cf[2] = -cosf(_rotAngle.y())*r;

    QVector3D cu;
    cu[0] = cosf(_rotAngle.y())*cosf(-_rotAngle.x());
    cu[1] = cosf(_rotAngle.y())*sinf(-_rotAngle.x());
    cu[2] = -sinf(_rotAngle.y());

    view.lookAt(cf + _posCenter, _posCenter, cu.normalized());

    QMatrix4x4 modelview;

    program1.bind();
    program1.setUniformValue(matrixUniform1, persp*view*modelview);
    paintScene();
    program1.release();

    glDisable(GL_DEPTH_TEST);
//    glDisable(GL_CULL_FACE);
}

void Scene3D::createGeometry()
{

    vQuads.clear();



//    _mashStep = 0.015;
//    QVector<qreal> z(100);

//    QVector<QVector<qreal>> rnd_m(100);

//    for(int k = 0; k < 100; k ++) {
//        rnd_m[k].resize(100);
//        for(int i = 0; i < 100; i ++) {
//            rnd_m[k][i] = (QRandomGenerator::global()->generateDouble() - 0.5)*1.5;
//        }
//    }

//    for(int k = 0; k < 100; k ++) {
//        for(int i = 0; i < 96; i ++) {
//            rnd_m[k][i] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+1] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+2] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+3] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+4] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//        }
//    }

//    for(int i = 0; i < 100; i ++) {
//        for(int k = 0; k < 96; k ++) {
//            rnd_m[k][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+1][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+2][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+3][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+4][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//        }
//    }

//    for(int k = 0; k < 100; k ++) {
//        for(int i = 0; i < 96; i ++) {
//            rnd_m[k][i] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+1] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+2] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+3] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+4] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//        }
//    }

//    for(int i = 0; i < 100; i ++) {
//        for(int k = 0; k < 96; k ++) {
//            rnd_m[k][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+1][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+2][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+3][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+4][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//        }
//    }

//    for(int k = 0; k < 100; k ++) {
//        for(int i = 0; i < 96; i ++) {
//            rnd_m[k][i] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+1] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+2] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+3] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+4] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//        }
//    }

//    for(int i = 0; i < 100; i ++) {
//        for(int k = 0; k < 96; k ++) {
//            rnd_m[k][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+1][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+2][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+3][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+4][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//        }
//    }

//    for(int k = 0; k < 100; k ++) {
//        for(int i = 0; i < 96; i ++) {
//            rnd_m[k][i] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+1] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+2] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+3] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//            rnd_m[k][i+4] = (rnd_m[k][i] + rnd_m[k][i+1] + rnd_m[k][i+2] + rnd_m[k][i+3] + rnd_m[k][i+4])*0.2;
//        }
//    }

//    for(int i = 0; i < 100; i ++) {
//        for(int k = 0; k < 96; k ++) {
//            rnd_m[k][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+1][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+2][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+3][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//            rnd_m[k+4][i] = (rnd_m[k][i] + rnd_m[k+1][i] + rnd_m[k+2][i] + rnd_m[k+3][i] + rnd_m[k+4][i])*0.2;
//        }
//    }



//    for(int i = 0; i < 100; i ++) {

//        mash(rnd_m[i]);

//    }

//    mashAlign();

//    float step = 0.05f;
//    for(float i = -0.3; i < 0.3; i+=step) {
//        for(float k = -0.3; k < 0.3; k+=step) {

//            mash(QVector3D(k, i, (i+k)*2));
//            mash(QVector3D(k, i+step, (i+k)*2));
//        }
//    }

//    const qreal Pi = M_PI;
//    const int NumSectors = 500;

//    for (int i = 0; i < NumSectors; ++i) {
//        qreal angle1 = (i * 2 * Pi) / (NumSectors/10);
//        qreal x = (qreal)(i - NumSectors/2)/(NumSectors*2) * sin(angle1);
//        qreal y = (qreal)(NumSectors/2 - i)/(NumSectors*2) * cos(angle1);
//        qreal z = (qreal)(i - NumSectors/2)/(NumSectors*2);
//        line(x, y, z);
//    }


//    quad(x1, y1, x2, y2, y2, x2, y1, x1);

//    triag(x1, y1, x2, y2, x3, y3);
//    quad(x3, y3, x4, y4, y4, x4, y3, x3);

//    extrude(x1, y1, x2, y2);
//    extrude(x2, y2, y2, x2);
//    extrude(y2, x2, y1, x1);
//    extrude(y1, x1, x1, y1);
//    extrude(x3, y3, x4, y4);
//    extrude(x4, y4, y4, x4);
//    extrude(y4, x4, y3, x3);

//    const qreal Pi = M_PI;
//    const int NumSectors = 100;

//    for (int i = 0; i < NumSectors; ++i) {
//        qreal angle1 = (i * 2 * Pi) / NumSectors;
//        qreal x5 = 0.10 * sin(angle1);
//        qreal y5 = 0.10 * cos(angle1);
//        qreal x6 = 0;
//        qreal y6 = 0;

//        qreal angle2 = ((i + 1) * 2 * Pi) / NumSectors;
//        qreal x7 = 0;
//        qreal y7 = 0;
//        qreal x8 = 0.10 * sin(angle2);
//        qreal y8 = 0.10 * cos(angle2);

//        quad(x5, y5, x6, y6, x7, y7, x8, y8);

//        extrude(x6, y6, x7, y7);
//        extrude(x8, y8, x5, y5);
//    }

/*    for (int i = 0;i < vLines.size();i++)
        vLines[i] *= 2.0f*/;
}

void Scene3D::mash(QVector<qreal> z) {
    if(_mashZ.size() != 0) {
        vQuads.append(QVector<QVector3D>());

        const int mash_width_prev = _mashZ.size() - 1;
        const int mash_width_next = z.size() - 1;
        const int mash_width_min = mash_width_prev < mash_width_next ? mash_width_prev : mash_width_next;
        const int mash_height = vQuads.size() - 1;

        for(int i = 0; i < mash_width_min; i++) {
            float x = (float)(i) *_mashStep;
            float xn = (float)(i+ 1)*_mashStep;
            float y = (float)mash_height*_mashStep;
            float yn = (float)(mash_height+1)*_mashStep;

            qreal xm = (x + xn)*0.5;
            qreal ym = (y + yn)*0.5;
            qreal zm = (_mashZ[i] + _mashZ[i+1] + z[i] + z[i+1])*0.25;

            vQuads.last() << QVector3D(x, y, _mashZ[i]);
            vQuads.last() << QVector3D(xn, y, _mashZ[i+1]);
            vQuads.last() << QVector3D(xm, ym, zm);

            vQuads.last() << QVector3D(x, y, _mashZ[i]);
            vQuads.last() << QVector3D(x, yn, z[i]);
            vQuads.last() << QVector3D(xm, ym, zm);

            vQuads.last() << QVector3D(x, yn, z[i]);
            vQuads.last() << QVector3D(xn, yn, z[i+1]);
            vQuads.last() << QVector3D(xm, ym, zm);


            vQuads.last() << QVector3D(xn, yn, z[i+1]);
            vQuads.last() << QVector3D(xn, y, _mashZ[i+1]);
            vQuads.last() << QVector3D(xm, ym, zm);
        }
    }

    _mashZ = z;
}

void Scene3D::mashAlign() {

    const int mash_height = vQuads.size();

    float mash_x_offset = 0.0;

    for(int i = 0; i < mash_height; i++) {
        mash_x_offset += vQuads[i].size();
    }
    mash_x_offset /= (float)(mash_height*12);
    mash_x_offset *= _mashStep*0.5;

    const float  mash_y_offset = (float)mash_height*_mashStep*0.5;


    for(int i = 0; i < mash_height; i++) {
        const int row_size = vQuads[i].size();
        for(int k = 0; k < row_size; k++) {
            vQuads[i][k][0] -= mash_x_offset;
            vQuads[i][k][1] -= mash_y_offset;
        }
    }
}

void Scene3D::line(qreal x, qreal y, qreal z) {
    vLines << QVector3D(x, y, z);
//    QVector3D n = QVector3D::normal
//        (QVector3D(x, y, 0.0f), QVector3D(0.0f, 0.0f, 1.f));
//    nLines << n;
}
