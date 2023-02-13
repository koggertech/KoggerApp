#ifndef H3PLOT_H
#define H3PLOT_H


#include <QtQuick/QQuickFramebufferObject>

#include <QtGui/qvector3d.h>
#include <QtGui/qmatrix4x4.h>
#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qopenglfunctions.h>

#include <QObject>

#include <QTime>
#include <QVector>
#include <memory>

#include "Model/Q3DSceneModel.h"

using ModelPointer = std::shared_ptr <Q3DSceneModel>;

class Scene3D : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    Scene3D();
    ~Scene3D();

    void render();
    void initialize();

    void setModel(const ModelPointer pModel);

    void setLines(QVector<QVector3D> p);

    void scale(float sc) {
        m_fScale = sc;
    }

    void size(QVector2D sz) {
        _size = sz;
    }

    void mouse(QVector2D m) {
        _mouse = m;
    }

    QVector3D acrball(QVector2D m);

    void setCameraView(QVector3D eye, QVector3D lookat, QVector3D camup) {
        _eye = eye;
        _lookat = lookat;
        _camup = camup;
        updateView();
    }

    QVector3D getViewDir() const { return QVector3D(-_view.column(2)[0],-_view.column(2)[1],-_view.column(2)[2]); }
    QVector3D getRight() const { return QVector3D(_view.column(0)[0],_view.column(0)[1],_view.column(0)[2]); }

    void rotationFlag(bool is_rotation) { _isRotation = is_rotation; }

private slots:

    void modelStateChanged();

private:
    qreal   m_fScale;
    QVector2D _size;
    QVector2D _rotAngle;
    QVector3D _posCenter;

    QVector2D _lastMouse;
    QVector2D _mouse;

    QVector3D _eye;
    QVector3D _lookat;
    QVector3D _camup;
    QMatrix4x4 _view;

    bool _isRotation;

    void updateView() {
        _view = QMatrix4x4();
        _view.lookAt(_eye, _lookat, _camup);
    }

    void paintScene();
    void createGeometry();
    void mash(QVector<qreal> z);
    void mashAlign();
    void line(qreal x, qreal y, qreal z);

    //! Draws gps track lines
    void displayGPSTrack();
    //! Draws bottom surface
    void displayBottomSurface();
    //! Draws bottom surface mesh
    void displayBottomSurfaceMesh();

    Vector3Pointer mpTriangles;
    QVector<QVector3D> vLines;
    QVector<QVector3D> vTriangle;
    QVector<QVector3D> _gridXY;
    QVector<QVector<QVector3D>> vQuads;
    QOpenGLShaderProgram program1;
    int vertexAttr1;
    int vertexAttr2;
    int vertexAttr3;
    int vertexAttr4;
    int normalAttr1;
    int matrixUniform1;
    int projectionUniform1;
    QVector<qreal> _mashZ;
    qreal _mashStep = 0.05;

    QQuaternion q;

    //! Указатель на модель сцены
    ModelPointer mpModel;
    //! Данные трека морского дна
    Vector3 mBottomTrack;
    //! Данные треугольников поверхности
    Vector3 mTriangles;
};

class FboInSGRenderer : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Renderer)

public:
    Renderer *createRenderer() const;

    QVector<QVector3D> lines() {
        return _bottomTrack;
    }

    //! Передать указатель на модель 3D - сцены
    void setModel(const ModelPointer pModel);

    void updateBottomTrack(QVector<QVector3D> p) {
        _bottomTrack = p;

        update();
    }

public slots:
    void scaleDelta(float delta) {

        if(delta > 100.0f) {
            delta = 100.0f;
        } else if(delta < -100.0f) {
            delta = -100.0f;
        }

        float coef = 1.0f - abs(delta)/400.0;

        if(delta > 0) {
            _scale /= coef;
            if(_scale > 120.0) {
                _scale = 120.0;
            }
        } else if(delta < 0){
            _scale *= coef;
            if(_scale < 0.1) {
                _scale = 0.1;
            }
        }
        update();
    }

    void scale(float scale) {
        _scale = scale;
        update();
    }

    float scaleDelta() { return _scale; }

    QVector2D size() { return QVector2D(width(), height()); }

    void mouse(int x, int y, bool rotation_flag) {
        _lastMouseX = x;
        _lastMouseY = y;
        _rotationFlag = rotation_flag;
        update();
    }

    QVector2D mouse() { return QVector2D(_lastMouseX, _lastMouseY); }

    bool isRotation() { return _rotationFlag; }

private:
    QVector<QVector3D> _bottomTrack;
    Vector3Pointer mpTriangles;

    float _scale = 30.0;
    int _lastMouseX = -1, _lastMouseY = -1;
    bool _rotationFlag = false;

    Renderer* mpRenderer;
    ModelPointer mpModel;
};

#endif // H3PLOT_H
