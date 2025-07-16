#ifndef GRAPHICSSCENE3DVIEW_H
#define GRAPHICSSCENE3DVIEW_H

#include <QQuickFramebufferObject>
#include <QtMath>
#include "coordinate_axes.h"
#include "plane_grid.h"
#include "ray_caster.h"
#include "surface.h"
#include "side_scan_view.h"
#include "image_view.h"
#include "map_view.h"
#include "contacts.h"
#include "boat_track.h"
#include "bottom_track.h"
#include "polygon_group.h"
#include "point_group.h"
#include "ray.h"
#include "navigation_arrow.h"
#include "usbl_view.h"
#include "tile_manager.h"
#include "surface_view.h"


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
        explicit Camera(GraphicsScene3dView* viewPtr = nullptr);
        Camera(qreal pitch,
               qreal yaw,
               qreal distToFocusPoint,
               qreal fov,
               qreal sensivity);

        float distForMapView() const;
        qreal distToFocusPoint() const;
        qreal fov() const;
        qreal pitch() const;
        qreal yaw() const;
        QMatrix4x4 viewMatrix() const;

        void setCameraListener(Camera* cameraListener) {
            cameraListener_ = cameraListener;
        };

        //void rotate(qreal yaw, qreal pitch); //TODO! Process this method later
        void rotate(const QVector2D& lastMouse, const QVector2D& mousePos);
        void rotate(const QPointF& prevCenter, const QPointF& currCenter, qreal angleDelta, qreal widgetHeight);
        //void move(const QVector2D& startPos, const QVector2D& endPos);
        void move(const QVector2D &lastMouse, const QVector2D &mousePos);
        void resetZAxis();
        void moveZAxis(float z);
        void zoom(qreal delta);
        void commitMovement();
        void focusOnObject(std::weak_ptr<SceneObject> object);
        void focusOnPosition(const QVector3D& pos);
        void setDistance(qreal distance);
        void setIsometricView();
        void setMapView();
        void reset();
        void resetRotationAngle();

        float getHeightAboveGround() const;
        float getAngleToGround() const;
        bool getIsPerspective() const;
        bool getIsFarAwayFromOriginLla() const;
        map::CameraTilt getCameraTilt() const;

    private:
        void updateCameraParams();
        void tryToChangeViewLlaRef();
        void updateViewMatrix();
        void checkRotateAngle();
        void tryResetRotateAngle();

    private:
        friend class GraphicsScene3dView;
        friend class GraphicsScene3dRenderer;

        Camera* cameraListener_ = nullptr;

        QVector3D m_eye = {0.0f, 0.0f, 0.0f};
        QVector3D m_up = {0.0f, 1.0f, 0.0f};
        QVector3D m_lookAt = {0.0f, 0.0f, 0.0f};
        QVector3D m_lookAtSave = {0.0f, 0.0f, 0.0f};
        QVector3D m_relativeOrbitPos = {0.0f, 0.0f, 0.0f};

        QMatrix4x4 m_view;

        std::weak_ptr<SceneObject> m_focusedObject;
        QVector3D m_offset;
        QVector3D m_deltaOffset;
        QVector3D m_focusPoint;

        qreal m_pitch = 0.f;
        qreal m_yaw = 0.f;
        qreal m_fov = 45.f;
        float m_distToFocusPoint = 50.f;
        float distForMapView_ = m_distToFocusPoint;
        qreal m_sensivity = 4.f;
        float distToGround_ = 0.0f;
        float angleToGround_ = 0.0f;
        bool isPerspective_ = false;
        float highDistThreshold_ = 5000.0f;
        float lowDistThreshold_ = highDistThreshold_ * 0.9f;
        QVector2D m_rotAngle;
        GraphicsScene3dView* viewPtr_;
        LLARef datasetLlaRef_;
        LLA yerevanLla = LLA(40.1852f, 44.5149f, 0.0f);
        LLARef viewLlaRef_ = LLARef(yerevanLla);
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

        void processMapTextures(GraphicsScene3dView* viewPtr) const;
        void processColorTableTexture(GraphicsScene3dView* viewPtr) const;
        void processTileTexture(GraphicsScene3dView* viewPtr) const;
        void processImageTexture(GraphicsScene3dView* viewPtr) const;
        void processSurfaceViewTexture(GraphicsScene3dView* viewPtr) const;

        QString checkOpenGLError() const;

        std::unique_ptr <GraphicsScene3dRenderer> m_renderer;
    };

    enum ActiveMode{
        Idle                                = 0, // not used
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
    std::shared_ptr<BoatTrack> boatTrack() const;
    std::shared_ptr<BottomTrack> bottomTrack() const;
    std::shared_ptr<Surface> surface() const;
    std::shared_ptr<SurfaceView> getSurfaceViewPtr() const;
    std::shared_ptr<SideScanView> getSideScanViewPtr() const;
    std::shared_ptr<ImageView> getImageViewPtr() const;
    std::shared_ptr<MapView> getMapViewPtr() const;
    std::shared_ptr<Contacts> getContactsPtr() const;
    std::shared_ptr<PointGroup> pointGroup() const;
    std::shared_ptr<PolygonGroup> polygonGroup() const;
    std::shared_ptr<UsblView> getUsblViewPtr() const;
    std::shared_ptr<NavigationArrow> getNavigationArrowPtr() const;
    std::weak_ptr <Camera> camera() const;
    float verticalScale() const;
    bool sceneBoundingBoxVisible() const;
    Dataset* dataset() const;
    void clear(bool cleanMap = false);
    QVector3D calculateIntersectionPoint(const QVector3D &rayOrigin, const QVector3D &rayDirection, float planeZ);
    void setUpdateMosaic(bool state);
    void setUpdateIsobaths(bool state);
    void interpolateDatasetEpochs(bool fromStart);
    void updateProjection();
    void setNeedToResetStartPos(bool state);
    void forceUpdateDatasetRef();
    void setOpeningFileState(bool state);

    Q_INVOKABLE void switchToBottomTrackVertexComboSelectionMode(qreal x, qreal y);
    Q_INVOKABLE void mousePressTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey = Qt::Key::Key_unknown);
    Q_INVOKABLE void mouseMoveTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey = Qt::Key::Key_unknown);
    Q_INVOKABLE void mouseReleaseTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey = Qt::Key::Key_unknown);
    Q_INVOKABLE void mouseWheelTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, QPointF angleDelta, Qt::Key keyboardKey = Qt::Key::Key_unknown);
    Q_INVOKABLE void pinchTrigger(const QPointF& prevCenter, const QPointF& currCenter, qreal scaleDelta, qreal angleDelta);
    Q_INVOKABLE void keyPressTrigger(Qt::Key key);
    Q_INVOKABLE void bottomTrackActionEvent(BottomTrack::ActionEvent actionEvent);

    void setTrackLastData(bool state);
    void setUpdateBottomTrack(bool state);
    void setIsFileOpening(bool state);

protected:
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override final;

public Q_SLOTS:
    void setSceneBoundingBoxVisible(bool visible);
    void fitAllInView();
    void setIsometricView();
    void setCancelZoomView();
    void setMapView();
    void setLastEpochFocusView();
    void setIdleMode();
    void setVerticalScale(float scale);
    void shiftCameraZAxis(float shift);
    void setBottomTrackVertexSelectionMode();
    void setPolygonCreationMode();
    void setPolygonEditingMode();
    void setDataset(Dataset* dataset);
    void addPoints(QVector<QVector3D>, QColor color, float width = 1);
    void setQmlRootObject(QObject* object);
    void setQmlAppEngine(QQmlApplicationEngine* engine);
    void updateMapView();
    void updateViews();

signals:
    void sendRectRequest(QVector<LLA> rect, bool isPerspective, LLARef viewLlaRef, bool moveUp, map::CameraTilt tiltCam);
    void sendLlaRef(LLARef viewLlaRef);
    void cameraIsMoved();

private:
    void updateBounds();
    void updatePlaneGrid();
    void clearComboSelectionRect();

private:
    friend class BottomTrack;
    friend class BoatTrack;

    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<Camera> m_axesThumbnailCamera;
    QPointF m_startMousePos = {0.0f, 0.0f};
    QPointF m_lastMousePos = {0.0f, 0.0f};
    std::shared_ptr<RayCaster> m_rayCaster;
    std::shared_ptr<Surface> m_surface;
    std::shared_ptr<SurfaceView> surfaceView_;
    std::shared_ptr<SideScanView> sideScanView_;
    std::shared_ptr<ImageView> imageView_;
    std::shared_ptr<MapView> mapView_;
    std::shared_ptr<Contacts> contacts_;
    std::shared_ptr<BoatTrack> m_boatTrack;
    std::shared_ptr<BottomTrack> m_bottomTrack;
    std::shared_ptr<PolygonGroup> m_polygonGroup;
    std::shared_ptr<PointGroup> m_pointGroup;
    std::shared_ptr<CoordinateAxes> m_coordAxes;
    std::shared_ptr<PlaneGrid> m_planeGrid;
    std::shared_ptr<SceneObject> m_vertexSynchroCursour;
    std::shared_ptr<NavigationArrow> navigationArrow_;
    std::shared_ptr<UsblView> usblView_;
    std::shared_ptr<map::TileManager> tileManager_;

    QMatrix4x4 m_model;
    QMatrix4x4 m_projection;
    Cube m_bounds;
    ActiveMode m_mode = ActiveMode::BottomTrackVertexSelectionMode;
    ActiveMode lastMode_ = ActiveMode::BottomTrackVertexSelectionMode;
    QRect m_comboSelectionRect = { 0, 0, 0, 0 };
    Ray m_ray;
    float m_verticalScale = 1.0f;
    bool m_isSceneBoundingBoxVisible = true;
    Dataset* m_dataset = nullptr;
    bool updateMosaic_;
    bool updateIsobaths_;
#if defined (Q_OS_ANDROID) || defined (LINUX_ES)
    static constexpr double mouseThreshold_{ 15.0 };
#else
    static constexpr double mouseThreshold_{ 10.0 };
#endif

    static constexpr float perspectiveEdge_{ 5000.0f };
    static constexpr float nearPlanePersp_{ 1.0f };
    static constexpr float farPlanePersp_{ 20000.0f };
    static constexpr float nearPlaneOrthoCoeff_{ 0.05f };
    static constexpr float farPlaneOrthoCoeff_{ 1.2f };

    bool wasMoved_;
    Qt::MouseButtons wasMovedMouseButton_;
    QObject* qmlRootObject_ = nullptr;
    bool switchedToBottomTrackVertexComboSelectionMode_;
    int bottomTrackWindowCounter_;
    bool needToResetStartPos_;
    float lastCameraDist_;
    bool trackLastData_;
    bool updateBottomTrack_;
    bool isOpeningFile_;
    bool isFileOpening_ = false;
};

#endif // GRAPHICSSCENE3DVIEW_H
