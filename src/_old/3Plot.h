#ifndef H3PLOT_H
#define H3PLOT_H


#include <QtQuick/QQuickFramebufferObject>

#include <QtGui/qvector3d.h>
#include <QtGui/qmatrix4x4.h>
//#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qopenglfunctions.h>

#include <QObject>
#include <qdebug.h>
#include <QTime>
#include <QVector>

#include <memory>

class Scene3D : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    Scene3D();
    ~Scene3D();

    void render();
    void initialize();

    void scale(float sc) {
        m_fScale = sc;
    }

    void modelScale(float scale) {
        _modelScaleZ = scale;
    }

    void size(QVector2D sz) {
        _size = sz;
    }

    void setRightMouseButtonPressed(bool pressed){
        mIsRightMouseButtonPressed = pressed;
    }

    void mouse(QVector2D m) {
        _mouse = m;
    }

    QMatrix4x4 modelMatrix() const {
        return mModel;
    }

    QMatrix4x4 projectionMatrix() const {
        return mProjection;
    }

    QMatrix4x4 viewMatrix() const {
        return mView;
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

private:
    float _modelScaleZ = 1.0f;
    qreal   m_fScale = 1;
    QVector2D _size;
    QVector2D _rotAngle;
    QVector3D _posCenter;

    QVector2D _lastMouse;
    QVector2D _mouse;

    QVector3D _eye;
    QVector3D _lookat;
    QVector3D _camup;
    QMatrix4x4 _view;

    QVector<QVector3D> _testPonts;

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

    QVector<QVector3D> vLines;
    QVector<QVector3D> vTriangle;
    QVector<QVector3D> _gridXY;
    QVector<QVector<QVector3D>> vQuads;

    int vertexAttr1;
    int normalAttr1;
    int matrixUniform1;
    int projectionUniform1;
    QVector<qreal> _mashZ;
    qreal _mashStep = 0.05;

    QQuaternion q;

    QMatrix4x4 mModel;
    QMatrix4x4 mView;
    QMatrix4x4 mProjection;

    float mMaxZ = 0.0f;
    float mMinZ = 0.0f;

    void displayTest();
    void displayRays();
    void displayPoint(const QVector3D& p);

    bool mIsRightMouseButtonPressed = false;

    QVector2D mMousePos;
    QSize mSceneSize;
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

    //void setSceneObjectsListModel(std::shared_ptr <SceneObjectsListModel> sceneObjectsListModel);

    void updateBottomTrack(QVector<QVector3D> p) {
        _bottomTrack = p;

       // update();
    }

    bool isRightMouseButtonPressed() const {
        return mIsRightMouseButtonPressed;
    };

    bool isLeftMouseButtonPressed() const {
        return mIsLeftMouseButtonPressed;
    }

    QVector2D mousePos() const {
        return mMousePos;
    }

public slots:
    void scaleDelta(float delta) {

        qDebug() << "delta: " << delta;

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
       // update();
    }

    void scale(float scale) {
        _scale = scale;
       // update();
    }

    float scaleDelta() { return _scale; }

    void mouse(int x, int y, bool rotation_flag) {
        _lastMouseX = x;
        _lastMouseY = y;
        _rotationFlag = rotation_flag;
       // update();
    }

    void mousePressed(int modifier){
        switch(modifier){
        case Qt::LeftButton:
            mIsLeftMouseButtonPressed = true;
            break;
        case Qt::RightButton:
            mIsRightMouseButtonPressed = true;
            break;
        }

      //  update();
    };

    void mouseReleased(int modifier){
        switch(modifier){
        case Qt::LeftButton:
            mIsLeftMouseButtonPressed = false;
            break;
        case Qt::RightButton:
            mIsRightMouseButtonPressed = false;
            break;
        }

     //   update();
    };

    void mouseMoved(int x, int y){
        mMousePos = QVector2D(x,y);
    //    update();
    };

    QVector2D mouse() { return QVector2D(_lastMouseX, _lastMouseY); }

    bool isRotation() { return _rotationFlag; }

private:
    QVector<QVector3D> _bottomTrack;

    float _scale = 30.0;
    float _modelScaleZ = 1.0f;
    int _lastMouseX = -1, _lastMouseY = -1;
    bool _rotationFlag = false;

    Renderer* mpRenderer;
    //std::shared_ptr <SceneObjectsListModel> mpSceneObjectsListModel;

    bool mIsRightMouseButtonPressed = false;
    bool mIsLeftMouseButtonPressed = false;

    QVector2D mMousePos = {0.0f, 0.0f};
};

#endif // H3PLOT_H
