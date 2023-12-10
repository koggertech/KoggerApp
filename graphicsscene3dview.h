#ifndef GRAPHICSSCENE3DVIEW_H
#define GRAPHICSSCENE3DVIEW_H

#include <coordinateaxes.h>
#include <planegrid.h>
#include <raycaster.h>
#include <surface.h>
#include <bottomtrack.h>
#include <polygongroup.h>
#include <pointgroup.h>
#include <vertexeditingdecorator.h>
#include <ray.h>

#include <QQuickFramebufferObject>
#include <QtMath>

class Dataset;
class GraphicsScene3dRenderer;
class GraphicsScene3dView : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GraphicsScene3dView)

public:
    //Camera
    class Camera
    {
    public:
        Camera();
        Camera(qreal pitch,
               qreal yaw,
               qreal distToFocusPoint,
               qreal fov,
               qreal sensivity);

        qreal distToFocusPoint() const;
        qreal fov() const;
        qreal pitch() const;
        qreal yaw() const;
        QMatrix4x4 viewMatrix() const;

        //TODO! Process this method later
        //void rotate(qreal yaw, qreal pitch);
        void rotate(const QVector2D& lastMouse, const QVector2D& mousePos);

        //void move(const QVector2D& startPos, const QVector2D& endPos);
        void move(const QVector2D &lastMouse, const QVector2D &mousePos);
        void zoom(qreal delta);
        void commitMovement();
        void focusOnObject(std::weak_ptr<SceneObject> object);
        void focusOnPosition(const QVector3D& pos);
        void setDistance(qreal distance);
        void setIsometricView();
        void reset();

    private:
        void updateViewMatrix(QVector3D* lookAt = nullptr);
    private:
        friend class GraphicsScene3dView;
        friend class GraphicsScene3dRenderer;

        QVector3D m_eye = {0.0f, 0.0f, 0.0f};
        QVector3D m_up = {0.0f, 1.0f, 0.0f};
        QVector3D m_lookAt = {0.0f, 0.0f, 0.0f};
        QVector3D m_relativeOrbitPos = {0.0f, 0.0f, 0.0f};

        QMatrix4x4 m_view;

        std::weak_ptr<SceneObject> m_focusedObject;
        QVector3D m_offset;
        QVector3D m_deltaOffset;
        QVector3D m_focusPoint;

        qreal m_pitch = 0.0f;
        qreal m_yaw = 0.0f;
        qreal m_fov = 45.0f;
        float m_distToFocusPoint = 25.0f;
        qreal m_sensivity = 0.5f;

        QVector2D m_rotAngle;
    };

    //Renderer
    class InFboRenderer : public QQuickFramebufferObject::Renderer
    {
    public:
        InFboRenderer();
        virtual ~InFboRenderer();

    protected:
        virtual void render() override;
        virtual void synchronize(QQuickFramebufferObject * fbo) override;
        virtual QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

    private:
        friend class GraphicsScene3dView;
        std::unique_ptr <GraphicsScene3dRenderer> m_renderer;
    };

    enum ActiveMode{
        Idle                                = 0,
        BottomTrackVertexSelectionMode      = 1,
        BottomTrackVertexComboSelectionMode = 2,
        PolygonCreationMode                 = 3,
        MarkCreationMode                    = 4,
        PolygonEditingMode                  = 5,
        BottomTrackSyncPointCreationMode    = 6,
        ShoreCreationMode                   = 7,
        MeasuringRouteCreationMode          = 8
    };

    /**
     * @brief Constructor
     */
    GraphicsScene3dView();

    /**
     * @brief Destructor
     */
    virtual ~GraphicsScene3dView();

    /**
     * @brief Creates renderer
     * @return renderer
     */
    Renderer *createRenderer() const override;
    std::shared_ptr <BottomTrack> bottomTrack() const;
    std::shared_ptr <Surface> surface() const;
    std::shared_ptr <PointGroup> pointGroup() const;
    std::shared_ptr <PolygonGroup> polygonGroup() const;
    std::weak_ptr <Camera> camera() const;
    float verticalScale() const;
    bool sceneBoundingBoxVisible() const;
    Dataset* dataset() const;
    void clear();


    Q_INVOKABLE void mouseMoveTrigger(Qt::MouseButtons buttons, qreal x, qreal y);
    Q_INVOKABLE void mousePressTrigger(Qt::MouseButtons buttons, qreal x, qreal y);
    Q_INVOKABLE void mouseReleaseTrigger(Qt::MouseButtons buttons, qreal x, qreal y);
    Q_INVOKABLE void mouseWheelTrigger(Qt::MouseButtons buttons, qreal x, qreal y, QPointF angleDelta);
    Q_INVOKABLE void keyPressTrigger(Qt::Key key);

public Q_SLOTS:
    void setSceneBoundingBoxVisible(bool visible);
    void fitAllInView();
    void setIsometricView();
    void setIdleMode();
    void setVerticalScale(float scale);
    void setBottomTrackVertexSelectionMode();
    void setBottomTrackVertexComboSelectionMode();
    void setPolygonCreationMode();
    void setPolygonEditingMode();
    void setDataset(Dataset* dataset);

private:
    void updateBounds();
    void updatePlaneGrid();

private:
    friend class BottomTrack;

    std::shared_ptr <Camera> m_camera;
    std::shared_ptr <Camera> m_axesThumbnailCamera;
    QPointF m_startMousePos = {0.0f, 0.0f};
    QPointF m_lastMousePos = {0.0f, 0.0f};
    std::shared_ptr <RayCaster> m_rayCaster;
    std::shared_ptr <Surface> m_surface;
    std::shared_ptr <BottomTrack> m_bottomTrack;
    std::shared_ptr <PolygonGroup> m_polygonGroup;
    std::shared_ptr <PointGroup> m_pointGroup;
    std::shared_ptr <CoordinateAxes> m_coordAxes;
    std::shared_ptr <PlaneGrid> m_planeGrid;
    std::shared_ptr <SceneObject> m_boatTrack;
    std::shared_ptr <SceneObject> m_vertexSynchroCursour;
    QMatrix4x4 m_model;
    QMatrix4x4 m_projection;
    Cube m_bounds;
    ActiveMode m_mode = Idle;
    QRect m_comboSelectionRect = {0,0,0,0};
    Ray m_ray;
    float m_verticalScale = 1.0f;
    bool m_isSceneBoundingBoxVisible = true;
    Dataset* m_dataset = nullptr;
};

#endif // GRAPHICSSCENE3DVIEW_H
