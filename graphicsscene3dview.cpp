#include "graphicsscene3dview.h"
#include <graphicsscene3drenderer.h>
#include <surface.h>
#include <plotcash.h>

#include <cmath>
#include <memory.h>
#include <math.h>

#include <QOpenGLFramebufferObject>
#include <QVector3D>


GraphicsScene3dView::GraphicsScene3dView() :
    QQuickFramebufferObject(),
    m_camera(std::make_shared<Camera>()),
    m_axesThumbnailCamera(std::make_shared<Camera>()),
    m_rayCaster(std::make_shared<RayCaster>()),
    m_surface(std::make_shared<Surface>()),
    sideScanView_(std::make_shared<SideScanView>()),
    m_boatTrack(std::make_shared<BoatTrack>()),
    m_bottomTrack(std::make_shared<BottomTrack>(this, this)),
    m_polygonGroup(std::make_shared<PolygonGroup>()),
    m_pointGroup(std::make_shared<PointGroup>()),
    m_coordAxes(std::make_shared<CoordinateAxes>()),
    m_planeGrid(std::make_shared<PlaneGrid>()),
    m_navigationArrow(std::make_shared<NavigationArrow>()),
    navigationArrowState_(true),
    wasMoved_(false),
    wasMovedMouseButton_(Qt::MouseButton::NoButton),
    switchedToBottomTrackVertexComboSelectionMode_(false),
    renderer_(nullptr)
{
    setObjectName("GraphicsScene3dView");
    setMirrorVertically(true);
    setAcceptedMouseButtons(Qt::AllButtons);

    m_boatTrack->setColor({80,0,180});
    m_boatTrack->setWidth(6.0f);

    m_navigationArrow->setColor({ 255, 0, 0 });

    QObject::connect(m_surface.get(), &Surface::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(sideScanView_.get(), &SideScanView::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_boatTrack.get(), &BoatTrack::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_bottomTrack.get(), &BottomTrack::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_polygonGroup.get(), &PolygonGroup::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_pointGroup.get(), &PointGroup::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_coordAxes.get(), &CoordinateAxes::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_planeGrid.get(), &PlaneGrid::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_navigationArrow.get(), &NavigationArrow::changed, this, &QQuickFramebufferObject::update);

    QObject::connect(m_surface.get(), &Surface::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(sideScanView_.get(), &SideScanView::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_bottomTrack.get(), &BottomTrack::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_polygonGroup.get(), &PolygonGroup::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_pointGroup.get(), &PointGroup::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_coordAxes.get(), &CoordinateAxes::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_boatTrack.get(), &PlaneGrid::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_navigationArrow.get(), &NavigationArrow::boundsChanged, this, &GraphicsScene3dView::updateBounds);

    updatePlaneGrid();
}

GraphicsScene3dView::~GraphicsScene3dView()
{

}

QQuickFramebufferObject::Renderer *GraphicsScene3dView::createRenderer() const
{
    renderer_ = new GraphicsScene3dView::InFboRenderer();
    return renderer_;
}

std::shared_ptr<BoatTrack> GraphicsScene3dView::boatTrack() const
{
    return m_boatTrack;
}

std::shared_ptr<BottomTrack> GraphicsScene3dView::bottomTrack() const
{
    return m_bottomTrack;
}

std::shared_ptr<Surface> GraphicsScene3dView::surface() const
{
    return m_surface;
}

std::shared_ptr<SideScanView> GraphicsScene3dView::getSideScanViewPtr() const
{
    return sideScanView_;
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
    sideScanView_->clearData();
    m_boatTrack->clearData();
    m_bottomTrack->clearData();
    m_polygonGroup->clearData();
    m_pointGroup->clearData();
    m_navigationArrow->clearData();
    navigationArrowState_ = false;
    m_bounds = Cube();

    setMapView();

    QQuickFramebufferObject::update();
}

QVector3D GraphicsScene3dView::calculateIntersectionPoint(const QVector3D &rayOrigin, const QVector3D &rayDirection, float planeZ) {
    QVector3D retVal;
    if (qAbs(rayDirection.z()) < 1e-6)
        return retVal;
    const float t = (planeZ - rayOrigin.z()) / rayDirection.z();
    if (t < 0)
        return retVal;
    retVal = rayOrigin + rayDirection * t;

    return retVal;
}

void GraphicsScene3dView::setTextureId(GLuint id)
{
    sideScanView_->setTextureId(id);
}

void GraphicsScene3dView::switchToBottomTrackVertexComboSelectionMode(qreal x, qreal y)
{
    switchedToBottomTrackVertexComboSelectionMode_ = true;

    m_bottomTrack->resetVertexSelection();
    lastMode_ = m_mode;
    m_mode = ActiveMode::BottomTrackVertexComboSelectionMode;
    m_comboSelectionRect.setTopLeft({ static_cast<int>(x), static_cast<int>(height() - y) });
    m_comboSelectionRect.setBottomRight({ static_cast<int>(x), static_cast<int>(height() - y) });

}

void GraphicsScene3dView::mousePressTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey)
{
    Q_UNUSED(keyboardKey)

    wasMoved_ = false;
    clearComboSelectionRect();

    if (engine_) { // maybe this will be removed
        if (auto selectionToolButton = engine_->findChild<QObject*>("selectionToolButton"); selectionToolButton) {
            selectionToolButton->property("checked").toBool() ? m_mode = ActiveMode::BottomTrackVertexSelectionMode : m_mode = ActiveMode::Idle;
        }
    }

    if (mouseButton == Qt::MouseButton::RightButton) {
        switchToBottomTrackVertexComboSelectionMode(x, y);
    }

    m_startMousePos = { x, y };
    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseMoveTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey)
{
    // movement threshold for sync
    if (!wasMoved_) {
        double dist{ std::sqrt(std::pow(x - m_startMousePos.x(), 2) + std::pow(y - m_startMousePos.y(), 2)) };
        if (dist > mouseThreshold_) {
            wasMoved_ = true;
            if (wasMovedMouseButton_ != mouseButton)
                wasMovedMouseButton_ = mouseButton;
        }
    }

    // ray for marker
    auto toOrig = QVector3D(x, height() - y, -1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
    auto toEnd = QVector3D(x, height() - y, 1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
    auto toDir = (toEnd - toOrig).normalized();
    auto to = calculateIntersectionPoint(toOrig, toDir, 0);
    m_ray.setOrigin(toOrig);
    m_ray.setDirection(toDir);

    if (switchedToBottomTrackVertexComboSelectionMode_) {
        m_comboSelectionRect.setBottomRight({ static_cast<int>(x), static_cast<int>(height() - y) });
        m_bottomTrack->mouseMoveEvent(mouseButton, x, y);
    }
    else {
#ifdef Q_OS_ANDROID
        Q_UNUSED(keyboardKey);
        auto fromOrig = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), -1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
        auto fromEnd = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), 1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
        auto fromDir = (fromEnd - fromOrig).normalized();
        auto from = calculateIntersectionPoint(fromOrig, fromDir , 0);
        m_camera->move(QVector2D(from.x(), from.y()), QVector2D(to.x() ,to.y()));
#else
        if (mouseButton.testFlag(Qt::LeftButton) && (keyboardKey == Qt::Key_Control)) {
            m_camera->commitMovement();
            m_camera->rotate(QVector2D(m_lastMousePos), QVector2D(x, y));
            m_axesThumbnailCamera->rotate(QVector2D(m_lastMousePos), QVector2D(x, y));
            m_startMousePos = { x, y };
        }
        else if (mouseButton.testFlag(Qt::LeftButton)) {
            auto fromOrig = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), -1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
            auto fromEnd = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), 1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
            auto fromDir = (fromEnd - fromOrig).normalized();
            auto from = calculateIntersectionPoint(fromOrig, fromDir , 0);
            m_camera->move(QVector2D(from.x(), from.y()), QVector2D(to.x() ,to.y()));
        }
#endif
    }

    m_lastMousePos = { x, y };
    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseReleaseTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey)
{
    Q_UNUSED(keyboardKey);

    clearComboSelectionRect();

    m_camera->commitMovement(); //TODO: Commit only if camera in movement state
    m_lastMousePos = { x, y };

    if (switchedToBottomTrackVertexComboSelectionMode_) {
        m_mode = lastMode_;
        m_bottomTrack->mouseReleaseEvent(mouseButton, x, y);
    }

    if (!wasMoved_ && wasMovedMouseButton_ == Qt::MouseButton::NoButton) {
        m_bottomTrack->resetVertexSelection();
        m_bottomTrack->mousePressEvent(Qt::MouseButton::LeftButton, x, y);
    }

    switchedToBottomTrackVertexComboSelectionMode_ = false;
    wasMoved_ = false;
    wasMovedMouseButton_ = Qt::MouseButton::NoButton;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseWheelTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, QPointF angleDelta, Qt::Key keyboardKey)
{
    Q_UNUSED(mouseButton)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if (keyboardKey == Qt::Key_Control) {
        float tempVerticalScale = m_verticalScale;
        angleDelta.y() > 0.0f ? tempVerticalScale += 0.3f : tempVerticalScale -= 0.3f;
        setVerticalScale(tempVerticalScale);
    }
    else if (keyboardKey == Qt::Key_Shift)
        angleDelta.y() > 0.0f ? shiftCameraZAxis(5) : shiftCameraZAxis(-5);
    else
        m_camera->zoom(angleDelta.y());

    updatePlaneGrid();
    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::pinchTrigger(const QPointF& prevCenter, const QPointF& currCenter, qreal scaleDelta, qreal angleDelta)
{
    m_camera->zoom(scaleDelta);

    m_camera->rotate(prevCenter, currCenter, angleDelta, height());
    m_axesThumbnailCamera->rotate(prevCenter, currCenter, angleDelta , height());

    updatePlaneGrid();
    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::keyPressTrigger(Qt::Key key)
{
    m_bottomTrack->keyPressEvent(key);

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::bottomTrackActionEvent(BottomTrack::ActionEvent actionEvent)
{
    m_bottomTrack->actionEvent(actionEvent);

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setTextureImage(const QImage &image, bool usingFilters) {
    if (renderer_) {
        renderer_->setTextureImage(image, usingFilters);
    }

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

    auto d = (maxSize/2.0f)/(std::tan(m_camera->fov()/2.0f)) * 2.0f;

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

void GraphicsScene3dView::setMapView() {
    m_camera->setMapView();
    m_axesThumbnailCamera->setMapView();

    fitAllInView();
    updatePlaneGrid();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setIdleMode()
{
    m_mode = Idle; 

    clearComboSelectionRect();
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

void GraphicsScene3dView::shiftCameraZAxis(float shift)
{
    m_camera->moveZAxis(shift);
}

void GraphicsScene3dView::setBottomTrackVertexSelectionMode()
{
    setIdleMode();

    m_mode = BottomTrackVertexSelectionMode;

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
    if (m_dataset)
        QObject::disconnect(m_dataset);

    m_dataset = dataset;

    if (!m_dataset)
        return;

    m_bottomTrack->setDatasetPtr(m_dataset);
    sideScanView_->setDatasetPtr(m_dataset);

    QObject::connect(m_dataset, &Dataset::bottomTrackUpdated,
                     this,      [this](int lEpoch, int rEpoch) -> void {
                                    clearComboSelectionRect();
                                    m_bottomTrack->isEpochsChanged(lEpoch, rEpoch);
                                }
                     );
    QObject::connect(m_dataset, &Dataset::boatTrackUpdated,
                     this,      [this]() -> void {
                                    m_boatTrack->setData(m_dataset->boatTrack(), GL_LINE_STRIP);
                                    if (navigationArrowState_) {
                                        const Position pos = m_dataset->getLastPosition();
                                        m_navigationArrow->setPositionAndAngle(
                                            QVector3D(pos.ned.n, pos.ned.e, !isfinite(pos.ned.d) ? 0.f : pos.ned.d), m_dataset->getLastYaw() - 90.f);
                                    }
                                }
                     );
}

void GraphicsScene3dView::addPoints(QVector<QVector3D> positions, QColor color, float width) {
    for(int i = 0; i < positions.size(); i++) {
        auto p = std::make_shared<PointObject>();
        p->setPosition(positions[i]);
        p->setColor(color);
        p->setWidth(width);
        pointGroup()->append(p);
    }
}

void GraphicsScene3dView::setQmlEngine(QObject* engine)
{
    engine_ = engine;
}

void GraphicsScene3dView::updateBounds()
{
    m_bounds = m_boatTrack->bounds()
                   .merge(m_surface->bounds())
                    .merge(m_bottomTrack->bounds())
                    .merge(m_polygonGroup->bounds())
                    .merge(m_pointGroup->bounds());

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

void GraphicsScene3dView::clearComboSelectionRect()
{
    m_comboSelectionRect = { 0, 0, 0, 0 };
}

//---------------------Renderer---------------------------//
GraphicsScene3dView::InFboRenderer::InFboRenderer()
    :QQuickFramebufferObject::Renderer()
    , m_renderer(new GraphicsScene3dRenderer)
    , textureId_(0)
    , needToInitializeTexture_(false)
    , usingFilters_(true)
{
    m_renderer->initialize();
}

GraphicsScene3dView::InFboRenderer::~InFboRenderer()
{
    if (textureId_) {
        glDeleteTextures(1, &textureId_);
    }
}

void GraphicsScene3dView::InFboRenderer::setTextureImage(const QImage &image, bool usingFilters)
{
    textureImage_ = image;
    usingFilters_ = usingFilters;

    needToInitializeTexture_ = true;
}

void GraphicsScene3dView::InFboRenderer::render()
{
    m_renderer->render();
}

void GraphicsScene3dView::InFboRenderer::synchronize(QQuickFramebufferObject * fbo)
{
    auto view = qobject_cast<GraphicsScene3dView*>(fbo);

    if (!view) {
        return;
    }

    //read from renderer
    view->m_model = m_renderer->m_model;
    view->m_projection = m_renderer->m_projection;
    if (needToInitializeTexture_ && !textureImage_.isNull()) {
        initializeTexture();
        view->setTextureId(textureId_);
        needToInitializeTexture_ = false;
    }

    // write to renderer
    m_renderer->m_coordAxesRenderImpl       = *(dynamic_cast<CoordinateAxes::CoordinateAxesRenderImplementation*>(view->m_coordAxes->m_renderImpl));
    m_renderer->m_planeGridRenderImpl       = *(dynamic_cast<PlaneGrid::PlaneGridRenderImplementation*>(view->m_planeGrid->m_renderImpl));
    m_renderer->m_boatTrackRenderImpl       = *(dynamic_cast<BoatTrack::BoatTrackRenderImplementation*>(view->m_boatTrack->m_renderImpl));
    m_renderer->m_bottomTrackRenderImpl     = *(dynamic_cast<BottomTrack::BottomTrackRenderImplementation*>(view->m_bottomTrack->m_renderImpl));
    m_renderer->m_surfaceRenderImpl         = *(dynamic_cast<Surface::SurfaceRenderImplementation*>(view->m_surface->m_renderImpl));
    m_renderer->sideScanViewRenderImpl_     = *(dynamic_cast<SideScanView::SideScanViewRenderImplementation*>(view->sideScanView_->m_renderImpl));
    m_renderer->m_polygonGroupRenderImpl    = *(dynamic_cast<PolygonGroup::PolygonGroupRenderImplementation*>(view->m_polygonGroup->m_renderImpl));
    m_renderer->m_pointGroupRenderImpl      = *(dynamic_cast<PointGroup::PointGroupRenderImplementation*>(view->m_pointGroup->m_renderImpl));
    m_renderer->navigationArrowRenderImpl_  = *(dynamic_cast<NavigationArrow::NavigationArrowRenderImplementation*>(view->m_navigationArrow->m_renderImpl));
    m_renderer->m_viewSize                  = view->size();
    m_renderer->m_camera                    = *view->m_camera;
    m_renderer->m_axesThumbnailCamera       = *view->m_axesThumbnailCamera;
    m_renderer->m_comboSelectionRect        = view->m_comboSelectionRect;
    m_renderer->m_verticalScale             = view->m_verticalScale;
    m_renderer->m_boundingBox               = view->m_bounds;
    m_renderer->m_isSceneBoundingBoxVisible = view->m_isSceneBoundingBoxVisible;
}

QOpenGLFramebufferObject *GraphicsScene3dView::InFboRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

void GraphicsScene3dView::InFboRenderer::initializeTexture()
{
    if (!usingFilters_) { // without mipmap
        if (textureId_) {
            glDeleteTextures(1, &textureId_);
        }

        glGenTextures(1, &textureId_);
        glBindTexture(GL_TEXTURE_2D, textureId_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        QImage glImage = textureImage_.convertToFormat(QImage::Format_RGBA8888);
        QTransform transform;
        transform.rotate(90);
        QImage rotatedImage = glImage.transformed(transform);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rotatedImage.width(), rotatedImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, rotatedImage.bits());
    }
    else {
        QOpenGLContext* context = QOpenGLContext::currentContext();
        QOpenGLFunctions* functions = context->functions();

        if (textureId_) {
            functions->glDeleteTextures(1, &textureId_);
        }

        functions->glGenTextures(1, &textureId_);
        functions->glBindTexture(GL_TEXTURE_2D, textureId_);

        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        QImage glImage = textureImage_.convertToFormat(QImage::Format_RGBA8888);
        QTransform transform;
        transform.rotate(90);
        QImage rotatedImage = glImage.transformed(transform);

        functions->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rotatedImage.width(), rotatedImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, rotatedImage.bits());

        functions->glGenerateMipmap(GL_TEXTURE_2D);
    }
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

    checkRotateAngle();

    updateViewMatrix();
}

void GraphicsScene3dView::Camera::rotate(const QPointF& prevCenter, const QPointF& currCenter, qreal angleDelta, qreal widgetHeight)
{
    const qreal increaseCoeff{ 1.3 };
    const qreal angleDeltaY = (prevCenter - currCenter).y() / widgetHeight * 90.0;

    m_rotAngle.setX(m_rotAngle.x() - qDegreesToRadians(angleDelta));
    m_rotAngle.setY(m_rotAngle.y() + qDegreesToRadians(angleDeltaY * increaseCoeff));

    checkRotateAngle();
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

void GraphicsScene3dView::Camera::moveZAxis(float z)
{
    m_lookAt.setZ(m_lookAt.z() + z);
    updateViewMatrix();
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
#ifdef Q_OS_ANDROID
    const float increaseCoeff{ 0.95f };
    m_distToFocusPoint -= delta * m_distToFocusPoint * increaseCoeff;
#else
    m_distToFocusPoint = delta > 0.f ? m_distToFocusPoint / 1.15f : m_distToFocusPoint * 1.15f;
#endif

    const float minFocusDist = 2.0f;
    const float maxFocusDist = 10000.0f;
    if (m_distToFocusPoint < minFocusDist)
        m_distToFocusPoint = minFocusDist;
    if (m_distToFocusPoint >= maxFocusDist)
        m_distToFocusPoint = maxFocusDist;

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

void GraphicsScene3dView::Camera::setMapView() {
    reset();

    m_rotAngle.setX(qDegreesToRadians(0.0f));
    m_rotAngle.setY(qDegreesToRadians(0.0f));

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

void GraphicsScene3dView::Camera::checkRotateAngle()
{
    if (m_rotAngle[1] > M_PI_2)
        m_rotAngle[1] = M_PI_2;
    else if (m_rotAngle[1] < 0)
        m_rotAngle[1] = 0;
}


qreal GraphicsScene3dView::Camera::distToFocusPoint() const
{
    return m_distToFocusPoint;
}
