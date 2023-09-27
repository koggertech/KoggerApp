#ifndef GRAPHICSSCENE3D_H
#define GRAPHICSSCENE3D_H

#include <memory>

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QVector2D>
#include <QMutex>

class BottomTrack;
class Surface;
class PointGroup;
class PolygonGroup;
class GraphicsScene3dView;
class SceneGraphicsObject;
class QOpenGLShaderProgram;
class GraphicsScene3d : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GraphicsScene3d(QObject* parent = nullptr);
    virtual ~GraphicsScene3d();

    void lock();
    void unlock();
    std::shared_ptr <BottomTrack> bottomTrack() const;
    std::shared_ptr <Surface> surface() const;
    std::shared_ptr <PointGroup> pointGroup() const;
    std::shared_ptr <PolygonGroup> polygonGroup() const;
    QList <std::shared_ptr <SceneGraphicsObject>> objects() const;
    QRectF rect() const;
    qreal width() const;
    qreal height() const;
    //GraphicsScene3dView* view() const;
    bool isInitialized() const;
    qreal fov() const;

public Q_SLOTS:
    void addGraphicsObject(std::shared_ptr <SceneGraphicsObject> object);
    void setGraphicsObjects(const QList<std::shared_ptr<SceneGraphicsObject>>& objects);
    void clear();
    void clearGraphicsObjects();
    void removeGraphicsObject(std::shared_ptr <SceneGraphicsObject> object);
    void setView(GraphicsScene3dView* view);
    void draw();
    void setBottomTrack(std::shared_ptr <BottomTrack> bottomTrack);
    void setSurface(std::shared_ptr <Surface> surface);
    void setCameraZoom(qreal angle);
    void rotateCamera(qreal pitchOffset, qreal yawOffset);
    void setWidth(qreal width);
    void setRect(const QRectF& rect);
    void setHeight(qreal height);
    void setDragOffset(QVector3D offset);
    void setRotationAngle(QVector2D angle);

Q_SIGNALS:
    void objectsCountChanged(QList <std::shared_ptr <SceneGraphicsObject>> objects);

private:
    void initialize();
    void drawObjects();
    QMatrix4x4 model() const;
    QMatrix4x4 view() const;
    QMatrix4x4 projection() const;

protected:
    QMatrix4x4 m_model;
    QMatrix4x4 m_view;
    QMatrix4x4 m_projection;
    QMap <QString, std::shared_ptr <QOpenGLShaderProgram>> m_shaderProgramMap;
    bool m_isInitialized = false;

private:
    friend class GraphicsScene3dView;
    std::shared_ptr <BottomTrack> m_bottomTrack;
    std::shared_ptr <Surface> m_surface;
    std::shared_ptr <PointGroup> m_pointGroup;
    std::shared_ptr <PolygonGroup> m_polygonGroup;
    QList <std::shared_ptr <SceneGraphicsObject>> m_objectList;
    GraphicsScene3dView* mp_view = nullptr;
    QRectF m_rect;
    QVector3D m_dragOffset;
    QVector3D m_rotationAngle;
    QVector3D m_center;
    qreal m_fov = 45.0f;
    qreal m_pitch = 0.0f;
    qreal m_yaw = -90.0f;
    QMutex m_mutex;
};

#endif // GRAPHICSSCENE3D_H
