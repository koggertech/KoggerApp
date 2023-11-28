#include "graphicsscene3dview.h"
#include <graphicsscene3drenderer.h>
#include <surface.h>

#include <memory.h>

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
, m_sceneBoundsPlane(std::make_shared<SceneObject>())
{
    setMirrorVertically(true);
    setAcceptedMouseButtons(Qt::AllButtons);

    QObject::connect(m_surface.get(), &Surface::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_bottomTrack.get(), &BottomTrack::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_polygonGroup.get(), &PolygonGroup::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_pointGroup.get(), &PointGroup::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_coordAxes.get(), &CoordinateAxes::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_planeGrid.get(), &PlaneGrid::changed, this, &QQuickFramebufferObject::update);

    QObject::connect(m_surface.get(), &Surface::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_bottomTrack.get(), &BottomTrack::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_polygonGroup.get(), &PolygonGroup::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_pointGroup.get(), &PointGroup::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_coordAxes.get(), &CoordinateAxes::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_planeGrid.get(), &PlaneGrid::boundsChanged, this, &GraphicsScene3dView::updateBounds);

    QObject::connect(m_surface.get(), &Surface::visibilityChanged, m_bottomTrack.get(), &BottomTrack::setDisplayingWithSurface);
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

void GraphicsScene3dView::clear()
{
    m_surface->clearData();
    m_bottomTrack->clearData();
    m_polygonGroup->clearData();
    m_pointGroup->clearData();

    m_camera->setIsometricView();
    m_axesThumbnailCamera->setIsometricView();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mousePressTrigger(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)

    if(m_mode == BottomTrackVertexComboSelectionMode){
        if(buttons & Qt::LeftButton){
            m_bottomTrack->resetVertexSelection();
            m_comboSelectionRect.setTopLeft({static_cast<int>(x),static_cast<int>(height()-y)});
            m_comboSelectionRect.setBottomRight({static_cast<int>(x),static_cast<int>(height()-y)});
        }
    }

    m_startMousePos = {x,y};

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseMoveTrigger(Qt::MouseButtons buttons, qreal x, qreal y)
{
    if(buttons.testFlag(Qt::MiddleButton)){
        m_comboSelectionRect = {0,0,0,0};
        m_camera->move(QVector2D(m_startMousePos.x(),m_startMousePos.y()),
                       QVector2D(x,y));
        QQuickFramebufferObject::update();
    }

    if(buttons.testFlag(Qt::RightButton)){
        m_comboSelectionRect = {0,0,0,0};
        float deltaAngleX = (2 * M_PI / size().width());
        float deltaAngleY = (2 * M_PI / size().height());
        float yaw = (m_lastMousePos.x() - x) * deltaAngleX;
        float pitch = (m_lastMousePos.y() - y) * deltaAngleY;

        m_camera->rotate(yaw, -pitch);
        m_axesThumbnailCamera->rotate(yaw, -pitch);
        QQuickFramebufferObject::update();
    }

    if(m_mode == BottomTrackVertexComboSelectionMode){
        if(buttons & Qt::LeftButton)
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
    //---------------------------------------------//

    //m_rayCaster->reset();
    //m_rayCaster->setMode(RayCaster::RayCastMode::Triangle);
    //m_rayCaster->addObject(m_surface);
    //m_rayCaster->trigger(m_ray.origin(), m_ray.direction());
    //auto hits = m_rayCaster->hits();
    //
    //m_polygonGroup->clearData();
    //if(!hits.isEmpty()){
    //    auto polygon = std::make_shared<PolygonObject>();
    //    auto hit = hits.first();
    //    for(int i = hit.indices().first; i <= hit.indices().second; i++){
    //        auto vertex = hit.sourceObject().lock()->cdata().at(i);
    //        auto point = std::make_shared<PointObject>();
    //        point->setPosition(vertex.x(), vertex.y(), vertex.z());
    //        polygon->append(point);
    //    }
    //    m_polygonGroup->addPolygon(polygon);
    //}
    //
    //qDebug() << "Surface triangle raycast test. Size is " << hits.size();

    //--------------------------------------------

    m_bottomTrack->mouseMoveEvent(buttons,x,y);
    m_lastMousePos = {x,y};
    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseReleaseTrigger(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(buttons)

    //TODO: Commit only if camera in movement state
    m_camera->commitMovement();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseWheelTrigger(Qt::MouseButtons buttons, qreal x, qreal y, QPointF angleDelta)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if(m_mode == BottomTrackVertexComboSelectionMode){
        m_comboSelectionRect = {0,0,0,0};
        m_bottomTrack->resetVertexSelection();
    }

    m_camera->zoom(angleDelta.y());

    updatePlaneGrid();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::keyPressTrigger(Qt::Key key)
{
    m_bottomTrack->keyPressEvent(key);

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::fitAllInView()
{
    auto maxSize = std::max(m_bounds.width(),
                            std::max(m_bounds.height() * m_verticalScale,
                                     m_bounds.length()));

    auto d = (maxSize/2.0f)/(std::tan(m_camera->fov()/2.0f)) * 2.5f;

    m_camera->focusOnPosition(m_bounds.center());

    if(d>0) m_camera->setDistance(d);

    updatePlaneGrid();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setIsometricView()
{
    m_camera->setIsometricView();
    m_axesThumbnailCamera->setIsometricView();

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
    else if(m_verticalScale < 1.0f)
        m_verticalScale = 1.0f;
    else if(m_verticalScale > 10.0f)
        m_verticalScale = 10.0f;
    else
        m_verticalScale = scale;

    //m_model.translate(0.0f,0.0f,m_verticalScale*0.5);
    //m_model.scale(1.0f, 1.0f, m_verticalScale);


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

void GraphicsScene3dView::updateBounds()
{
    Cube bounds;
    bounds.merge(m_bottomTrack->bounds())
          .merge(m_surface->bounds())
          .merge(m_polygonGroup->bounds())
          .merge(m_pointGroup->bounds());

    m_bounds = std::move(bounds);

    updatePlaneGrid();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::updatePlaneGrid()
{
    m_planeGrid->setSize(QSize(m_bounds.length(), m_bounds.width()));
    m_planeGrid->setPosition(m_bounds.bottomPos());

    if(m_camera->distToFocusPoint() < 65)
        m_planeGrid->setCellSize(1);
    if(m_camera->distToFocusPoint() >= 65 && m_camera->distToFocusPoint() <= 130)
        m_planeGrid->setCellSize(3);
    if(m_camera->distToFocusPoint() >= 130 && m_camera->distToFocusPoint() <= 230)
        m_planeGrid->setCellSize(5);
    if(m_camera->distToFocusPoint() > 230)
        m_planeGrid->setCellSize(10);
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
    update();
}

void GraphicsScene3dView::InFboRenderer::synchronize(QQuickFramebufferObject * fbo)
{
    auto view = qobject_cast<GraphicsScene3dView*>(fbo);

    if(!view)
        return;

    // write to renderer
    m_renderer->m_coordAxesRenderImpl    = *(dynamic_cast<CoordinateAxes::CoordinateAxesRenderImplementation*>(view->m_coordAxes->m_renderImpl));
    m_renderer->m_planeGridRenderImpl    = *(dynamic_cast<PlaneGrid::PlaneGridRenderImplementation*>(view->m_planeGrid->m_renderImpl));
    m_renderer->m_bottomTrackRenderImpl  = *(dynamic_cast<BottomTrack::BottomTrackRenderImplementation*>(view->m_bottomTrack->m_renderImpl));
    m_renderer->m_surfaceRenderImpl      = *(dynamic_cast<Surface::SurfaceRenderImplementation*>(view->m_surface->m_renderImpl));
    m_renderer->m_polygonGroupRenderImpl = *(dynamic_cast<PolygonGroup::PolygonGroupRenderImplementation*>(view->m_polygonGroup->m_renderImpl));
    m_renderer->m_pointGroupRenderImpl   = *(dynamic_cast<PointGroup::PointGroupRenderImplementation*>(view->m_pointGroup->m_renderImpl));
    m_renderer->m_viewSize               = view->size();
    m_renderer->m_camera                 = *view->m_camera;
    m_renderer->m_axesThumbnailCamera    = *view->m_axesThumbnailCamera;
    m_renderer->m_comboSelectionRect     = view->m_comboSelectionRect;
    m_renderer->m_verticalScale          = view->m_verticalScale;

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

void GraphicsScene3dView::Camera::rotate(qreal yaw, qreal pitch)
{
    QVector3D viewDir = (m_eye-m_lookAt).normalized();
    QVector3D right = QVector3D::crossProduct(viewDir,m_up).normalized();

    float cosAngle = QVector3D::dotProduct(viewDir, m_up);

    auto sgn = [](float val){
        return (float(0) < val) - (val < float(0));
    };

    if(cosAngle * sgn(pitch) > 0.99f)
        pitch = 0.0f;

    yaw *= m_sensivity * 65.0f;
    pitch *= m_sensivity * 65.0f;

    QMatrix4x4 rotationMatrixX;
    rotationMatrixX.setToIdentity();
    rotationMatrixX.rotate(yaw, m_up);
    m_relativeOrbitPos = (rotationMatrixX * QVector4D(m_relativeOrbitPos, 1.0f)).toVector3D();

    QMatrix4x4 rotationMatrixY;
    rotationMatrixY.setToIdentity();
    rotationMatrixY.rotate(pitch, right);
    m_relativeOrbitPos = (rotationMatrixY * QVector4D(m_relativeOrbitPos, 1.0f)).toVector3D();

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::move(const QVector2D &startPos, const QVector2D &endPos)
{
    QMatrix4x4 inverted = m_view.inverted();

    QVector4D horizontalAxis = inverted * QVector4D(-1.0f, 0.0f, 0.0f, 0.0f);
    QVector4D verticalAxis = inverted * QVector4D(0.0f, 1.0f, 0.0f, 0.0f);

    m_deltaOffset = ((horizontalAxis * (float)(endPos.x() - startPos.x()) +
                      verticalAxis * (float)(endPos.y() - startPos.y())) *
                      m_sensivity *
                      0.05f)
            .toVector3D();

    m_useFocusPoint = false;

    auto lookAt = m_lookAt + m_deltaOffset;
    updateViewMatrix(&lookAt);
}

void GraphicsScene3dView::Camera::zoom(qreal delta)
{
    if(delta > 0.0f)
        m_distToFocusPoint = m_distToFocusPoint / 1.15f;
    else
        m_distToFocusPoint = m_distToFocusPoint * 1.15f;

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
    m_useFocusPoint = true;

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
    m_relativeOrbitPos = {static_cast<float>(sqrt(1.0f / 3.0f)),
                          static_cast<float>(sqrt(1.0f / 3.0f)),
                          static_cast<float>(sqrt(1.0f / 3.0f))};

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

    m_pitch = 0.0f;
    m_yaw = 0.0f;
    m_fov = 45.0f;
    m_distToFocusPoint = 25.0f;

    m_useFocusPoint = false;

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::updateViewMatrix(QVector3D* lookAt)
{
    auto _lookAt = lookAt ? *lookAt : m_lookAt;

    m_eye = _lookAt + m_relativeOrbitPos * m_distToFocusPoint;

    QMatrix4x4 view;
    view.lookAt(m_eye, _lookAt, m_up);

    m_view = std::move(view);
}

qreal GraphicsScene3dView::Camera::distToFocusPoint() const
{
    return m_distToFocusPoint;
}
