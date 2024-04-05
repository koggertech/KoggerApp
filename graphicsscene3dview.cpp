#include "graphicsscene3dview.h"
#include <graphicsscene3drenderer.h>
#include <surface.h>
#include <plotcash.h>

#include <memory.h>
#include <math.h>

#include <QOpenGLFramebufferObject>
#include <QVector3D>

GraphicsScene3dView::GraphicsScene3dView()
: QQuickFramebufferObject()
, m_camera(std::make_shared<Camera>())
, m_axesThumbnailCamera(std::make_shared<Camera>())
, m_rayCaster(std::make_shared<RayCaster>())
, m_surface(std::make_shared<Surface>())
, m_bottomTrack(std::make_shared<BottomTrack>(this, this))
, m_polygonGroup(std::make_shared<PolygonGroup>())
, m_pointGroup(std::make_shared<PointGroup>())
, m_coordAxes(std::make_shared<CoordinateAxes>())
, m_planeGrid(std::make_shared<PlaneGrid>())
, m_boatTrack(std::make_shared<SceneObject>())
, m_navigationArrow(std::make_shared<NavigationArrow>())
, navigationArrowState_(false)
{
    setObjectName("GraphicsScene3dView");
    setMirrorVertically(true);
    setAcceptedMouseButtons(Qt::AllButtons);

    m_boatTrack->setColor({80,0,180});
    m_boatTrack->setWidth(6.0f);

    m_navigationArrow->setColor({ 255, 0, 0 });

    QObject::connect(m_surface.get(), &Surface::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_bottomTrack.get(), &BottomTrack::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_polygonGroup.get(), &PolygonGroup::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_pointGroup.get(), &PointGroup::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_coordAxes.get(), &CoordinateAxes::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_planeGrid.get(), &PlaneGrid::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_navigationArrow.get(), &NavigationArrow::changed, this, &QQuickFramebufferObject::update);

    QObject::connect(m_surface.get(), &Surface::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_bottomTrack.get(), &BottomTrack::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_polygonGroup.get(), &PolygonGroup::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_pointGroup.get(), &PointGroup::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_coordAxes.get(), &CoordinateAxes::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_boatTrack.get(), &PlaneGrid::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_surface.get(), &Surface::visibilityChanged, m_bottomTrack.get(), &BottomTrack::setDisplayingWithSurface);
    QObject::connect(m_navigationArrow.get(), &NavigationArrow::boundsChanged, this, &GraphicsScene3dView::updateBounds);

    updatePlaneGrid();
}

GraphicsScene3dView::~GraphicsScene3dView()
{}

QQuickFramebufferObject::Renderer *GraphicsScene3dView::createRenderer() const
{
    return new GraphicsScene3dView::InFboRenderer();
}

std::shared_ptr<BottomTrack> GraphicsScene3dView::bottomTrack() const
{
    return m_bottomTrack;
}

std::shared_ptr<Surface> GraphicsScene3dView::surface() const
{
    return m_surface;
}

std::shared_ptr<PointGroup> GraphicsScene3dView::pointGroup() const
{
    return m_pointGroup;
}

std::shared_ptr<PolygonGroup> GraphicsScene3dView::polygonGroup() const
{
    return m_polygonGroup;
}

std::weak_ptr<GraphicsScene3dView::Camera> GraphicsScene3dView::camera() const
{
    return m_camera;
}

float GraphicsScene3dView::verticalScale() const
{
    return m_verticalScale;
}

bool GraphicsScene3dView::sceneBoundingBoxVisible() const
{
    return m_isSceneBoundingBoxVisible;
}

Dataset *GraphicsScene3dView::dataset() const
{
    return m_dataset;
}

void GraphicsScene3dView::setNavigationArrowState(bool state)
{
    m_navigationArrow->setEnabled(state);
    navigationArrowState_ = state;
}

void GraphicsScene3dView::clear()
{
    m_surface->clearData();
    m_bottomTrack->clearData();
    m_polygonGroup->clearData();
    m_pointGroup->clearData();
    m_boatTrack->clearData();
    m_navigationArrow->clearData();
    navigationArrowState_ = false;
    m_bounds = Cube();

    setIsometricView();

    QQuickFramebufferObject::update();
}

QVector3D GraphicsScene3dView::calculateIntersectionPoint(const QVector3D &rayOrigin, const QVector3D &rayDirection, float planeZ) {
    QVector3D intersectionPoint;

    // Check if the ray is parallel or nearly parallel to the plane
    if (qAbs(rayDirection.z()) < 1e-6) {
        // Ray is parallel or nearly parallel to the plane, no intersection
        return intersectionPoint; // Zero vector indicates no intersection
    }

    // Calculate parameter t
    float t = (planeZ - rayOrigin.z()) / rayDirection.z();

    // Check if intersection point is behind the ray origin
    if (t < 0) {
        // Intersection point is behind the ray origin, discard
        return intersectionPoint; // Zero vector indicates no intersection
    }

    // Calculate intersection point
    intersectionPoint = rayOrigin + rayDirection * t;

    return intersectionPoint;
}

void GraphicsScene3dView::mousePressTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey)
{
    Q_UNUSED(mouseButton)
    Q_UNUSED(keyboardKey)

    if(m_mode == BottomTrackVertexComboSelectionMode){
        if(mouseButton & Qt::LeftButton){
            m_bottomTrack->resetVertexSelection();
            m_comboSelectionRect.setTopLeft({static_cast<int>(x),static_cast<int>(height()-y)});
            m_comboSelectionRect.setBottomRight({static_cast<int>(x),static_cast<int>(height()-y)});
        }
    }

    m_startMousePos = {x, y};

    m_bottomTrack->mousePressEvent(mouseButton, x, y);

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseMoveTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey)
{
    if(mouseButton.testFlag(Qt::RightButton) && (keyboardKey != Qt::Key_Control)) {
        m_comboSelectionRect = {0,0,0,0};
        float deltaAngleX = (2 * M_PI / size().width());
        float deltaAngleY = (2 * M_PI / size().height());
        float yaw = (m_lastMousePos.x() - x) * deltaAngleX * m_camera->m_sensivity;
        float pitch = (m_lastMousePos.y() - y) * deltaAngleY * m_camera->m_sensivity;
        //m_camera->rotate(yaw, -pitch);
        //m_axesThumbnailCamera->rotate(yaw, -pitch);

        m_camera->rotate(QVector2D(m_lastMousePos), QVector2D(x,y));
        m_axesThumbnailCamera->rotate(QVector2D(m_lastMousePos), QVector2D(x,y));

        QQuickFramebufferObject::update();
    }

    if(m_mode == BottomTrackVertexComboSelectionMode){
        if(mouseButton & Qt::LeftButton)
            m_comboSelectionRect.setBottomRight({static_cast<int>(x),static_cast<int>(height()-y)});
        QQuickFramebufferObject::update();
    }

    //---------->Calculate ray in 3d space<---------//
    auto origin = QVector3D(x, height() - y, -1.0f)
            .unproject(m_camera->m_view*m_model,
                       m_projection,
                       boundingRect().toRect());

    auto end = QVector3D(x, height() - y, 1.0f)
            .unproject(m_camera->m_view*m_model,
                       m_projection,
                       boundingRect().toRect());

    auto direction = (end-origin).normalized();

    m_ray.setOrigin(origin);
    m_ray.setDirection(direction);

    if(mouseButton.testFlag(Qt::RightButton) && (keyboardKey == Qt::Key_Control)) {
        auto startMouseOrig = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), -1.0f)
                                  .unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
        auto startMouseEnd = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), 1.0f)
                                 .unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
        auto startMouseDir = (startMouseEnd - startMouseOrig).normalized();

        auto from = calculateIntersectionPoint(startMouseOrig, startMouseDir , 0);
        auto to = calculateIntersectionPoint(origin, direction, 0);

        m_camera->move(QVector2D(from.x(), from.y()), QVector2D(to.x() ,to.y()));

        QQuickFramebufferObject::update();
    }

    m_bottomTrack->mouseMoveEvent(mouseButton,x,y);
    m_lastMousePos = {x,y};
    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseReleaseTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey)
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(mouseButton)

    //TODO: Commit only if camera in movement state
    m_camera->commitMovement();
    m_bottomTrack->mouseReleaseEvent(mouseButton, x, y);
    m_lastMousePos = {x,y};

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseWheelTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, QPointF angleDelta, Qt::Key keyboardKey)
{
    Q_UNUSED(mouseButton)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if(m_mode == BottomTrackVertexComboSelectionMode){
        m_comboSelectionRect = {0,0,0,0};
        m_bottomTrack->resetVertexSelection();
    }

    if (keyboardKey == Qt::Key_Control) {
        float tempVerticalScale = m_verticalScale;
        angleDelta.y() > 0.0f ? tempVerticalScale += 0.3f : tempVerticalScale -= 0.3f;
        setVerticalScale(tempVerticalScale);
    }
    else
        m_camera->zoom(angleDelta.y());

    updatePlaneGrid();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::keyPressTrigger(Qt::Key key)
{
    m_bottomTrack->keyPressEvent(key);

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setSceneBoundingBoxVisible(bool visible)
{
    m_isSceneBoundingBoxVisible = visible;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::fitAllInView()
{
    auto maxSize = std::max(m_bounds.width(),
                            std::max(m_bounds.height(),
                                     m_bounds.length()));

    auto d = (maxSize/2.0f)/(std::tan(m_camera->fov()/2.0f)) * 2.5f;

    if(d>0)
        m_camera->setDistance(d);

    m_camera->focusOnPosition(m_bounds.center());

    updatePlaneGrid();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setIsometricView()
{
    m_camera->setIsometricView();
    m_axesThumbnailCamera->setIsometricView();

    fitAllInView();
    updatePlaneGrid();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setIdleMode()
{
    m_mode = Idle;

    m_comboSelectionRect = {0,0,0,0};
    m_bottomTrack->resetVertexSelection();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setVerticalScale(float scale)
{
    if(m_verticalScale == scale)
        return;
    else if(scale < 1.f)
        m_verticalScale = 1.0f;
    else if(scale > 10.f)
        m_verticalScale = 10.0f;
    else
        m_verticalScale = scale;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setBottomTrackVertexSelectionMode()
{
    setIdleMode();

    m_mode = BottomTrackVertexSelectionMode;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setBottomTrackVertexComboSelectionMode()
{
    setIdleMode();

    m_mode = BottomTrackVertexComboSelectionMode;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setPolygonCreationMode()
{
    setIdleMode();

    m_mode = PolygonCreationMode;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setPolygonEditingMode()
{
    setIdleMode();

    m_mode = PolygonEditingMode;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setDataset(Dataset *dataset)
{
    if(m_dataset)
        QObject::disconnect(m_dataset);

    m_dataset = dataset;

    if(!m_dataset)
        return;

    m_bottomTrack->setDatasetPtr(m_dataset);

    QObject::connect(m_dataset, &Dataset::bottomTrackUpdated, m_bottomTrack.get(), &BottomTrack::isEpochsChanged);
    QObject::connect(m_dataset, &Dataset::boatTrackUpdated, this, [this]()->void {
        m_boatTrack->setData(m_dataset->boatTrack(), GL_LINE_STRIP);
        if (navigationArrowState_) {
            const Position pos = m_dataset->getLastPosition();
            m_navigationArrow->setPositionAndAngle(QVector3D(pos.ned.n, pos.ned.e, !isfinite(pos.ned.d) ? 0.f : pos.ned.d), m_dataset->getLastYaw() - 90.f);
        }
    });

}

void GraphicsScene3dView::updateBounds()
{
    m_bounds = m_boatTrack->bounds()
        .merge(m_surface->bounds())
        .merge(m_bottomTrack->bounds())
        .merge(m_navigationArrow->bounds());
        //.merge(m_polygonGroup->bounds())
        //.merge(m_pointGroup->bounds());

    updatePlaneGrid();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::updatePlaneGrid()
{
    m_planeGrid->setPlane(m_bounds.bottom());
    m_planeGrid->setCellSize(10);
    // if(m_camera->distToFocusPoint() < 65)
    //     m_planeGrid->setCellSize(1);
    // if(m_camera->distToFocusPoint() >= 65 && m_camera->distToFocusPoint() <= 130)
    //     m_planeGrid->setCellSize(3);
    // if(m_camera->distToFocusPoint() >= 130 && m_camera->distToFocusPoint() <= 230)
    //     m_planeGrid->setCellSize(5);
    // if(m_camera->distToFocusPoint() > 230)
    //     m_planeGrid->setCellSize(10);
}

//---------------------Renderer---------------------------//
GraphicsScene3dView::InFboRenderer::InFboRenderer()
    :QQuickFramebufferObject::Renderer()
    , m_renderer(new GraphicsScene3dRenderer)
{
    m_renderer->initialize();
}

GraphicsScene3dView::InFboRenderer::~InFboRenderer()
{}

void GraphicsScene3dView::InFboRenderer::render()
{
    m_renderer->render();
}

void GraphicsScene3dView::InFboRenderer::synchronize(QQuickFramebufferObject * fbo)
{
    auto view = qobject_cast<GraphicsScene3dView*>(fbo);

    if(!view)
        return;

    // write to renderer
    m_renderer->m_coordAxesRenderImpl       = *(dynamic_cast<CoordinateAxes::CoordinateAxesRenderImplementation*>(view->m_coordAxes->m_renderImpl));
    m_renderer->m_planeGridRenderImpl       = *(dynamic_cast<PlaneGrid::PlaneGridRenderImplementation*>(view->m_planeGrid->m_renderImpl));
    m_renderer->m_bottomTrackRenderImpl     = *(dynamic_cast<BottomTrack::BottomTrackRenderImplementation*>(view->m_bottomTrack->m_renderImpl));
    m_renderer->m_surfaceRenderImpl         = *(dynamic_cast<Surface::SurfaceRenderImplementation*>(view->m_surface->m_renderImpl));
    m_renderer->m_polygonGroupRenderImpl    = *(dynamic_cast<PolygonGroup::PolygonGroupRenderImplementation*>(view->m_polygonGroup->m_renderImpl));
    m_renderer->m_pointGroupRenderImpl      = *(dynamic_cast<PointGroup::PointGroupRenderImplementation*>(view->m_pointGroup->m_renderImpl));
    m_renderer->m_boatTrackRenderImpl       = *(view->m_boatTrack->m_renderImpl);
    m_renderer->m_navigationArrowRenderImpl = *(dynamic_cast<NavigationArrow::NavigationArrowRenderImplementation*>(view->m_navigationArrow->m_renderImpl));
    m_renderer->m_viewSize                  = view->size();
    m_renderer->m_camera                    = *view->m_camera;
    m_renderer->m_axesThumbnailCamera       = *view->m_axesThumbnailCamera;
    m_renderer->m_comboSelectionRect        = view->m_comboSelectionRect;
    m_renderer->m_verticalScale             = view->m_verticalScale;
    m_renderer->m_boundingBox               = view->m_bounds;
    m_renderer->m_isSceneBoundingBoxVisible = view->m_isSceneBoundingBoxVisible;

    //read from renderer
    view->m_model = m_renderer->m_model;
    view->m_projection = m_renderer->m_projection;
}

QOpenGLFramebufferObject *GraphicsScene3dView::InFboRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

GraphicsScene3dView::Camera::Camera()
{
    setIsometricView();
}

GraphicsScene3dView::Camera::Camera(qreal pitch,
                                    qreal yaw,
                                    qreal distToFocusPoint,
                                    qreal fov,
                                    qreal sensivity)
    :m_pitch(std::move(pitch))
    ,m_yaw(std::move(yaw))
    ,m_fov(std::move(fov))
    ,m_distToFocusPoint(std::move(distToFocusPoint))
    ,m_sensivity(std::move(sensivity))
{
   setIsometricView();
}

qreal GraphicsScene3dView::Camera::fov() const
{
    return m_fov;
}

qreal GraphicsScene3dView::Camera::pitch() const
{
    return m_pitch;
}

qreal GraphicsScene3dView::Camera::yaw() const
{
    return m_yaw;
}

QMatrix4x4 GraphicsScene3dView::Camera::viewMatrix() const
{
    return m_view;
}

//void GraphicsScene3dView::Camera::rotate(qreal yaw, qreal pitch)
//{
//    QVector3D viewDir = (m_eye-m_lookAt).normalized();
//    QVector3D right = QVector3D::crossProduct(viewDir,m_up).normalized();
//
//    float cosAngle = QVector3D::dotProduct(viewDir,m_up);
//
//    auto sgn = [](float val){
//        return (float(0) < val) - (val < float(0));
//    };
//
//    if(cosAngle * sgn(pitch) > 0.99f)
//        pitch = 0.0f;
//
//    yaw *= m_sensivity * 65.0f;
//    pitch *= m_sensivity * 65.0f;
//
//    QMatrix4x4 rotationMatrixX;
//    rotationMatrixX.setToIdentity();
//    rotationMatrixX.rotate(yaw,m_up);
//    m_relativeOrbitPos = (rotationMatrixX * QVector4D(m_relativeOrbitPos, 1.0f)).toVector3D();
//
//    QMatrix4x4 rotationMatrixY;
//    rotationMatrixY.setToIdentity();
//    rotationMatrixY.rotate(pitch, right);
//    m_relativeOrbitPos = (rotationMatrixY * QVector4D(m_relativeOrbitPos, 1.0f)).toVector3D();
//
//    updateViewMatrix();
//}

void GraphicsScene3dView::Camera::rotate(const QVector2D& lastMouse, const QVector2D& mousePos)
{
    auto r = (lastMouse - mousePos)*0.2;
    r.setX(qDegreesToRadians(r.x()));
    r.setY(qDegreesToRadians(r.y()));

    m_rotAngle += r;

    if(m_rotAngle[1] > M_PI_2 ) {
        m_rotAngle[1] = M_PI_2;
    } else if(m_rotAngle[1] < 0) {
        m_rotAngle[1] = 0;
    }

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::move(const QVector2D &startPos, const QVector2D &endPos)
{
    QVector4D horizontalAxis{ -1.0f, 0.0f, 0.0f, 0.0f };
    QVector4D verticalAxis{ 0.0f, -1.0f, 0.0f, 0.0f };

    m_deltaOffset = ((horizontalAxis * (float)(endPos.x() - startPos.x()) +
                      verticalAxis * (float)(endPos.y() - startPos.y()))).toVector3D();

    auto lookAt = m_lookAt + m_deltaOffset;

    updateViewMatrix(&lookAt);
}

//void GraphicsScene3dView::Camera::move(const QVector2D &lastMouse, const QVector2D &mousePos)
//{
//    QVector3D vm = QVector3D(-(qDegreesToRadians(lastMouse.x()) - qDegreesToRadians(mousePos.x())), (qDegreesToRadians(lastMouse.y()) - qDegreesToRadians(mousePos.y())), 0)*(m_fov*0.002);
//
//    m_focusPosition[0] += (vm[1]*cosf(-m_rotAngle.x())*cosf(m_rotAngle.y()) - vm[0]*sinf(-m_rotAngle.x()));
//    m_focusPosition[1] += (vm[1]*sinf(-m_rotAngle.x())*cosf(m_rotAngle.y()) + vm[0]*cosf(-m_rotAngle.x()));
//    m_focusPosition[2] += -vm[1]*sinf(m_rotAngle.y())*sinf(m_rotAngle.y());
//
//    updateViewMatrix();
//}

void GraphicsScene3dView::Camera::zoom(qreal delta)
{
    m_distToFocusPoint = delta > 0.f ? m_distToFocusPoint / 1.15f : m_distToFocusPoint * 1.15f;

    if (m_distToFocusPoint < 2.f)
        m_distToFocusPoint = 2.f;
    if (m_distToFocusPoint >= 10000.f)
        m_distToFocusPoint = 10000.f;

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::commitMovement()
{
    m_lookAt += m_deltaOffset;
    m_deltaOffset = QVector3D();

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::focusOnObject(std::weak_ptr<SceneObject> object)
{
    Q_UNUSED(object)
}

void GraphicsScene3dView::Camera::focusOnPosition(const QVector3D &point)
{
    m_lookAt = point;

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::setDistance(qreal distance)
{
    m_distToFocusPoint = distance;

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::setIsometricView()
{
    reset();

    m_rotAngle.setX(qDegreesToRadians(135.0f));
    m_rotAngle.setY(qDegreesToRadians(45.0f));

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::reset()
{
    m_eye = {0.0f, 0.0f, 20.0f};
    m_lookAt = {0.0f, 0.0f, 0.0f};
    m_relativeOrbitPos = m_eye;

    m_focusedObject.lock() = nullptr;
    m_deltaOffset = {0.0f, 0.0f, 0.0f};
    m_focusPoint = {0.0f, 0.0f, 0.0f};

    m_pitch = 0.f;
    m_yaw = 0.f;
    m_fov = 45.f;
    m_distToFocusPoint = 50.f;

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::updateViewMatrix(QVector3D* lookAt)
{
    auto _lookAt = lookAt ? *lookAt : m_lookAt;
    _lookAt.setZ(-_lookAt.z());

    QVector3D cf;
    cf[0] = -sinf(m_rotAngle.y())*cosf(-m_rotAngle.x())*m_distToFocusPoint;
    cf[1] = -sinf(m_rotAngle.y())*sinf(-m_rotAngle.x())*m_distToFocusPoint;
    cf[2] = -cosf(m_rotAngle.y())*m_distToFocusPoint;

    QVector3D cu;
    cu[0] = cosf(m_rotAngle.y())*cosf(-m_rotAngle.x());
    cu[1] = cosf(m_rotAngle.y())*sinf(-m_rotAngle.x());
    cu[2] = -sinf(m_rotAngle.y());

    QMatrix4x4 view;
    view.lookAt(cf + _lookAt, _lookAt, cu.normalized());
    view.scale(1.0f,1.0f,-1.0f);

    m_view = std::move(view);
}

qreal GraphicsScene3dView::Camera::distToFocusPoint() const
{
    return m_distToFocusPoint;
}
