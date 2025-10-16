#include "scene3d_view.h"
#include <cmath>
#include <memory.h>
#include <math.h>
#include <QOpenGLFramebufferObject>
#include <QVector3D>
#include "scene3d_renderer.h"
#include "dataset.h"
#include "map_defs.h"


GraphicsScene3dView::GraphicsScene3dView() :
    QQuickFramebufferObject(),
    m_camera(std::make_shared<Camera>(this)),
    m_axesThumbnailCamera(std::make_shared<Camera>()),
    m_rayCaster(std::make_shared<RayCaster>()),
    isobathsView_(std::make_shared<IsobathsView>()),
    surfaceView_(std::make_shared<SurfaceView>()),
    imageView_(std::make_shared<ImageView>()),
    mapView_(std::make_shared<MapView>(this)),
    contacts_(std::make_shared<Contacts>(this)),
    boatTrack_(std::make_shared<BoatTrack>(this, this)),
    m_bottomTrack(std::make_shared<BottomTrack>(this, this)),
    m_polygonGroup(std::make_shared<PolygonGroup>()),
    m_pointGroup(std::make_shared<PointGroup>()),
    m_coordAxes(std::make_shared<CoordinateAxes>()),
    m_planeGrid(std::make_shared<PlaneGrid>()),
    navigationArrow_(std::make_shared<NavigationArrow>()),
    usblView_(std::make_shared<UsblView>()),
    wasMoved_(false),
    wasMovedMouseButton_(Qt::MouseButton::NoButton),
    switchedToBottomTrackVertexComboSelectionMode_(false),
    needToResetStartPos_(false),
    lastCameraDist_(m_camera->distForMapView()),
    trackLastData_(false),
    gridVisibility_(true),
    useAngleLocation_(false),
    navigatorViewLocation_(false)
{
    setObjectName("GraphicsScene3dView");
    setMirrorVertically(true);
    setAcceptedMouseButtons(Qt::AllButtons);

    m_camera->setCameraListener(m_axesThumbnailCamera.get());

    boatTrack_->setColor({80,0,180});
    boatTrack_->setWidth(6.0f);

    imageView_->setView(this);

    QObject::connect(isobathsView_.get(), &IsobathsView::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(surfaceView_.get(), &SurfaceView::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(imageView_.get(), &ImageView::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(mapView_.get(), &MapView::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(contacts_.get(), &Contacts::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(boatTrack_.get(), &BoatTrack::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_bottomTrack.get(), &BottomTrack::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_polygonGroup.get(), &PolygonGroup::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_pointGroup.get(), &PointGroup::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_coordAxes.get(), &CoordinateAxes::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(m_planeGrid.get(), &PlaneGrid::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(navigationArrow_.get(), &NavigationArrow::changed, this, &QQuickFramebufferObject::update);
    QObject::connect(usblView_.get(), &UsblView::changed, this, &QQuickFramebufferObject::update);

    QObject::connect(isobathsView_.get(), &IsobathsView::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(surfaceView_.get(), &SurfaceView::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(imageView_.get(), &ImageView::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(mapView_.get(), &MapView::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(contacts_.get(), &Contacts::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_bottomTrack.get(), &BottomTrack::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_polygonGroup.get(), &PolygonGroup::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_pointGroup.get(), &PointGroup::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(m_coordAxes.get(), &CoordinateAxes::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(boatTrack_.get(), &PlaneGrid::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(navigationArrow_.get(), &NavigationArrow::boundsChanged, this, &GraphicsScene3dView::updateBounds);
    QObject::connect(usblView_.get(), &UsblView::boundsChanged, this, &GraphicsScene3dView::updateBounds);

    QObject::connect(this, &GraphicsScene3dView::cameraIsMoved, this, &GraphicsScene3dView::updateMapView, Qt::DirectConnection);
    QObject::connect(this, &GraphicsScene3dView::cameraIsMoved, this, &GraphicsScene3dView::updateViews, Qt::DirectConnection);

    updatePlaneGrid();
}

GraphicsScene3dView::~GraphicsScene3dView()
{

}

QQuickFramebufferObject::Renderer *GraphicsScene3dView::createRenderer() const
{
    return new GraphicsScene3dView::InFboRenderer();
}

std::shared_ptr<BoatTrack> GraphicsScene3dView::getBoatTrackPtr() const
{
    return boatTrack_;
}

std::shared_ptr<BottomTrack> GraphicsScene3dView::bottomTrack() const
{
    return m_bottomTrack;
}

std::shared_ptr<IsobathsView> GraphicsScene3dView::getIsobathsViewPtr() const
{
    return isobathsView_;
}

std::shared_ptr<SurfaceView> GraphicsScene3dView::getSurfaceViewPtr() const
{
    return surfaceView_;
}

std::shared_ptr<ImageView> GraphicsScene3dView::getImageViewPtr() const
{
    return imageView_;
}

std::shared_ptr<MapView> GraphicsScene3dView::getMapViewPtr() const
{
    return mapView_;
}

std::shared_ptr<Contacts> GraphicsScene3dView::getContactsPtr() const
{
    return contacts_;
}

std::shared_ptr<PointGroup> GraphicsScene3dView::pointGroup() const
{
    return m_pointGroup;
}

std::shared_ptr<PolygonGroup> GraphicsScene3dView::polygonGroup() const
{
    return m_polygonGroup;
}

std::shared_ptr<UsblView> GraphicsScene3dView::getUsblViewPtr() const
{
    return usblView_;
}

std::shared_ptr<NavigationArrow> GraphicsScene3dView::getNavigationArrowPtr() const
{
    return navigationArrow_;
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
    return datasetPtr_;
}

void GraphicsScene3dView::clear(bool cleanMap)
{
    isobathsView_->clear();
    surfaceView_->clear();
    contacts_->clear();
    imageView_->clear();//
    if (cleanMap) {
        mapView_->clear();
    }
    boatTrack_->clearData();
    m_bottomTrack->clearData();
    m_polygonGroup->clearData();
    m_pointGroup->clearData();
    navigationArrow_->clearData();
    usblView_->clearTracks();
    m_bounds = Cube();

    //setMapView();
    updateBounds();

    QQuickFramebufferObject::update();
}

QVector3D GraphicsScene3dView::calculateIntersectionPoint(const QVector3D &rayOrigin, const QVector3D &rayDirection, float planeZ)
{
    QVector3D retVal;

    if (qAbs(rayDirection.z()) < 1e-6) {
        return retVal;
    }
    const float t = (planeZ - rayOrigin.z()) / rayDirection.z();

    if (t < 0) {
        return retVal;
    }
    retVal = rayOrigin + rayDirection * t;

    return retVal;
}

void GraphicsScene3dView::switchToBottomTrackVertexComboSelectionMode(qreal x, qreal y)
{
    switchedToBottomTrackVertexComboSelectionMode_ = true;

    m_bottomTrack->resetVertexSelection();
    boatTrack_->clearSelectedEpoch();
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

    if (qmlRootObject_) { // maybe this will be removed
        if (auto selectionToolButton = qmlRootObject_->findChild<QObject*>("selectionToolButton"); selectionToolButton) {
            selectionToolButton->property("checked").toBool() ? m_mode = ActiveMode::BottomTrackVertexSelectionMode : m_mode = ActiveMode::Idle;
        }
    }

    if (mouseButton == Qt::MouseButton::RightButton) {
        switchToBottomTrackVertexComboSelectionMode(x, y);
    }

    m_camera->m_lookAtSave = m_camera->m_lookAt;

    m_startMousePos = { x, y };
    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseMoveTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey)
{
    bool cameraWasMoved{ false };
    if (needToResetStartPos_) {
        m_camera->m_lookAtSave = m_camera->m_lookAt;
        m_startMousePos = QPointF(x, y);
        needToResetStartPos_ = false;
    }

    contacts_->mouseMoveEvent(mouseButton, x, y);

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
#if defined(Q_OS_ANDROID)
        Q_UNUSED(keyboardKey);
        auto fromOrig = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), -1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
        auto fromEnd = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), 1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
        auto fromDir = (fromEnd - fromOrig).normalized();
        auto from = calculateIntersectionPoint(fromOrig, fromDir , 0);
        m_camera->move(QVector2D(from.x(), from.y()), QVector2D(to.x() ,to.y()));
        cameraWasMoved = true;
#else
        if (mouseButton.testFlag(Qt::LeftButton) && (keyboardKey == Qt::Key_Control)) {
            if (m_camera->getIsPerspective() && !isNorth_) {
                m_camera->rotate(QVector2D(m_lastMousePos), QVector2D(x, y));
                m_axesThumbnailCamera->rotate(QVector2D(m_lastMousePos), QVector2D(x, y));
                m_startMousePos = { x, y };
                cameraWasMoved = true;
            }
        }
        else if (mouseButton.testFlag(Qt::LeftButton)) {
            auto fromOrig = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), -1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
            auto fromEnd = QVector3D(m_startMousePos.x(), height() - m_startMousePos.y(), 1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
            auto fromDir = (fromEnd - fromOrig).normalized();
            auto from = calculateIntersectionPoint(fromOrig, fromDir , 0);
            m_camera->move(QVector2D(from.x(), from.y()), QVector2D(to.x() ,to.y()));
            cameraWasMoved = true;
        }
#endif
    }

    m_lastMousePos = { x, y };
    QQuickFramebufferObject::update();

    if (cameraWasMoved) {
        emit cameraIsMoved();
    }
}

void GraphicsScene3dView::mouseReleaseTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, Qt::Key keyboardKey)
{
    Q_UNUSED(keyboardKey);

    clearComboSelectionRect();

    m_lastMousePos = { x, y };

    if (switchedToBottomTrackVertexComboSelectionMode_) {
        m_mode = lastMode_;
        m_bottomTrack->mouseReleaseEvent(mouseButton, x, y);
    }

    if (!wasMoved_ && wasMovedMouseButton_ == Qt::MouseButton::NoButton) {
        m_bottomTrack->resetVertexSelection();
        boatTrack_->clearSelectedEpoch();
        m_bottomTrack->mousePressEvent(Qt::MouseButton::LeftButton, x, y);
        boatTrack_->mousePressEvent(Qt::MouseButton::LeftButton, x, y);
    }

    switchedToBottomTrackVertexComboSelectionMode_ = false;
    wasMoved_ = false;
    wasMovedMouseButton_ = Qt::MouseButton::NoButton;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::mouseWheelTrigger(Qt::MouseButtons mouseButton, qreal x, qreal y, QPointF angleDelta, Qt::Key keyboardKey)
{
    bool cameraWasMoved{ false };
    Q_UNUSED(mouseButton)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if (needToResetStartPos_) {
        m_camera->m_lookAtSave = m_camera->m_lookAt;
        m_startMousePos = QPointF(x, y);
        needToResetStartPos_ = false;
    }

    if (keyboardKey == Qt::Key_Control) {
        float tempVerticalScale = m_verticalScale;
        angleDelta.y() > 0.0f ? tempVerticalScale += 0.3f : tempVerticalScale -= 0.3f;
        setVerticalScale(tempVerticalScale);
    }
    else if (keyboardKey == Qt::Key_Shift) {
        if (!isNorth_) {
            angleDelta.y() > 0.0f ? shiftCameraZAxis(5) : shiftCameraZAxis(-5);
            cameraWasMoved = true;
        }
    }
    else {
        m_camera->zoom(angleDelta.y());
        cameraWasMoved = true;
    }

    updatePlaneGrid();
    QQuickFramebufferObject::update();

    if (cameraWasMoved) {
        emit cameraIsMoved();
    }
}

void GraphicsScene3dView::pinchTrigger(const QPointF& prevCenter, const QPointF& currCenter, qreal scaleDelta, qreal angleDelta)
{
    m_camera->zoom(scaleDelta);

    if (!isNorth_) {
        m_camera->rotate(prevCenter, currCenter, angleDelta, height());
        m_axesThumbnailCamera->rotate(prevCenter, currCenter, angleDelta , height());
    }

    updatePlaneGrid();
    QQuickFramebufferObject::update();

    emit cameraIsMoved();
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

void GraphicsScene3dView::setTrackLastData(bool state)
{
    trackLastData_ = state;
}

void GraphicsScene3dView::setTextureIdByTileIndx(const map::TileIndex &tileIndx, GLuint textureId)
{
    emit sendMapTextureIdByTileIndx(tileIndx, textureId);
}

void GraphicsScene3dView::setGridVisibility(bool state)
{
    gridVisibility_ = state;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setUseAngleLocation(bool state)
{
    useAngleLocation_ = state;
}

void GraphicsScene3dView::setNavigatorViewLocation(bool state)
{
    navigatorViewLocation_ = state;
}

void GraphicsScene3dView::updateProjection()
{
    QMatrix4x4 currProj;

    if (m_camera) {
        if (m_camera->getIsPerspective()) {
            float coeff = m_camera->getHeightAboveGround() / perspectiveEdge_;
            qreal fixFov = m_camera->fov() + m_camera->fov() * coeff;
            currProj.perspective(fixFov, static_cast<float>(width() / height()), nearPlanePersp_, farPlanePersp_);
        }
        else {
            float orthV = m_camera->getHeightAboveGround();
            float aspectRatio = width() / height();
            currProj.ortho(-orthV * aspectRatio, orthV * aspectRatio, -orthV, orthV, orthV * nearPlaneOrthoCoeff_, orthV * farPlaneOrthoCoeff_);
        }

        m_projection = std::move(currProj);
    }
}

void GraphicsScene3dView::setNeedToResetStartPos(bool state)
{
    needToResetStartPos_ = state;
}

void GraphicsScene3dView::forceUpdateDatasetLlaRef()
{
    if (datasetPtr_) {
        auto ref = datasetPtr_->getLlaRef();
        m_camera->datasetLlaRef_ = ref.isInit ? ref : LLARef(m_camera->yerevanLla);
    }

    m_camera->viewLlaRef_ = m_camera->datasetLlaRef_;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickFramebufferObject::geometryChanged(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size()) {
       updateProjection();
       emit cameraIsMoved();
    }
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

    emit cameraIsMoved();
}

void GraphicsScene3dView::setIsometricView()
{
    m_camera->setIsometricView();
    m_axesThumbnailCamera->setIsometricView();

    fitAllInView();
    updatePlaneGrid();

    QQuickFramebufferObject::update();

    emit cameraIsMoved();
}

void GraphicsScene3dView::setCancelZoomView()
{
    m_verticalScale = 1.0f;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setMapView() {
    auto datasetLlaRef = datasetPtr_->getLlaRef();
    if (datasetLlaRef.isInit) {
        m_camera->viewLlaRef_ = datasetPtr_->getLlaRef();
    }
    else {
        m_camera->viewLlaRef_ = m_camera->yerevanLla;
    }

    m_camera->setMapView();
    m_axesThumbnailCamera->setMapView();

    fitAllInView();
    updatePlaneGrid();

    QQuickFramebufferObject::update();

    emit cameraIsMoved();
}

void GraphicsScene3dView::setLastEpochFocusView(bool useAngle, bool useNavigatorView)
{
    if (!m_camera->isPerspective_ || !datasetPtr_) {
        return;
    }

    auto* epoch = datasetPtr_->last();
    if (!epoch) {
        return;
    }

    if (useAngle && !isNorth_) {
        const float yawDeg = datasetPtr_->getLastYaw();
        if (std::isfinite(yawDeg)) {
            const float targetYaw = -yawDeg * static_cast<float>(M_PI) / 180.0f;

            if (!m_camera->navYawInited_) {
                m_camera->navYawFilteredRad_ = targetYaw;
                m_camera->navYawInited_ = true;
                m_camera->navYawTmr_.restart();
            }

            double dt = m_camera->navYawTmr_.restart() / 1000.0;
            if (dt <= 0.0 || dt > 0.5) {
                dt = 0.016; // 60hz
            }

            float diff = shortestDiff(m_camera->navYawFilteredRad_, targetYaw); // разница по кратчайшей дуге + deadband
            if (std::fabs(diff) < m_camera->navYawDeadbandRad_) {
                diff = 0.f;
            }


            if (std::fabs(diff) > m_camera->navYawSnapRad_) {
                m_camera->navYawFilteredRad_ = targetYaw;
            }
            else {
                const float alpha = 1.0f - std::exp(-float(dt) / m_camera->navYawTauSec_); // time-based EMA
                float step = diff * alpha;

                const float maxStep = m_camera->navYawMaxRateRadPerSec_ * float(dt); // ограничение скорости поворота
                if (step >  maxStep) {
                    step =  maxStep;
                }
                if (step < -maxStep) {
                    step = -maxStep;
                }

                m_camera->navYawFilteredRad_ = wrapPi(m_camera->navYawFilteredRad_ + step);
            }

            m_camera->m_rotAngle.setX(m_camera->navYawFilteredRad_);
        }
    }

    // pos
    NED posNed = epoch->getPositionGNSS().ned;
    QVector3D currPos(posNed.n, posNed.e, 1);
    if (currPos == QVector3D()) {
        return;
    }

    QVector3D focusPoint = currPos;


    if (useNavigatorView) {
        // смещение
        const float yawRadForDir = -m_camera->m_rotAngle.x();
        QVector3D forwardXY(std::cos(yawRadForDir), std::sin(yawRadForDir), 0.0f);
        if (!forwardXY.isNull()) {
            forwardXY.normalize();
        }
        const float dist = std::max(1.0f, static_cast<float>(m_camera->distForMapView()));
        const float kMin      = 10.0f;
        const float kFrac     = 0.30f;
        const float kMaxFrac  = 0.85f;
        float offset = std::max(kMin, dist * kFrac);
        offset = std::min(offset, dist * kMaxFrac);
        focusPoint += forwardXY * offset;

        // тангаж
        const float targetPitchRad = isNorth_ ? 0.0f : qDegreesToRadians(30.0f);
        const float alpha = 0.3f;
        const float curr = m_camera->m_rotAngle.y();
        float next = curr + (targetPitchRad - curr) * alpha;
        if (next < 0.0f) {
            next = 0.0f;
        }
        if (next > float(M_PI_2)) {
            next = float(M_PI_2);
        }
        m_camera->m_rotAngle.setY(next);
    }

    m_camera->focusOnPosition(focusPoint);
    updatePlaneGrid();
    QQuickFramebufferObject::update();
    emit cameraIsMoved();
}

void GraphicsScene3dView::setIdleMode()
{
    m_mode = Idle;

    clearComboSelectionRect();
    m_bottomTrack->resetVertexSelection();
    boatTrack_->clearSelectedEpoch();

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::setVerticalScale(float scale)
{
    if(m_verticalScale == scale)
        return;
    else if(scale < 0.05f)
        m_verticalScale = 0.05f;
    else if(scale > 10.f)
        m_verticalScale = 10.0f;
    else
        m_verticalScale = scale;

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::shiftCameraZAxis(float shift)
{
    m_camera->moveZAxis(shift);

    emit cameraIsMoved();
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
    if (!dataset) {
        return;
    }

    if (datasetPtr_) {
        QObject::disconnect(datasetPtr_);
    }

    datasetPtr_ = dataset;

    boatTrack_->setDatasetPtr(datasetPtr_);
    m_bottomTrack->setDatasetPtr(datasetPtr_);
    contacts_->setDatasetPtr(datasetPtr_);

    forceUpdateDatasetLlaRef();

    QObject::connect(datasetPtr_, &Dataset::bottomTrackUpdated,
                     this,      [this](const ChannelId& channelId, int lEpoch, int rEpoch, bool manual) -> void {
                         auto chList = datasetPtr_->channelsList();
                         if (!datasetPtr_ || chList.empty() || chList.first().channelId_ != channelId) {
                             return;
                         }
                         clearComboSelectionRect();
                         m_bottomTrack->isEpochsChanged(lEpoch, rEpoch, manual);

                     }, Qt::DirectConnection);

    QObject::connect(datasetPtr_, &Dataset::updatedLlaRef,
                     this,      [this]() -> void {
                         surfaceView_->setLlaRef(datasetPtr_->getLlaRef());
                         forceUpdateDatasetLlaRef();
                         fitAllInView();
                     }, Qt::DirectConnection);
}

void GraphicsScene3dView::setDataProcessorPtr(DataProcessor *dataProcessorPtr)
{
    dataProcessorPtr_ = dataProcessorPtr;

    m_bottomTrack->setDataProcessorPtr(dataProcessorPtr_);
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

void GraphicsScene3dView::setQmlRootObject(QObject* object)
{
    qmlRootObject_ = object;
}

void GraphicsScene3dView::setQmlAppEngine(QQmlApplicationEngine* engine)
{
    Contacts::setQmlInstance(contacts_.get(), engine);
}

void GraphicsScene3dView::updateBounds()
{
    m_bounds = boatTrack_->bounds()
                   .merge(isobathsView_->bounds())
                   .merge(m_bottomTrack->bounds())
                   .merge(boatTrack_->bounds())
                   .merge(m_polygonGroup->bounds())
                   .merge(m_pointGroup->bounds())
                   .merge(surfaceView_->bounds())
                   .merge(imageView_->bounds())
                   .merge(usblView_->bounds());

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

void GraphicsScene3dView::updateMapView()
{
    if (!m_camera || !mapView_) {
        return;
    }

    if (!mapView_->isVisible()) {
        return;
    }

    float reductorFactor = -0.05f; // debug
    QVector<QPair<float, float>> cornerMultipliers = {
        {       reductorFactor,         reductorFactor }, // lt
        {       reductorFactor,  1.0f - reductorFactor }, // lb
        {1.0f - reductorFactor , 1.0f - reductorFactor }, // rb
        {1.0f - reductorFactor ,        reductorFactor }  // rt
    };

    updateProjection();

    // calc ned
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    bool allPointsAreValid{ true };
    for (const auto& multiplier : cornerMultipliers) {
        float currWidth  = width() * multiplier.first;
        float currHeight = height() * multiplier.second;

        QVector3D point;
        if (m_camera->getIsPerspective()) {
            auto toOrigin = QVector3D(currWidth, currHeight, -1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
            auto toEnd = QVector3D(currWidth, currHeight,  1.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
            auto toDist = (toEnd - toOrigin).normalized();
            point = calculateIntersectionPoint(toOrigin, toDist, 0);
        }
        else {
            point = QVector3D(currWidth, currHeight, 0.0f).unproject(m_camera->m_view * m_model, m_projection, boundingRect().toRect());
        }

        if (point == QVector3D()) {
            allPointsAreValid = false;
            break;
        }

        minX = std::min(minX, point.x());
        minY = std::min(minY, point.y());
        maxX = std::max(maxX, point.x());
        maxY = std::max(maxY, point.y());
    }

    if (allPointsAreValid) {
        bool canRequest{ true };
        if (m_camera->getAngleToGround() > 5.0f) {
            const float maxSideSize = 14000.f;
            float maxS = std::pow(maxSideSize, 2.0f);
            float rectArea = std::fabs(maxX - minX) * std::fabs(maxY - minY);
            if (rectArea > maxS) { // TODO: using Z coeff
                canRequest = false;
            }
        }

        QVector<LLA> llaVerts;

        float dist = m_camera->distForMapView();
        bool moveUp = dist > lastCameraDist_;
        lastCameraDist_ = dist;

        NED ltNed(minX, minY, 0.0);
        NED lbNed(minX, maxY, 0.0);
        NED rbNed(maxX, maxY, 0.0);
        NED rtNed(maxX, minY, 0.0);
        LLA ltLla(&ltNed, &m_camera->viewLlaRef_, m_camera->getIsPerspective());
        LLA lbLla(&lbNed, &m_camera->viewLlaRef_, m_camera->getIsPerspective());
        LLA rbLla(&rbNed, &m_camera->viewLlaRef_, m_camera->getIsPerspective());
        LLA rtLla(&rtNed, &m_camera->viewLlaRef_, m_camera->getIsPerspective());

        ltLla.latitude = map::clampLatitude(ltLla.latitude);
        lbLla.latitude = map::clampLatitude(lbLla.latitude);
        rbLla.latitude = map::clampLatitude(rbLla.latitude);
        rtLla.latitude = map::clampLatitude(rtLla.latitude);

        double edge = 180.0;
        if (ltLla.longitude >  edge) ltLla.longitude =  edge;
        if (ltLla.longitude < -edge) ltLla.longitude = -edge;
        if (lbLla.longitude >  edge) lbLla.longitude =  edge;
        if (lbLla.longitude < -edge) lbLla.longitude = -edge;
        if (rbLla.longitude >  edge) rbLla.longitude =  edge;
        if (rbLla.longitude < -edge) rbLla.longitude = -edge;
        if (rtLla.longitude >  edge) rtLla.longitude =  edge;
        if (rtLla.longitude < -edge) rtLla.longitude = -edge;

        llaVerts.append(LLA(ltLla.latitude, ltLla.longitude, dist));
        llaVerts.append(LLA(lbLla.latitude, lbLla.longitude, dist));
        llaVerts.append(LLA(rbLla.latitude, rbLla.longitude, dist));
        llaVerts.append(LLA(rtLla.latitude, rtLla.longitude, dist));

        if (canRequest) {
            emit sendRectRequest(llaVerts, m_camera->getIsPerspective(), m_camera->viewLlaRef_, moveUp, m_camera->getCameraTilt());
        }
        else {
            emit sendLlaRef(m_camera->viewLlaRef_);
        }
    } // is rect
    else {
        emit sendLlaRef(m_camera->viewLlaRef_);
    }

    QQuickFramebufferObject::update();
}

void GraphicsScene3dView::updateViews()
{
    if (isobathsView_) {
        isobathsView_->setCameraDistToFocusPoint(m_camera->distForMapView());
    }
}

void GraphicsScene3dView::onPositionAdded(uint64_t indx)
{
    if (!datasetPtr_) {
        return;
    }

    auto* epPtr = datasetPtr_->fromIndex(indx);
    if (!epPtr) {
        return;
    }

    const Position pos = epPtr->getPositionGNSS();
    if (!pos.ned.isCoordinatesValid()) {
        return;
    }

    boatTrack_->onPositionAdded(indx);

    if (float lastYaw = datasetPtr_->getLastYaw(); std::isfinite(lastYaw)) {
        navigationArrow_->setPositionAndAngle(QVector3D(pos.ned.n, pos.ned.e, !isfinite(pos.ned.d) ? 0.f : pos.ned.d), lastYaw - 90.f);
    }

    if (trackLastData_) {
        setLastEpochFocusView(useAngleLocation_, navigatorViewLocation_);
    }
}

void GraphicsScene3dView::setIsNorth(bool state)
{
    if (isNorth_ == state) {
        return;
    }

    isNorth_ = state;

    if (isNorth_ && m_camera && m_camera->getIsPerspective()) {
        m_camera->resetRotationAngle();
        if (m_axesThumbnailCamera) {
            m_axesThumbnailCamera->resetRotationAngle();
        }

        m_camera->resetZAxis();
        updateProjection();
    }

    QQuickFramebufferObject::update();
    emit cameraIsMoved();
}

//---------------------Renderer---------------------------//
GraphicsScene3dView::InFboRenderer::InFboRenderer() :
    QQuickFramebufferObject::Renderer(),
    m_renderer(new GraphicsScene3dRenderer)
{
    m_renderer->initialize();
}

GraphicsScene3dView::InFboRenderer::~InFboRenderer()
{ }

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

    // process textures
    processMapTextures(view);
    processMosaicColorTableTexture(view);
    processMosaicTileTexture(view);
    processImageTexture(view);
    processSurfaceTexture(view);

    //read from renderer
    view->m_model = m_renderer->m_model;
    view->m_projection = m_renderer->m_projection;
    view->contacts_->contactBounds_ = std::move(m_renderer->contactsRenderImpl_.contactBounds_);

    // write to renderer
    m_renderer->m_coordAxesRenderImpl       = *(dynamic_cast<CoordinateAxes::CoordinateAxesRenderImplementation*>(view->m_coordAxes->m_renderImpl));
    m_renderer->m_planeGridRenderImpl       = *(dynamic_cast<PlaneGrid::PlaneGridRenderImplementation*>(view->m_planeGrid->m_renderImpl));
    m_renderer->m_boatTrackRenderImpl       = *(dynamic_cast<BoatTrack::BoatTrackRenderImplementation*>(view->boatTrack_->m_renderImpl));
    m_renderer->m_bottomTrackRenderImpl     = *(dynamic_cast<BottomTrack::BottomTrackRenderImplementation*>(view->m_bottomTrack->m_renderImpl));
    m_renderer->isobathsViewRenderImpl_     = *(dynamic_cast<IsobathsView::IsobathsViewRenderImplementation*>(view->isobathsView_->m_renderImpl));
    m_renderer->surfaceViewRenderImpl_      = *(dynamic_cast<SurfaceView::SurfaceViewRenderImplementation*>(view->surfaceView_->m_renderImpl));
    m_renderer->imageViewRenderImpl_        = *(dynamic_cast<ImageView::ImageViewRenderImplementation*>(view->imageView_->m_renderImpl));
    m_renderer->contactsRenderImpl_         = *(dynamic_cast<Contacts::ContactsRenderImplementation*>(view->contacts_->m_renderImpl));
    m_renderer->m_polygonGroupRenderImpl    = *(dynamic_cast<PolygonGroup::PolygonGroupRenderImplementation*>(view->m_polygonGroup->m_renderImpl));
    m_renderer->m_pointGroupRenderImpl      = *(dynamic_cast<PointGroup::PointGroupRenderImplementation*>(view->m_pointGroup->m_renderImpl));
    m_renderer->navigationArrowRenderImpl_  = *(dynamic_cast<NavigationArrow::NavigationArrowRenderImplementation*>(view->navigationArrow_->m_renderImpl));
    m_renderer->usblViewRenderImpl_         = *(dynamic_cast<UsblView::UsblViewRenderImplementation*>(view->usblView_->m_renderImpl));
    m_renderer->m_viewSize                  = view->size();
    m_renderer->m_camera                    = *view->m_camera;
    m_renderer->m_axesThumbnailCamera       = *view->m_axesThumbnailCamera;
    m_renderer->m_comboSelectionRect        = view->m_comboSelectionRect;
    m_renderer->m_verticalScale             = view->m_verticalScale;
    m_renderer->m_boundingBox               = view->m_bounds;
    m_renderer->m_isSceneBoundingBoxVisible = view->m_isSceneBoundingBoxVisible;
    m_renderer->gridVisibility_             = view->gridVisibility_;
}

QOpenGLFramebufferObject *GraphicsScene3dView::InFboRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

void GraphicsScene3dView::InFboRenderer::processMapTextures(GraphicsScene3dView *viewPtr) const
{
    auto& r = m_renderer->mapViewRenderImpl_;

    auto* src = dynamic_cast<MapView::MapViewRenderImplementation*>(viewPtr->mapView_->m_renderImpl);
    r.copyCpuSideFrom(*src);

    auto init = viewPtr->mapView_->takeInitTileTasks();
    r.pendingInit_.reserve(r.pendingInit_.size() + init.size());
    for (auto& itm : init) {
        r.pendingInit_.push_back({ itm.first, std::move(itm.second) });
    }
    auto upd = viewPtr->mapView_->takeUpdateTileTasks();
    r.pendingUpdate_.reserve(r.pendingUpdate_.size() + upd.size());
    for (auto& itm : upd) {
        r.pendingUpdate_.push_back({ itm.first, std::move(itm.second) });
    }
    auto del = viewPtr->mapView_->takeDeleteTileTasks();
    r.pendingDelete_ += del;
}

void GraphicsScene3dView::InFboRenderer::processMosaicColorTableTexture(GraphicsScene3dView* viewPtr) const
{
    auto surfacePtr = viewPtr->getSurfaceViewPtr();

    // del
    if (auto cTTDId = surfacePtr->takeMosaicColorTableToDelete(); cTTDId) {
        surfacePtr->setMosaicColorTableTextureId(0);
        glDeleteTextures(1, &cTTDId);
    }

    auto task = surfacePtr->takeMosaicColorTableToAppend();
    if (task.empty()) {
        return;
    }

    GLuint colorTableTextureId = surfacePtr->getMosaicColorTableTextureId();

#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    if (colorTableTextureId) {
        glBindTexture(GL_TEXTURE_2D, colorTableTextureId);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, task.size() / 4, 1, GL_RGBA, GL_UNSIGNED_BYTE, task.data());
    }
    else {
        glGenTextures(1, &colorTableTextureId);
        glBindTexture(GL_TEXTURE_2D, colorTableTextureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, task.size() / 4, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, task.data());

        surfacePtr->setMosaicColorTableTextureId(colorTableTextureId);
    }
#else
    if (colorTableTextureId) {
        glBindTexture(GL_TEXTURE_1D, colorTableTextureId);
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, task.size() / 4, GL_RGBA, GL_UNSIGNED_BYTE, task.data());
    }
    else {
        glGenTextures(1, &colorTableTextureId);
        glBindTexture(GL_TEXTURE_1D, colorTableTextureId);

        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, task.size() / 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, task.data());

        surfacePtr->setMosaicColorTableTextureId(colorTableTextureId);
    }
#endif
}

void GraphicsScene3dView::InFboRenderer::processMosaicTileTexture(GraphicsScene3dView* viewPtr) const // TODO CHECK
{
    auto surfacePtr = viewPtr->getSurfaceViewPtr();

    // delete
    {
        auto tasks = surfacePtr->takeMosaicTileTextureToDelete();
        for (auto it = tasks.begin(); it != tasks.end(); ++it) {
            if (*it != 0) {
                glDeleteTextures(1, &(*it));
            }
        }
    }

    // append or update
    {
        auto tasks = surfacePtr->takeMosaicTileTextureToAppend();

        for (auto it = tasks.begin(); it != tasks.end(); ++it) {
            const auto& tileId = it->first;
            const auto& data   = it->second;

            if (data.empty()) {
                continue;
            }

            const GLuint existingId = surfacePtr->getMosaicTextureIdByTileId(tileId);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            if (existingId) { // update
                glBindTexture(GL_TEXTURE_2D, existingId);

                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, defaultTileSidePixelSize, defaultTileSidePixelSize, GL_RED, GL_UNSIGNED_BYTE, data.data());

                QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();
                gl->glGenerateMipmap(GL_TEXTURE_2D);
            }
            else { // create
                GLuint texId = 0;
                glGenTextures(1, &texId);
                glBindTexture(GL_TEXTURE_2D, texId);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, defaultTileSidePixelSize, defaultTileSidePixelSize, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());

                QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();
                gl->glGenerateMipmap(GL_TEXTURE_2D);

                surfacePtr->setMosaicTextureIdByTileId(tileId, texId);
            }
        }
    }
}

void GraphicsScene3dView::InFboRenderer::processImageTexture(GraphicsScene3dView *viewPtr) const
{
    auto imagePtr = viewPtr->getImageViewPtr();
    auto& task = imagePtr->getTextureTasksRef();

    if (task.isNull())
        return;

    GLuint textureId = viewPtr->getImageViewPtr()->getTextureId();

    if (textureId) {
        glDeleteTextures(1, &textureId);
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, viewPtr->getImageViewPtr()->getUseLinearFilter() ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, viewPtr->getImageViewPtr()->getUseLinearFilter() ? GL_LINEAR : GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    QImage glImage = task.convertToFormat(QImage::Format_RGBA8888).mirrored();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    imagePtr->setTextureId(textureId);

    QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();
    glFuncs->glGenerateMipmap(GL_TEXTURE_2D);

    task = QImage();
}

void GraphicsScene3dView::InFboRenderer::processSurfaceTexture(GraphicsScene3dView *viewPtr) const
{
    // init/reinit
    auto surfacePtr = viewPtr->getSurfaceViewPtr();
    auto task = surfacePtr->takeSurfaceColorTableToAppend();

    if (task.empty()) {
        return;
    }

    GLuint textureId = surfacePtr->getSurfaceColorTableTextureId();

    if (textureId) {
        glDeleteTextures(1, &textureId);
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, task.size() / 4, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, task.data());

    surfacePtr->setSurfaceColorTableTextureId(textureId);

    // deleting
    auto textureIdtoDel = surfacePtr->takeSurfaceColorTableToDelete();
    if (textureIdtoDel) {
        glDeleteTextures(1, &textureIdtoDel);
    }
}

GraphicsScene3dView::Camera::Camera(GraphicsScene3dView* viewPtr) :
    viewPtr_(viewPtr)
{
    setMapView();

    if (viewPtr) { // for main cam
        navYawTmr_.start();
    }
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
    ,distForMapView_(m_distToFocusPoint)
    ,m_sensivity(std::move(sensivity))
{
   setIsometricView();
}

float GraphicsScene3dView::Camera::distForMapView() const
{
    return distForMapView_;
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

    tryResetRotateAngle();
    checkRotateAngle();
    updateCameraParams();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::rotate(const QPointF& prevCenter, const QPointF& currCenter, qreal angleDelta, qreal widgetHeight)
{
    const qreal increaseCoeff{ 1.3 };
    const qreal angleDeltaY = (prevCenter - currCenter).y() / widgetHeight * 90.0;

    m_rotAngle.setX(m_rotAngle.x() - qDegreesToRadians(angleDelta));
    m_rotAngle.setY(m_rotAngle.y() + qDegreesToRadians(angleDeltaY * increaseCoeff));

    tryResetRotateAngle();
    checkRotateAngle();
    updateCameraParams();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::move(const QVector2D &startPos, const QVector2D &endPos)
{
    QVector4D horizontalAxis{ -1.0f, 0.0f, 0.0f, 0.0f };
    QVector4D verticalAxis{ 0.0f, -1.0f, 0.0f, 0.0f };

    m_deltaOffset = ((horizontalAxis * (float)(endPos.x() - startPos.x()) +
                      verticalAxis * (float)(endPos.y() - startPos.y()))).toVector3D();

    m_lookAt = m_lookAtSave + m_deltaOffset;

    updateCameraParams();
    tryToChangeViewLlaRef();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::resetZAxis()
{
    m_lookAt.setZ(0);

    updateCameraParams();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::moveZAxis(float z)
{
    float xCamera = -sinf(m_rotAngle.y()) * cosf(-m_rotAngle.x()) * m_distToFocusPoint;
    float yCamera = -sinf(m_rotAngle.y()) * sinf(-m_rotAngle.x()) * m_distToFocusPoint;
    float zCamera = cosf(m_rotAngle.y()) * m_distToFocusPoint;
    float currLookAtHeight = -(m_lookAt.z() + z);
    float currCameraHeight = zCamera + currLookAtHeight;

    if (currCameraHeight > 0) {
        float cathetus = std::sqrt(std::pow(xCamera, 2) + std::pow(yCamera, 2));
        float hypotenuse = std::sqrt(std::pow(cathetus, 2) + std::pow(currCameraHeight, 2));
        distForMapView_ = hypotenuse;
    }
    else {
        distForMapView_ = 0.0f;
    }

    m_lookAt.setZ(m_lookAt.z() + z);

    updateCameraParams();
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
#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    const float increaseCoeff{ 0.95f };
    m_distToFocusPoint -= delta * m_distToFocusPoint * increaseCoeff;
    distForMapView_ = m_distToFocusPoint;
#else
    m_distToFocusPoint = delta > 0.f ? m_distToFocusPoint / 1.15f : m_distToFocusPoint * 1.15f;
    distForMapView_ = m_distToFocusPoint;
#endif

    const float minFocusDist = 2.0f;
    const float maxFocusDist = 100000.0f * 100.0f;
    if (m_distToFocusPoint < minFocusDist) {
        m_distToFocusPoint = minFocusDist;
        distForMapView_ = m_distToFocusPoint;
    }
    if (m_distToFocusPoint >= maxFocusDist) {
        m_distToFocusPoint = maxFocusDist;
        distForMapView_ = m_distToFocusPoint;
    }

    //
    bool preIsPersp{ false };
    distToGround_ = std::max(0.0f, std::fabs(-cosf(m_rotAngle.y()) * m_distToFocusPoint));
    float perspEdge = viewPtr_ ? viewPtr_->perspectiveEdge_ : 5000.0f;
    preIsPersp = distToGround_ < perspEdge;
    //bool changedToOrtho       =  isPerspective_ && !preIsPersp;
    //bool changedToPerspective = !isPerspective_ &&  preIsPersp;
    bool projectionChanged    =  isPerspective_ !=  preIsPersp;

    //if (projectionChanged) qDebug() << "CHANGED!";
    //if (changedToOrtho) qDebug() << "changed to ORTHO";
    //if (changedToPerspective) qDebug() << "changed to PERSP";

    NED lookAtNed(m_lookAt.x(), m_lookAt.y(), 0.0f);
    LLA lookAtLla(&lookAtNed, &viewLlaRef_, isPerspective_);
    LLARef lookAtLlaRef(lookAtLla);

    float datasetDist = map::calculateDistance(lookAtLlaRef, datasetLlaRef_);


    if (isPerspective_ && !projectionChanged) { // do nothing
    }
    //else if (isPerspective_ && !projectionChanged && datasetDist < lowDistThreshold_ && getIsFarAwayFromOriginLla()) {
    //    qDebug() << "2";

    //    viewPtr_->setNeedToResetStartPos(true);
    //    LLA datasetLla(datasetLlaRef_.refLla.latitude, datasetLlaRef_.refLla.longitude, 0.0);
    //    NED datasetNed(&datasetLla, &viewLlaRef_, isPerspective_);
    //    m_lookAt -= QVector3D(datasetNed.n, datasetNed.e, 0.0f);
    //    viewLlaRef_ = datasetLlaRef_;
    //}
    else if ( (!isPerspective_ && projectionChanged && datasetDist < lowDistThreshold_ && getIsFarAwayFromOriginLla())) { // catching when ortho->persp trans and near place

        if (cameraListener_) {
            cameraListener_->resetRotationAngle();
        }

        viewPtr_->setNeedToResetStartPos(true);
        LLA datasetLla(datasetLlaRef_.refLla.latitude, datasetLlaRef_.refLla.longitude, 0.0);
        NED datasetNed(&datasetLla, &viewLlaRef_, !isPerspective_);
        m_lookAt -= QVector3D(datasetNed.n, datasetNed.e, 0.0f);
        viewLlaRef_ = datasetLlaRef_;
        m_rotAngle = { 0.0f, 0.0f };
    }
    else if ((isPerspective_ && projectionChanged) || (!isPerspective_ && !projectionChanged)) { // pers -> ortho OR ortho without transfer
        viewPtr_->setNeedToResetStartPos(true);
        viewLlaRef_ = lookAtLlaRef;
        m_lookAt = QVector3D(0.0f, 0.0f, 0.0f);
        m_rotAngle = { 0.0f, 0.0f };
    }

    updateCameraParams();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::commitMovement()
{
    m_lookAt += m_deltaOffset;
    m_deltaOffset = QVector3D();

    updateCameraParams();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::focusOnObject(std::weak_ptr<SceneObject> object)
{
    Q_UNUSED(object)
}

void GraphicsScene3dView::Camera::focusOnPosition(const QVector3D &point)
{
    m_lookAt = point;

    updateCameraParams();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::setDistance(qreal distance)
{
    m_distToFocusPoint = distance;
    distForMapView_ = m_distToFocusPoint;

    updateCameraParams();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::setIsometricView()
{
    reset();

    m_rotAngle.setX(qDegreesToRadians(135.0f));
    m_rotAngle.setY(qDegreesToRadians(45.0f));

    updateCameraParams();
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
    distForMapView_ = m_distToFocusPoint;

    distToGround_ = 0.0f;
    angleToGround_ = 0.0f;
    isPerspective_ = false;

    updateCameraParams();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::resetRotationAngle()
{
    m_rotAngle = {0.0f, 0.0f};

    updateCameraParams();
    updateViewMatrix();
}

void GraphicsScene3dView::Camera::updateCameraParams()
{
    distToGround_ = std::max(0.0f, std::fabs(-cosf(m_rotAngle.y()) * m_distToFocusPoint));

    float perspEdge = 5000.f;
    if (viewPtr_) {
        perspEdge = viewPtr_->perspectiveEdge_;
    }

    isPerspective_ = distToGround_ < perspEdge;
}

void GraphicsScene3dView::Camera::tryToChangeViewLlaRef()
{
    if (isPerspective_ && viewPtr_) {
        NED lookAtNed(m_lookAt.x(), m_lookAt.y(), 0.0f);
        LLA lookAtLla(&lookAtNed, &viewLlaRef_, isPerspective_);
        LLARef lookAtLlaRef(lookAtLla);

        float viewDist = map::calculateDistance(lookAtLlaRef, viewLlaRef_);
        float datasetDist = map::calculateDistance(lookAtLlaRef, datasetLlaRef_);

        if (datasetDist < lowDistThreshold_ && getIsFarAwayFromOriginLla()) {
            viewPtr_->setNeedToResetStartPos(true);
            LLA datasetLla(datasetLlaRef_.refLla.latitude, datasetLlaRef_.refLla.longitude, 0.0);
            NED datasetNed(&datasetLla, &viewLlaRef_, isPerspective_);
            m_lookAt -= QVector3D(datasetNed.n, datasetNed.e, 0.0f);
            viewLlaRef_ = datasetLlaRef_;
        }
        else if (viewDist > highDistThreshold_) {
            viewPtr_->setNeedToResetStartPos(true);
            viewLlaRef_ = lookAtLlaRef;
            m_lookAt = QVector3D(0.0f, 0.0f, 0.0f);
        }
    }
}

void GraphicsScene3dView::Camera::updateViewMatrix()
{
    QVector3D cf;
    cf[0] = -sinf(m_rotAngle.y())*cosf(-m_rotAngle.x())*m_distToFocusPoint;
    cf[1] = -sinf(m_rotAngle.y())*sinf(-m_rotAngle.x())*m_distToFocusPoint;
    cf[2] = -cosf(m_rotAngle.y())*m_distToFocusPoint;

    if (!isPerspective_) {
        m_rotAngle = QVector2D();
    }

    QVector3D cu;
    cu[0] = cosf(m_rotAngle.y())*cosf(-m_rotAngle.x());
    cu[1] = cosf(m_rotAngle.y())*sinf(-m_rotAngle.x());
    cu[2] = -sinf(m_rotAngle.y());

    angleToGround_ = 90.f * std::fabs(cu.z());

    QMatrix4x4 view;
    view.lookAt(cf + m_lookAt, m_lookAt, cu.normalized());
    view.scale(1.0f,1.0f,-1.0f);

    m_view = std::move(view);
}

void GraphicsScene3dView::Camera::checkRotateAngle()
{
    if (m_rotAngle[1] > M_PI_2)
        m_rotAngle[1] = M_PI_2;
    else if (m_rotAngle[1] < 0.0f)
        m_rotAngle[1] = 0.0f;
}

void GraphicsScene3dView::Camera::tryResetRotateAngle()
{
    bool preIsPersp{ false };
    distToGround_ = std::max(0.0f, std::fabs(-cosf(m_rotAngle.y()) * m_distToFocusPoint));
    float perspEdge = viewPtr_ ? viewPtr_->perspectiveEdge_ : 5000.0f;
    preIsPersp = distToGround_ < perspEdge;
    bool projectionChanged = isPerspective_ != preIsPersp;
    if (projectionChanged && isPerspective_) {
        m_rotAngle = { 0.0f, 0.0f };
    }
}

float GraphicsScene3dView::Camera::getHeightAboveGround() const
{
    return distToGround_;
}

float GraphicsScene3dView::Camera::getAngleToGround() const
{
    return angleToGround_;
}

bool GraphicsScene3dView::Camera::getIsPerspective() const
{
    return isPerspective_;
}

bool GraphicsScene3dView::Camera::getIsFarAwayFromOriginLla() const
{
    return !isPerspective_ || (viewLlaRef_ != datasetLlaRef_);
}

map::CameraTilt GraphicsScene3dView::Camera::getCameraTilt() const
{
    float xRot = m_rotAngle.x();

    while (xRot >  M_PI) {
        xRot -= 2.f * M_PI;
    }
    while (xRot <= -M_PI) {
        xRot += 2.f * M_PI;
    }

    float deg = qRadiansToDegrees(xRot);

    if (deg > -45.f && deg <= 45.f) {
        return map::CameraTilt::Down;
    }
    else if (deg > 45.f && deg <= 135.f) {
        return map::CameraTilt::Right;
    }
    else if (deg >= -135.f && deg <= -45.f) {
        return map::CameraTilt::Left;
    }
    else {
        return map::CameraTilt::Up;
    }
}

qreal GraphicsScene3dView::Camera::distToFocusPoint() const
{
    return m_distToFocusPoint;
}

QString GraphicsScene3dView::InFboRenderer::checkOpenGLError() const {
    GLenum errorCode = glGetError();
    QString retVal;

    if (errorCode != GL_NO_ERROR) {
        switch (errorCode) {
        case GL_INVALID_ENUM:
            retVal = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            retVal = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            retVal = "GL_INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            retVal = "GL_STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            retVal = "GL_STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            retVal = "GL_OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            retVal = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        default:
            retVal = QString("Unknown error: 0x%1").arg(errorCode, 0, 16);
            break;
        }
    }

    return retVal;
}
