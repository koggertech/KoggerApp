#include "mission_run_controller.h"

#include <algorithm>
#include <cmath>

#include <QLineF>

#include "mission_geometry.h"
#include "mission_run.h"
#include "mission_vehicle.h"
#include "scene3d_view.h"
#include "themes.h"

namespace {

constexpr double kGeodesicStepMeters = 2000.0;
constexpr int kGeodesicMaxSteps = 256;
constexpr double kArrowAlongSegment = 0.75;
constexpr double kTapRadiusPx = 22.0;

constexpr int kVehicleLineAlpha = 150;
constexpr int kAheadAlpha = 235;
constexpr int kCurrentAlpha = 245;
constexpr int kDoneAlpha = 170;

QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}
constexpr double kMinWidthPx = 0.5;
constexpr double kMaxWidthPx = 12.0;
const QColor kPointColor(255, 255, 255, 240);
const QColor kDonePointColor(148, 163, 184, 220);
const QColor kStartColor(245, 158, 11, 240);
const QColor kVehicleHomeColor(22, 163, 74, 240);
const QColor kRallyColor(20, 184, 166, 240);
const QColor kFenceInclusion(34, 197, 94, 200);
const QColor kFenceExclusion(239, 68, 68, 200);
const QColor kFenceExclusionFill(239, 68, 68, 35);
const QColor kSelectedRing(255, 255, 255, 255);
const QColor kOutline(17, 24, 39, 230);

constexpr float kVehicleLineWidthPx = 1.5f;
constexpr float kFenceWidthPx = 1.5f;
constexpr float kLabelledPointPx = 7.0f;
constexpr float kPathPointPx = 4.0f;
constexpr float kTargetRingPx = 13.0f;
constexpr float kSelectedRingPx = 17.0f;

void appendPoint(MissionLayer::RenderData& rd, const QVector3D& world, const QColor& color, float sizePx, const QString& label,
                 MissionLayer::Marker::Shape shape = MissionLayer::Marker::Shape::Circle)
{
    MissionLayer::Marker body;
    body.world = world;
    body.color = color;
    body.sizePx = sizePx;
    body.shape = shape;
    body.label = label;
    body.topmost = true;
    rd.markers.append(body);
    if (shape != MissionLayer::Marker::Shape::Circle) {
        return;
    }
    MissionLayer::Marker outline = body;
    outline.color = kOutline;
    outline.sizePx = sizePx + 1.25f;
    outline.shape = MissionLayer::Marker::Shape::Ring;
    outline.label.clear();
    outline.halo = false;
    rd.markers.append(outline);
}

void appendRing(MissionLayer::RenderData& rd, const QVector3D& world, const QColor& color, float sizePx)
{
    MissionLayer::Marker ring;
    ring.world = world;
    ring.color = color;
    ring.sizePx = sizePx;
    ring.shape = MissionLayer::Marker::Shape::Ring;
    ring.topmost = true;
    rd.markers.append(ring);
}

} // namespace

MissionRunController::MissionRunController(GraphicsScene3dView* view, MissionLayer* layer, QObject* parent)
    : QObject(parent)
    , view_(view)
    , layer_(layer)
{
}

void MissionRunController::setTracker(mission::MissionRunTracker* tracker)
{
    if (tracker_ == tracker) {
        return;
    }
    if (tracker_) {
        disconnect(tracker_, nullptr, this, nullptr);
    }
    tracker_ = tracker;
    if (tracker_) {
        connect(tracker_, &mission::MissionRunTracker::snapshotChanged, this, &MissionRunController::onSnapshotChanged);
        connect(tracker_, &mission::MissionRunTracker::progressChanged, this, &MissionRunController::markDirty);
        connect(tracker_, &mission::MissionRunTracker::vehicleMoved, this, &MissionRunController::markDirty);
    }
    onSnapshotChanged();
}

void MissionRunController::setEditorActive(bool active)
{
    if (editorActive_ == active) {
        return;
    }
    editorActive_ = active;
    updateShown();
    markDirty();
}

void MissionRunController::setLayerVisible(bool visible)
{
    if (layerVisible_ == visible) {
        return;
    }
    layerVisible_ = visible;
    emit layerVisibleChanged();
    updateShown();
    markDirty();
}

void MissionRunController::setDrawOption(bool& option, bool value)
{
    if (option == value) {
        return;
    }
    option = value;
    emit drawOptionsChanged();
    markDirty();
}

void MissionRunController::setColorOption(QColor& option, const QColor& value)
{
    if (!value.isValid() || option == value) {
        return;
    }
    option = value;
    emit drawOptionsChanged();
    markDirty();
}

void MissionRunController::setWidthOption(double& option, double value)
{
    const double clamped = std::clamp(value, kMinWidthPx, kMaxWidthPx);
    if (qFuzzyCompare(option, clamped)) {
        return;
    }
    option = clamped;
    emit drawOptionsChanged();
    markDirty();
}

void MissionRunController::setAheadColor(const QColor& color)
{
    setColorOption(aheadColor_, color);
}

void MissionRunController::setCurrentColor(const QColor& color)
{
    setColorOption(currentColor_, color);
}

void MissionRunController::setDoneColor(const QColor& color)
{
    setColorOption(doneColor_, color);
}

void MissionRunController::setRouteWidth(double width)
{
    setWidthOption(routeWidth_, width);
}

void MissionRunController::setCurrentWidth(double width)
{
    setWidthOption(currentWidth_, width);
}

void MissionRunController::setShowFence(bool show)
{
    setDrawOption(showFence_, show);
}

void MissionRunController::setShowRally(bool show)
{
    setDrawOption(showRally_, show);
}

void MissionRunController::setShowHome(bool show)
{
    setDrawOption(showHome_, show);
}

void MissionRunController::setShowVehicleLine(bool show)
{
    setDrawOption(showVehicleLine_, show);
}

void MissionRunController::setShowLabels(bool show)
{
    setDrawOption(showLabels_, show);
}

bool MissionRunController::shown() const
{
    return layerVisible_ && !editorActive_ && tracker_ && tracker_->known() && !tracker_->vehicleEmpty();
}

int MissionRunController::selectedJumpSeq() const
{
    return (tracker_ && selectedSeq_ >= 0) ? tracker_->jumpSeqFor(selectedSeq_) : -1;
}

int MissionRunController::selectedNav() const
{
    return (tracker_ && selectedSeq_ >= 0) ? tracker_->navIndexOf(selectedSeq_) : 0;
}

QString MissionRunController::selectedLabel() const
{
    if (!tracker_ || selectedSeq_ < 0 || selectedSeq_ >= tracker_->items().size()) {
        return QString();
    }
    return tracker_->labelFor(tracker_->items().at(selectedSeq_));
}

bool MissionRunController::onTap(qreal x, qreal y)
{
    if (!shown() || !tracker_->actionable()) {
        clearSelection();
        return false;
    }
    const auto& seqs = tracker_->navSeqs();
    const auto& positions = tracker_->navPositions();
    const QPointF tap(x, y);
    double bestDist = kTapRadiusPx * renderScale();
    int best = -1;
    for (int k = 0; k < seqs.size(); ++k) {
        const QPointF s = toScreen(toScene(positions.at(k)));
        if (!std::isfinite(s.x()) || !std::isfinite(s.y())) {
            continue;
        }
        const double d = QLineF(s, tap).length();
        if (d <= bestDist) {
            bestDist = d;
            best = seqs.at(k);
        }
    }
    if (best < 0) {
        clearSelection();
        return false;
    }
    if (best != selectedSeq_) {
        selectedSeq_ = best;
        emit selectionChanged();
    }
    updateSelectedScreen();
    markDirty();
    return true;
}

void MissionRunController::clearSelection()
{
    if (selectedSeq_ < 0) {
        return;
    }
    selectedSeq_ = -1;
    selectedOnScreen_ = false;
    emit selectionChanged();
    emit selectedScreenChanged();
    markDirty();
}

void MissionRunController::rebuildIfNeeded()
{
    if (!view_ || !view_->m_camera) {
        return;
    }
    updateSelectedScreen();
    const bool persp = view_->m_camera->getIsPerspective();
    const bool viewRefChanged = (lastViewRef_ != view_->m_camera->viewLlaRef_);
    const bool perspChanged = (lastPerspective_ != persp);
    if (!dirty_ && !viewRefChanged && !perspChanged) {
        return;
    }
    lastViewRef_ = view_->m_camera->viewLlaRef_;
    lastPerspective_ = persp;
    dirty_ = false;
    rebuild();
}

void MissionRunController::markDirty()
{
    dirty_ = true;
    if (view_) {
        view_->update();
    }
}

void MissionRunController::updateShown()
{
    const bool now = shown();
    if (now != shown_) {
        shown_ = now;
        emit shownChanged();
    }
    if (!now || !tracker_ || !tracker_->actionable()) {
        clearSelection();
    }
}

void MissionRunController::onSnapshotChanged()
{
    fence_.clear();
    rally_.clear();
    if (tracker_ && tracker_->known()) {
        const mission::VehicleImport imported = mission::fromVehicle(tracker_->snapshot(), 0.0);
        fence_ = imported.plan.fence;
        for (const auto& r : imported.plan.rally) {
            rally_.append(r.pos);
        }
    }
    if (selectedSeq_ >= 0 && (!tracker_ || tracker_->navIndexOf(selectedSeq_) == 0)) {
        clearSelection();
    } else if (selectedSeq_ >= 0) {
        emit selectionChanged();
    }
    updateShown();
    markDirty();
}

void MissionRunController::updateSelectedScreen()
{
    bool onScreen = false;
    QPointF screen = selectedScreen_;
    if (selectedSeq_ >= 0 && shown() && view_->width() > 0 && view_->height() > 0) {
        const int k = tracker_->navIndexOf(selectedSeq_) - 1;
        if (k >= 0) {
            screen = toScreen(toScene(tracker_->navPositions().at(k)));
            onScreen = std::isfinite(screen.x()) && std::isfinite(screen.y())
                       && screen.x() >= 0.0 && screen.y() >= 0.0 && screen.x() <= view_->width() && screen.y() <= view_->height();
        }
    }
    const bool moved = QLineF(screen, selectedScreen_).length() > 0.5;
    if (onScreen == selectedOnScreen_ && !moved) {
        return;
    }
    selectedOnScreen_ = onScreen;
    selectedScreen_ = screen;
    if (!screenNotifyQueued_) {
        screenNotifyQueued_ = true;
        QMetaObject::invokeMethod(this, [this]() {
            screenNotifyQueued_ = false;
            emit selectedScreenChanged();
        }, Qt::QueuedConnection);
    }
}

void MissionRunController::appendLeg(MissionLayer::RenderData& rd, const mission::GeoPoint& a, const mission::GeoPoint& b,
                                     const QColor& color, float widthPx, bool arrow) const
{
    const double meters = mission::geoDistance(a, b);
    const int steps = std::clamp(static_cast<int>(std::ceil(meters / kGeodesicStepMeters)), 1, kGeodesicMaxSteps);
    const bool perspective = view_->m_camera->getIsPerspective();

    MissionLayer::Line strip;
    strip.color = color;
    strip.widthPx = widthPx;
    double prevLon = a.lon;
    for (int k = 0; k <= steps; ++k) {
        const mission::GeoPoint g = mission::greatCirclePoint(a, b, static_cast<double>(k) / steps);
        if (!perspective && k > 0 && std::fabs(g.lon - prevLon) > 180.0) {
            if (strip.points.size() >= 2) {
                rd.lines.append(strip);
            }
            strip.points.clear();
        }
        prevLon = g.lon;
        strip.points.append(toScene(g));
    }
    if (strip.points.size() >= 2) {
        rd.lines.append(strip);
    }
    if (!arrow) {
        return;
    }
    const mission::GeoPoint m0 = mission::greatCirclePoint(a, b, kArrowAlongSegment);
    if (perspective || std::fabs(b.lon - m0.lon) <= 180.0) {
        rd.arrows.append({toScene(m0), toScene(b)});
    }
}

QVector3D MissionRunController::toScene(const mission::GeoPoint& p) const
{
    GeoJsonCoord c;
    c.lat = p.lat;
    c.lon = p.lon;
    c.z = 0.0;
    c.hasZ = false;
    return view_->geojsonToScene(c);
}

QPointF MissionRunController::toScreen(const QVector3D& world) const
{
    const QRect viewport = view_->boundingRect().toRect();
    const QVector3D win = world.project(view_->m_camera->m_view * view_->m_model, view_->m_projection, viewport);
    return QPointF(win.x(), view_->height() - win.y());
}

void MissionRunController::rebuild()
{
    MissionLayer::RenderData rd;
    rd.enabled = shown();
    if (!rd.enabled || !layer_) {
        if (layer_) {
            layer_->setRenderData(rd);
        }
        return;
    }

    for (const auto& f : std::as_const(fence_)) {
        if (!showFence_ || f.ring.size() < mission::kFenceMinVertices) {
            continue;
        }
        if (!f.inclusion) {
            MissionLayer::Fill fill;
            fill.ring.reserve(f.ring.size());
            for (const auto& p : f.ring) {
                fill.ring.append(toScene(p));
            }
            fill.color = kFenceExclusionFill;
            rd.fills.append(fill);
        }
        for (int i = 0; i < f.ring.size(); ++i) {
            appendLeg(rd, f.ring.at(i), f.ring.at((i + 1) % f.ring.size()), f.inclusion ? kFenceInclusion : kFenceExclusion, kFenceWidthPx, false);
        }
    }

    const auto& seqs = tracker_->navSeqs();
    const auto& positions = tracker_->navPositions();
    const auto& items = tracker_->items();
    const int targetIdx = tracker_->finished() ? int(positions.size()) : tracker_->navIndexOf(tracker_->targetSeq()) - 1;

    for (int k = 1; k < positions.size(); ++k) {
        const bool done = targetIdx >= 0 && k < targetIdx;
        const bool current = k == targetIdx;
        appendLeg(rd, positions.at(k - 1), positions.at(k),
                  done ? withAlpha(doneColor_, kDoneAlpha) : (current ? withAlpha(currentColor_, kCurrentAlpha) : withAlpha(aheadColor_, kAheadAlpha)),
                  float(current ? currentWidth_ : routeWidth_), !done);
    }

    const mission::GeoPoint vehicle = tracker_->vehiclePosition();
    if (showVehicleLine_ && targetIdx >= 0 && targetIdx < positions.size() && vehicle.isValid() && tracker_->running()) {
        appendLeg(rd, vehicle, positions.at(targetIdx), withAlpha(currentColor_, kVehicleLineAlpha), kVehicleLineWidthPx, false);
    }

    for (int k = 0; k < seqs.size(); ++k) {
        const mission::RunItem& item = items.at(seqs.at(k));
        const bool done = targetIdx >= 0 && k < targetIdx;
        const QVector3D world = toScene(positions.at(k));
        QString label;
        float size = kPathPointPx;
        QColor color = done ? kDonePointColor : kPointColor;
        auto shape = MissionLayer::Marker::Shape::Circle;
        switch (item.kind) {
        case mission::RunItem::Kind::Start:
            label = QStringLiteral("S");
            size = kLabelledPointPx;
            color = done ? kDonePointColor : kStartColor;
            break;
        case mission::RunItem::Kind::Waypoint:
            label = QString::number(item.ordinal);
            size = kLabelledPointPx;
            break;
        case mission::RunItem::Kind::ReturnToLaunch:
        case mission::RunItem::Kind::Hold:
            shape = MissionLayer::Marker::Shape::Diamond;
            size = kLabelledPointPx;
            break;
        default:
            break;
        }
        appendPoint(rd, world, color, size, showLabels_ ? label : QString(), shape);
        if (k == targetIdx) {
            appendRing(rd, world, withAlpha(currentColor_, kCurrentAlpha), kTargetRingPx);
        }
        if (seqs.at(k) == selectedSeq_) {
            appendRing(rd, world, kSelectedRing, kSelectedRingPx);
        }
    }

    if (showRally_) {
        for (const auto& r : std::as_const(rally_)) {
            appendPoint(rd, toScene(r), kRallyColor, kLabelledPointPx, QStringLiteral("R"));
        }
    }

    const mission::GeoPoint home = tracker_->vehicleHome();
    if (showHome_ && home.isValid()) {
        appendPoint(rd, toScene(home), kVehicleHomeColor, kLabelledPointPx + 1.0f, QStringLiteral("H"));
    }

    layer_->setRenderData(rd);
}
