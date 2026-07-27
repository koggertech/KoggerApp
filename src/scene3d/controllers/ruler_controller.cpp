#include "ruler_controller.h"

#include <cmath>
#include <QtMath>
#include <QLineF>

#include "scene3d_view.h"
#include "ruler_tool.h"
#include "map_defs.h"

static double wrapLon180(double lon)
{
    while (lon > 180.0) lon -= 360.0;
    while (lon < -180.0) lon += 360.0;
    return lon;
}

RulerController::RulerController(GraphicsScene3dView* view, RulerTool* tool, QObject* parent)
    : QObject(parent)
    , view_(view)
    , tool_(tool)
{
}

bool RulerController::enabled() const
{
    return enabled_;
}

bool RulerController::drawing() const
{
    return drawing_;
}

bool RulerController::selected() const
{
    return selected_;
}

bool RulerController::hasGeometry() const
{
    return geoPoints_.size() >= 2;
}

void RulerController::setEnabled(bool enabled)
{
    if (enabled_ == enabled) {
        return;
    }

    enabled_ = enabled;
    tool_->setSelected(enabled);
    if (enabled_) {
        tool_->setEnabled(true);
    }

    if (!enabled_) {
        previewActive_ = false;
        updateGeometry();
        setDrawing(false);
        setSelected(false);
        resetInteraction();
    } else {
        setDrawing(false);
        setSelected(false);
        view_->notifyManualCameraInteraction();
    }

    emit enabledChanged();
    view_->update();
}

void RulerController::clear()
{
    geoPoints_.clear();
    previewActive_ = false;
    rebuildCommitted();
    updateGeometry();
    setDrawing(false);
    setSelected(false);
    resetInteraction();
    emit stateChanged();
    view_->update();
}

void RulerController::finishDrawing()
{
    if (!enabled_ || !drawing_) {
        return;
    }
    if (geoPoints_.size() < 2) {
        return;
    }

    previewActive_ = false;
    updateGeometry();
    setDrawing(false);
    setSelected(true);
    resetInteraction();
    setEnabled(false);
    emit stateChanged();
}

void RulerController::cancelDrawing()
{
    if (!enabled_) {
        return;
    }

    geoPoints_.clear();
    previewActive_ = false;
    rebuildCommitted();
    updateGeometry();
    setDrawing(false);
    setSelected(false);
    resetInteraction();
    view_->update();
}

void RulerController::deleteSelected()
{
    if (drawing_ || !selected_) {
        return;
    }

    geoPoints_.clear();
    previewActive_ = false;
    rebuildCommitted();
    updateGeometry();
    setSelected(false);
    resetInteraction();
    view_->update();
}

void RulerController::onLeftClick(qreal x, qreal y)
{
    QVector3D p;
    const bool haveP = view_->tryProjectScreenToPlane(x, y, 0.0f, p);

    if (!drawing_) {
        if (haveP) {
            geoPoints_.clear();
            previewActive_ = false;
            resetInteraction();
            setDrawing(true);
            addPoint(p);
            lastClickPos_ = QPointF(x, y);
            hasLastClick_ = true;
            lastClickTimer_.restart();
        }
    } else {
        const QPointF clickPos(x, y);
        const bool isDoubleClick = hasLastClick_ &&
                                   lastClickTimer_.isValid() &&
                                   lastClickTimer_.elapsed() < 350 &&
                                   (QLineF(clickPos, lastClickPos_).length() < 6.0);

        lastClickPos_ = clickPos;
        hasLastClick_ = true;
        lastClickTimer_.restart();

        if (isDoubleClick && geoPoints_.size() >= 2) {
            finishDrawing();
            hasLastClick_ = false;
        } else if (haveP) {
            previewActive_ = false;
            addPoint(p);
        }
    }

    emit stateChanged();
    view_->update();
}

void RulerController::onMouseMove(Qt::MouseButtons button, qreal x, qreal y)
{
    if (!enabled_ || button != Qt::MouseButton::NoButton) {
        return;
    }

    if (drawing_ && !geoPoints_.isEmpty()) {
        QVector3D previewWorld;
        if (view_->tryProjectScreenToPlane(x, y, 0.0f, previewWorld)) {
            previewGeo_ = view_->sceneToGeojson(previewWorld);
            previewActive_ = true;
            updateGeometry();
            view_->update();
        } else if (previewActive_) {
            previewActive_ = false;
            updateGeometry();
            view_->update();
        }
    } else if (!drawing_ && previewActive_) {
        previewActive_ = false;
        updateGeometry();
        view_->update();
    }
}

bool RulerController::onKey(Qt::Key key)
{
    if (enabled_ || selected_) {
        if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
            deleteSelected();
            return true;
        }
    }
    if (enabled_) {
        if (key == Qt::Key_Escape) {
            cancelDrawing();
            return true;
        }
        if (key == Qt::Key_Enter || key == Qt::Key_Return) {
            finishDrawing();
            return true;
        }
    }
    return false;
}

void RulerController::onPointerCanceled()
{
    resetInteraction();
}

void RulerController::rebuildIfNeeded()
{
    const bool persp = view_->m_camera->getIsPerspective();
    const bool viewRefChanged = (lastViewRef_ != view_->m_camera->viewLlaRef_);
    const bool perspChanged = (lastPerspective_ != persp);
    if (!viewRefChanged && !perspChanged) {
        return;
    }

    lastViewRef_ = view_->m_camera->viewLlaRef_;
    lastPerspective_ = persp;

    if (geoPoints_.isEmpty() && !previewActive_) {
        return;
    }

    rebuildCommitted();
    updateGeometry();
}

void RulerController::setDrawing(bool drawing)
{
    if (drawing_ == drawing) {
        return;
    }
    drawing_ = drawing;
    emit stateChanged();
}

void RulerController::setSelected(bool /*selected*/)
{
    if (!selected_) {
        return;
    }
    selected_ = false;
    tool_->setSelected(false);
    emit stateChanged();
}

void RulerController::resetInteraction()
{
    hasLastClick_ = false;
}

void RulerController::addPoint(const QVector3D& p)
{
    geoPoints_.push_back(view_->sceneToGeojson(p));
    lastViewRef_ = view_->m_camera->viewLlaRef_;
    lastPerspective_ = view_->m_camera->getIsPerspective();
    rebuildCommitted();
    updateGeometry();
}

QVector3D RulerController::toScene(double latDeg, double lonDeg) const
{
    GeoJsonCoord c;
    c.lat = latDeg;
    c.lon = lonDeg;
    c.z = 0.0;
    c.hasZ = false;
    return view_->geojsonToScene(c);
}

double RulerController::geoDistanceMeters(const GeoJsonCoord& a, const GeoJsonCoord& b) const
{
    return map::calculateDistance(LLARef(LLA(a.lat, a.lon, 0.0)), LLARef(LLA(b.lat, b.lon, 0.0)));
}

void RulerController::buildSegmentStrips(const GeoJsonCoord& a,
                                         const GeoJsonCoord& b,
                                         bool perspective,
                                         QVector<QVector<QVector3D>>& strips,
                                         QVector<QVector3D>& mids,
                                         QVector<double>& segMeters) const
{
    segMeters.push_back(geoDistanceMeters(a, b));

    const double lat1 = qDegreesToRadians(a.lat);
    const double lon1 = qDegreesToRadians(a.lon);
    const double lat2 = qDegreesToRadians(b.lat);
    const double lon2 = qDegreesToRadians(b.lon);

    const QVector3D va(static_cast<float>(std::cos(lat1) * std::cos(lon1)),
                       static_cast<float>(std::cos(lat1) * std::sin(lon1)),
                       static_cast<float>(std::sin(lat1)));
    const QVector3D vb(static_cast<float>(std::cos(lat2) * std::cos(lon2)),
                       static_cast<float>(std::cos(lat2) * std::sin(lon2)),
                       static_cast<float>(std::sin(lat2)));

    double dot = static_cast<double>(QVector3D::dotProduct(va, vb));
    dot = std::max(-1.0, std::min(1.0, dot));
    const double omega = std::acos(dot);

    int steps = 1;
    if (omega > 1e-6) {
        steps = std::max(1, std::min(256, static_cast<int>(std::ceil(qRadiansToDegrees(omega)))));
    }

    auto sampleGeo = [&](double t, double& outLat, double& outLon) {
        if (omega < 1e-6) {
            outLat = a.lat + (b.lat - a.lat) * t;
            outLon = wrapLon180(a.lon + (b.lon - a.lon) * t);
            return;
        }
        const double s1 = std::sin((1.0 - t) * omega) / std::sin(omega);
        const double s2 = std::sin(t * omega) / std::sin(omega);
        QVector3D v = va * static_cast<float>(s1) + vb * static_cast<float>(s2);
        v.normalize();
        outLat = qRadiansToDegrees(std::asin(std::max(-1.0f, std::min(1.0f, v.z()))));
        outLon = qRadiansToDegrees(std::atan2(v.y(), v.x()));
    };

    double midLat, midLon;
    sampleGeo(0.5, midLat, midLon);
    mids.push_back(toScene(midLat, wrapLon180(midLon)));

    QVector<QVector3D> cur;
    double prevLon = 0.0;
    for (int k = 0; k <= steps; ++k) {
        double lat, lon;
        sampleGeo(static_cast<double>(k) / steps, lat, lon);
        if (!perspective && k > 0 && std::abs(lon - prevLon) > 180.0) {
            if (cur.size() >= 2) {
                strips.push_back(cur);
            }
            cur.clear();
        }
        prevLon = lon;
        cur.push_back(toScene(lat, lon));
    }
    if (cur.size() >= 2) {
        strips.push_back(cur);
    }
}

void RulerController::rebuildCommitted()
{
    committedVertices_.clear();
    committedStrips_.clear();
    committedMids_.clear();
    committedSeg_.clear();

    const bool persp = view_->m_camera->getIsPerspective();
    committedVertices_.reserve(geoPoints_.size());
    for (const auto& v : geoPoints_) {
        committedVertices_.push_back(toScene(v.lat, wrapLon180(v.lon)));
    }
    for (int i = 1; i < geoPoints_.size(); ++i) {
        buildSegmentStrips(geoPoints_[i - 1], geoPoints_[i], persp, committedStrips_, committedMids_, committedSeg_);
    }
}

void RulerController::updateGeometry()
{
    QVector<QVector3D> vertices = committedVertices_;
    QVector<QVector<QVector3D>> strips = committedStrips_;
    QVector<QVector3D> mids = committedMids_;
    QVector<double> seg = committedSeg_;

    if (previewActive_ && !geoPoints_.isEmpty()) {
        const bool persp = view_->m_camera->getIsPerspective();
        vertices.push_back(toScene(previewGeo_.lat, wrapLon180(previewGeo_.lon)));
        buildSegmentStrips(geoPoints_.last(), previewGeo_, persp, strips, mids, seg);
    }

    tool_->setGeometry(vertices, strips, mids, seg);
}

bool RulerController::pick(qreal x, qreal y) const
{
    const auto strips = tool_->lineStrips();
    if (strips.isEmpty()) {
        return false;
    }

    const QRect viewport = view_->boundingRect().toRect();
    const QPointF target(x, view_->height() - y);
    const QMatrix4x4 mv = view_->m_camera->m_view * view_->m_model;

    const float thresholdPx = 8.0f;
    float best = thresholdPx;
    bool found = false;

    auto distPointSegment = [](const QPointF& p, const QPointF& segA, const QPointF& segB) -> float {
        const QPointF ab = segB - segA;
        const double ab2 = ab.x() * ab.x() + ab.y() * ab.y();
        if (ab2 <= 1e-6) {
            const QPointF d = p - segA;
            return static_cast<float>(std::sqrt(d.x() * d.x() + d.y() * d.y()));
        }
        const QPointF ap = p - segA;
        double t = (ap.x() * ab.x() + ap.y() * ab.y()) / ab2;
        t = std::max(0.0, std::min(1.0, t));
        const QPointF proj(segA.x() + ab.x() * t, segA.y() + ab.y() * t);
        const QPointF d = p - proj;
        return static_cast<float>(std::sqrt(d.x() * d.x() + d.y() * d.y()));
    };

    for (const auto& strip : strips) {
        QVector<QPointF> screenPoints;
        screenPoints.reserve(strip.size());
        for (const auto& world : strip) {
            const QVector3D win = world.project(mv, view_->m_projection, viewport);
            screenPoints.push_back(QPointF(win.x(), win.y()));
        }
        for (int i = 0; i + 1 < screenPoints.size(); ++i) {
            const float d = distPointSegment(target, screenPoints[i], screenPoints[i + 1]);
            if (d < best) {
                best = d;
                found = true;
            }
        }
    }

    return found;
}
