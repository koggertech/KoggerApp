#ifndef GRAPHICSSCENE3D_H
#define GRAPHICSSCENE3D_H

#include <memory>

#include <QOpenGLFunctions>

#include <scenegraphicsobject.h>

class GraphicsScene3dView;
class GraphicsScene3d : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GraphicsScene3d();
    virtual ~GraphicsScene3d();

    void addGraphicsObject(std::shared_ptr <SceneGraphicsObject> object);
    void removeGraphicsObject(std::shared_ptr <SceneGraphicsObject> object);
    void setView(GraphicsScene3dView* view);
    void draw();
    QRectF rect() const;
    qreal width() const;
    qreal height() const;
    GraphicsScene3dView* view() const;
    bool isInitialized() const;
    qreal fov() const;

public slots:
    void setCameraZoom(qreal angle);
    void rotateCamera(qreal pitchOffset, qreal yawOffset);
    void setWidth(qreal width);
    void setRect(const QRectF& rect);
    void setHeight(qreal height);

private:
    void initialize();
    void drawObjects();

protected:
    QMatrix4x4 m_model;
    QMatrix4x4 m_view;
    QMatrix4x4 m_projection;
    QMap <QString, std::shared_ptr <QOpenGLShaderProgram>> m_shaderProgramMap;
    bool m_isInitialized = false;
    QOpenGLShaderProgram* m_shaderProgram;

private:
    friend class GraphicsScene3dView;
    QList <std::shared_ptr <SceneGraphicsObject>> m_objectList;
    GraphicsScene3dView* mp_view = nullptr;
    QRectF m_rect;
    qreal m_zoomFactor = 1.0f;
    qreal m_fov = 45.0f;
    qreal m_pitch = 0.0f;
    qreal m_yaw = -90.0f;
};

#endif // GRAPHICSSCENE3D_H
