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
, m_bottomTrack(std::make_shared<BottomTrack>())
, m_polygonGroup(std::make_shared<PolygonGroup>())
, m_pointGroup(std::make_shared<PointGroup>())
, m_coordAxes(std::make_shared<CoordinateAxes>())
, m_planeGrid(std::make_shared<PlaneGrid>())
, m_sceneBoundsPlane(std::make_shared<SceneObject>())
{
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

void GraphicsScene3dView::clear()
{
    m_surface->clearData();
    m_bottomTrack->clearData();
    m_polygonGroup->clearData();
    m_pointGroup->clearData();

    m_camera->reset();
    m_axesThumbnailCamera->reset();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseMoveTrigger(Qt::MouseButtons buttons, qreal x, qreal y)
{
    if(buttons.testFlag(Qt::RightButton)){
        m_camera->move(QVector2D(m_startMousePos.x(),m_startMousePos.y()),
                       QVector2D(x,y));
    }else{
        m_camera->commitMovement();
    }

    if(buttons.testFlag(Qt::LeftButton)){
        float deltaAngleX = (2 * M_PI / size().width());
        float deltaAngleY = (2 * M_PI / size().height());
        float yaw = (m_lastMousePos.x() - x) * deltaAngleX;
        float pitch = (m_lastMousePos.y() - y) * deltaAngleY;

        m_camera->rotate(yaw, -pitch);
        m_axesThumbnailCamera->rotate(yaw, -pitch);
    }

    m_lastMousePos = {x,y};

    auto origin = QVector3D(x, y, -1.0f)
            .unproject(m_model * m_view,
                       m_projection,
                       boundingRect().toRect());

    auto end = QVector3D(x, y, 1.0f)
            .unproject(m_model * m_view,
                       m_projection,
                       boundingRect().toRect());

    auto direction = (end - origin).normalized();

    m_rayCaster->trigger(origin, direction);

    QList <std::shared_ptr <SceneObject>> pickedObjects;

    for(const auto& hit : m_rayCaster->hits()){
        auto object = std::make_shared<SceneObject>();
        object->setData(hit.sourcePrimitive().first, hit.sourcePrimitive().second);
        pickedObjects.append(object);
    }

    //m_scene->setGraphicsObjects(pickedObjects);

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mousePressTrigger(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)

    m_startMousePos = {x,y};

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseReleaseTrigger(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(buttons)
}

void GraphicsScene3dView::mouseWheelTrigger(Qt::MouseButtons buttons, qreal x, qreal y, QPointF angleDelta)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    m_camera->zoom(angleDelta.y());

    if(m_camera->distToFocusPoint() < 65)
        m_planeGrid->setCellSize(1);
    if(m_camera->distToFocusPoint() >= 65 && m_camera->distToFocusPoint() <= 130)
        m_planeGrid->setCellSize(3);
    if(m_camera->distToFocusPoint() >= 130 && m_camera->distToFocusPoint() <= 230)
        m_planeGrid->setCellSize(5);
    if(m_camera->distToFocusPoint() > 230)
        m_planeGrid->setCellSize(10);

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::fitAllInView()
{
    m_camera->focusOnPosition(m_bounds.center());

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

    m_planeGrid->setSize(QSize(m_bounds.length(), m_bounds.width()));
    m_planeGrid->setPosition(m_bounds.bottomPos());

    QQuickFramebufferObject::update();
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
    updateViewMatrix();
}

GraphicsScene3dView::Camera::Camera(const QVector3D &offset,
                                    qreal pitch,
                                    qreal yaw,
                                    qreal distToFocusPoint,
                                    qreal fov,
                                    qreal sensivity)
    :m_offset(std::move(offset))
    ,m_pitch(std::move(pitch))
    ,m_yaw(std::move(yaw))
    ,m_fov(std::move(fov))
    ,m_distToFocusPoint(std::move(distToFocusPoint))
    ,m_sensivity(std::move(sensivity))
{
    updateViewMatrix();
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

void GraphicsScene3dView::Camera::rotate(qreal yaw, qreal pitch)
{
    QVector3D viewDir = (m_lookAt - m_eye).normalized();
    QVector3D right = QVector3D::crossProduct(viewDir, m_up);

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
    //m_eye = (rotationMatrixX * (m_eye - m_lookAt)) + m_lookAt;
    m_relativeOrbitPos = (rotationMatrixX * QVector4D(m_relativeOrbitPos, 1.0f)).toVector3D();

    QMatrix4x4 rotationMatrixY;
    rotationMatrixY.setToIdentity();
    rotationMatrixY.rotate(pitch, right);
    //m_eye = (rotationMatrixY * (m_eye - m_lookAt)) + m_lookAt;
    m_relativeOrbitPos = (rotationMatrixY * QVector4D(m_relativeOrbitPos, 1.0f)).toVector3D();

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::move(const QVector2D &startPos, const QVector2D &endPos)
{
    QMatrix4x4 inverted = m_view.inverted();

    QVector4D horizontalAxis = inverted * QVector4D(-1, 0, 0, 0);
    QVector4D verticalAxis = inverted * QVector4D(0, -1, 0, 0);

    m_deltaOffset = ((horizontalAxis * (float)(endPos.x() - startPos.x()) + verticalAxis * (float)(endPos.y() - startPos.y())) * m_sensivity * 0.05f).toVector3D();

    m_useFocusPoint = false;

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::zoom(qreal delta)
{
    m_distToFocusPoint -= delta * m_sensivity;
    m_distToFocusPoint = std::fmaxf(static_cast<float>(m_distToFocusPoint), 2.0f);

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::commitMovement()
{
    m_offset += m_deltaOffset;
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
    m_offset = QVector3D();
    m_deltaOffset = QVector3D();

    m_useFocusPoint = true;

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::reset()
{
    m_eye = {-0.45f, -0.45f, 0.75f};
    m_up = {0.0f, 1.0f, 0.0f};
    m_lookAt = {0.0f, 0.0f, 0.0f};
    m_startDragPos = {0.0f, 0.0f};

    m_focusedObject.lock() = nullptr;
    m_offset = {0.0f, 0.0f, 0.0f};
    m_deltaOffset = {0.0f, 0.0f, 0.0f};
    m_focusPoint = {0.0f, 0.0f, 0.0f};

    m_pitch = 0.0f;
    m_yaw = 0.0f;
    m_fov = 45.0f;
    m_distToFocusPoint = 32.0f;

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::updateViewMatrix()
{
    if(!m_useFocusPoint)
        m_lookAt = QVector3D() + m_offset + m_deltaOffset;

    m_eye = m_lookAt + m_relativeOrbitPos * m_distToFocusPoint;

    QMatrix4x4 view;
    //view.lookAt(m_eye*m_distToFocusPoint + m_offset + m_deltaOffset, m_lookAt + m_offset + m_deltaOffset, m_up);
    view.lookAt(m_eye, m_lookAt, m_up);
    m_view = std::move(view);
}

qreal GraphicsScene3dView::Camera::distToFocusPoint() const
{
    return m_distToFocusPoint;
}
