#include "mission_controller.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QLineF>
#include <QVector2D>
#include <QVariantList>
#include <QVariantMap>

#include "mission_generators.h"
#include "mission_geometry.h"
#include "mission_layer.h"
#include "mission_plan_controller.h"
#include "scene3d_view.h"
#include "themes.h"

namespace {

const QString kHomeId = QStringLiteral("home");
constexpr qint64 kFlashMs = 1200;
constexpr int kFlashPulses = 2;
constexpr double kGeodesicStepMeters = 2000.0;
constexpr int kGeodesicMaxSteps = 256;
constexpr double kLatClampDeg = 85.0;
constexpr double kHitRadiusPx = 16.0;
constexpr double kMidpointHitFactor = 0.9;
constexpr float kVertexHandlePx = 7.0f;
constexpr float kEntryMarkerPx = 6.0f;
constexpr float kExitMarkerPx = 7.0f;
constexpr double kDraftCompactDeg = 5.0;
constexpr float kTemplateSurveyFraction = 0.6f;
constexpr float kTemplateCorridorFraction = 0.5f;
constexpr float kTemplateCorridorWidthFraction = 0.4f;
constexpr double kTemplateSpacingDivisor = 4.0;
constexpr double kTraceSurveySpacingDivisor = 8.0;
constexpr double kTraceCorridorWidthDivisor = 5.0;
constexpr double kCorridorSpacingDivisor = 4.0;
constexpr int kCircleTemplateSegments = 16;

const QColor kRouteColor(59, 130, 246, 235);
const QColor kReturnColor(245, 158, 11, 200);
const QColor kSurveyFill(59, 130, 246, 40);
const QColor kSurveyStroke(37, 99, 235, 230);
const QColor kCorridorFill(168, 85, 247, 40);
const QColor kCorridorStroke(147, 51, 234, 230);
const QColor kWaypointColor(255, 255, 255, 240);
const QColor kHomeColor(245, 158, 11, 240);
const QColor kRallyColor(34, 197, 94, 240);
const QColor kSelectedColor(250, 204, 21, 240);
const QColor kHandleColor(255, 255, 255, 235);
const QColor kErrorStroke(248, 113, 113, 235);
const QColor kDeleteBadge(220, 38, 38, 240);
const QVector2D kDeleteBadgeOffset(28.0f, 14.0f);
const QVector2D kCorridorCenterOffset(0.0f, 26.0f);

constexpr float kHandleSizePx = 8.0f;
const QColor kHandleOutline(17, 24, 39, 230);
const QColor kSelectedRing(250, 204, 21, 255);

void appendHandle(MissionLayer::RenderData& rd, const QVector3D& world, const QColor& fill, float sizePx, const QString& label, bool selected, const QVector2D& offsetPx = QVector2D(), float outlinePx = 2.5f)
{
    MissionLayer::Marker body;
    body.world = world;
    body.offsetPx = offsetPx;
    body.color = fill;
    body.sizePx = sizePx;
    body.shape = MissionLayer::Marker::Shape::Circle;
    body.label = label;
    body.topmost = true;
    rd.markers.append(body);

    MissionLayer::Marker outline = body;
    outline.color = kHandleOutline;
    outline.sizePx = sizePx + outlinePx * 0.5f;
    outline.shape = MissionLayer::Marker::Shape::Ring;
    outline.label.clear();
    outline.halo = false;
    rd.markers.append(outline);

    if (selected) {
        MissionLayer::Marker ring = outline;
        ring.color = kSelectedRing;
        ring.sizePx = sizePx + 6.0f;
        ring.shape = MissionLayer::Marker::Shape::Ring;
        ring.halo = true;
        rd.markers.append(ring);
    }
}

void appendDeleteBadge(MissionLayer::RenderData& rd, const QVector3D& world, float handlePx, const QColor& badgeColor, const QVector2D& offsetPx)
{
    appendHandle(rd, world, badgeColor, handlePx + 3.0f, QString(), false, offsetPx, 1.2f);
    MissionLayer::Marker x;
    x.world = world;
    x.offsetPx = offsetPx;
    x.color = QColor(255, 255, 255, 245);
    x.sizePx = handlePx * 0.7f;
    x.shape = MissionLayer::Marker::Shape::Cross;
    x.topmost = true;
    x.halo = false;
    rd.markers.append(x);
}

double roundTenth(double v)
{
    return std::round(v * 10.0) / 10.0;
}

double clampSpacing(double v)
{
    return std::max(mission::kMinLineSpacing, v);
}

struct BoundsAccumulator
{
    bool init = false;
    QVector3D min;
    QVector3D max;

    void add(const QVector3D& p)
    {
        if (!init) {
            min = max = p;
            init = true;
            return;
        }
        min = QVector3D(std::min(min.x(), p.x()), std::min(min.y(), p.y()), std::min(min.z(), p.z()));
        max = QVector3D(std::max(max.x(), p.x()), std::max(max.y(), p.y()), std::max(max.z(), p.z()));
    }
};

float distPointSegment(const QPointF& p, const QPointF& a, const QPointF& b)
{
    const QPointF ab = b - a;
    const double ab2 = ab.x() * ab.x() + ab.y() * ab.y();
    if (ab2 <= 1e-6) {
        return static_cast<float>(QLineF(p, a).length());
    }
    const QPointF ap = p - a;
    double t = (ap.x() * ab.x() + ap.y() * ab.y()) / ab2;
    t = std::clamp(t, 0.0, 1.0);
    return static_cast<float>(QLineF(p, a + ab * t).length());
}

bool pointInPolygon(const QPointF& p, const QVector<QPointF>& poly)
{
    bool inside = false;
    for (int i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const QPointF& pi = poly[i];
        const QPointF& pj = poly[j];
        const bool intersect = ((pi.y() > p.y()) != (pj.y() > p.y())) &&
                               (p.x() < (pj.x() - pi.x()) * (p.y() - pi.y()) / (pj.y() - pi.y() + 1e-9) + pi.x());
        if (intersect) {
            inside = !inside;
        }
    }
    return inside;
}

} // namespace

MissionController::MissionController(GraphicsScene3dView* view, MissionLayer* layer, QObject* parent)
    : QObject(parent)
    , view_(view)
    , layer_(layer)
{
    flashTimer_.setInterval(33);
    connect(&flashTimer_, &QTimer::timeout, this, [this]() {
        if (flashClock_.elapsed() >= kFlashMs) {
            flashTimer_.stop();
            flashId_.clear();
        }
        markDirty();
    });
}

void MissionController::setPlan(mission::MissionPlanController* plan)
{
    if (plan_ == plan) {
        return;
    }
    if (plan_) {
        disconnect(plan_, nullptr, this, nullptr);
    }
    plan_ = plan;
    if (plan_) {
        connect(plan_, &mission::MissionPlanController::planChanged, this, [this]() {
            if (!selectedId_.isEmpty() && selectedId_ != kHomeId && plan_->indexOfItem(selectedId_) < 0 && !plan_->rallyIds().contains(selectedId_)) {
                selectedId_.clear();
                selectedVertex_ = -1;
                emit selectionChanged();
            }
            markDirty();
        });
    }
    markDirty();
}

void MissionController::setEditing(bool editing)
{
    if (editing_ == editing) {
        return;
    }
    editing_ = editing;
    if (!editing_) {
        onPointerCanceled();
        flashTimer_.stop();
        flashId_.clear();
        draft_.clear();
        emit draftChanged();
        tool_ = ToolNone;
        emit toolChanged();
        clearSelection();
    }
    emit editingChanged();
    markDirty();
}

void MissionController::setTool(int tool)
{
    if (tool < ToolNone || tool > ToolCorridorTrace || tool == ToolSurvey || tool == ToolCorridor) {
        tool = ToolNone;
    }
    const bool leavingTrace = (tool_ == ToolSurveyTrace || tool_ == ToolCorridorTrace) && tool != tool_;
    if (leavingTrace && !draft_.isEmpty()) {
        draft_.clear();
        emit draftChanged();
        markDirty();
    }
    if (tool_ == tool) {
        return;
    }
    tool_ = tool;
    emit toolChanged();
}

bool MissionController::onPress(qreal x, qreal y)
{
    if (!editing_ || !plan_ || dragging_) {
        return false;
    }
    const Hit hit = hitTest(x, y);
    if (hit.kind == HitKind::None) {
        return false;
    }

    QVector3D scenePoint;
    if (!view_->tryProjectScreenToPlane(x, y, 0.0f, scenePoint)) {
        scenePoint = hit.world;
    }

    switch (hit.kind) {
    case HitKind::DeleteVertex:
        if (hit.index < 0) {
            deleteSelected();
        } else if (plan_->removeVertex(hit.id, hit.index)) {
            selectedVertex_ = -1;
            emit selectionChanged();
            markDirty();
        }
        return true;
    case HitKind::Vertex:
    case HitKind::Center:
        select(hit.id, hit.kind == HitKind::Vertex ? hit.index : -1);
        beginDrag(hit, scenePoint);
        return true;
    case HitKind::Midpoint: {
        const mission::GeoPoint g = toGeo(hit.world);
        plan_->beginTransaction();
        if (plan_->insertVertex(hit.id, hit.insertIndex, g.lat, g.lon)) {
            Hit drag = hit;
            drag.kind = HitKind::Vertex;
            drag.index = hit.insertIndex;
            select(hit.id, hit.insertIndex);
            beginDrag(drag, scenePoint);
        } else {
            plan_->endTransaction(true);
        }
        return true;
    }
    case HitKind::RouteMidpoint: {
        const mission::GeoPoint g = toGeo(hit.world);
        plan_->beginTransaction();
        const QString id = plan_->addWaypoint(g.lat, g.lon, hit.insertIndex);
        if (!id.isEmpty()) {
            Hit drag;
            drag.kind = HitKind::Vertex;
            drag.id = id;
            drag.index = 0;
            drag.world = hit.world;
            select(id, -1);
            beginDrag(drag, scenePoint);
        } else {
            plan_->endTransaction(true);
        }
        return true;
    }
    default:
        return false;
    }
}

void MissionController::onDrag(const QVector3D& scenePoint)
{
    if (!dragging_ || !plan_) {
        return;
    }
    const QVector3D delta = scenePoint - dragStartScene_;
    if (!dragMoved_ && delta.length() < 1e-4f) {
        return;
    }
    dragMoved_ = true;

    if (dragHit_.kind == HitKind::Center) {
        QVector<mission::GeoPoint> moved;
        moved.reserve(dragStartVertices_.size());
        for (const auto& v : std::as_const(dragStartVertices_)) {
            moved.append(toGeo(v + delta));
        }
        const QString type = itemType(dragHit_.id);
        QVariantMap patch;
        patch.insert(type == QStringLiteral("survey") ? QStringLiteral("polygon") : QStringLiteral("axis"), QVariant(mission::pointsToVariant(moved)));
        plan_->updateItem(dragHit_.id, patch);
        return;
    }

    if (dragStartVertices_.isEmpty()) {
        return;
    }
    const mission::GeoPoint g = toGeo(dragStartVertices_.first() + delta);
    if (dragHit_.id == kHomeId) {
        plan_->setHome(g.lat, g.lon);
    } else {
        plan_->moveVertex(dragHit_.id, dragHit_.index, g.lat, g.lon);
    }
}

void MissionController::onRelease(const QVector3D& scenePoint, bool hasPoint, bool wasMoved)
{
    if (dragging_) {
        endDrag();
        return;
    }
    if (!editing_ || !plan_ || wasMoved || !hasPoint) {
        return;
    }

    const mission::GeoPoint g = toGeo(scenePoint);
    if (!g.isValid()) {
        return;
    }

    switch (tool_) {
    case ToolHome:
        plan_->setHome(g.lat, g.lon);
        select(kHomeId, -1);
        setTool(ToolNone);
        break;
    case ToolWaypoint: {
        const QString id = plan_->addWaypoint(g.lat, g.lon, -1);
        if (!id.isEmpty()) {
            select(id, -1);
        }
        break;
    }
    case ToolRally: {
        const QString id = plan_->addRally(g.lat, g.lon);
        if (!id.isEmpty()) {
            select(id, -1);
        }
        break;
    }
    case ToolSurveyTrace:
    case ToolCorridorTrace:
        draft_.append(g);
        emit draftChanged();
        markDirty();
        break;
    default: {
        const QPointF screen = toScreen(scenePoint);
        const Hit hit = hitTest(screen.x(), screen.y());
        if (hit.kind == HitKind::Body) {
            select(hit.id, -1);
        } else {
            clearSelection();
        }
        break;
    }
    }
}

bool MissionController::draftReady() const
{
    if (tool_ == ToolSurveyTrace) {
        return draft_.size() >= 3;
    }
    if (tool_ == ToolCorridorTrace) {
        return draft_.size() >= 2;
    }
    return false;
}

void MissionController::finishDraft()
{
    if (!plan_ || !draftReady()) {
        return;
    }
    const bool survey = tool_ == ToolSurveyTrace;
    const QVector<mission::GeoPoint> pts = draft_;
    draft_.clear();
    emit draftChanged();

    const int insertIndex = -1;

    mission::LocalPlane plane(mission::centroid(pts));
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = minX;
    double maxY = maxX;
    for (const auto& g : pts) {
        const QPointF l = plane.toLocal(g);
        minX = std::min(minX, l.x());
        maxX = std::max(maxX, l.x());
        minY = std::min(minY, l.y());
        maxY = std::max(maxY, l.y());
    }
    const double shortSide = std::max(1.0, std::min(maxX - minX, maxY - minY));
    const double longSide = std::max(1.0, std::max(maxX - minX, maxY - minY));

    QString id;
    QVariantMap patch;
    plan_->beginTransaction();
    if (survey) {
        id = plan_->addSurvey(mission::pointsToVariant(pts), insertIndex);
        patch.insert(QStringLiteral("lineSpacing"), clampSpacing(roundTenth(shortSide / kTraceSurveySpacingDivisor)));
    } else {
        id = plan_->addCorridor(mission::pointsToVariant(pts), insertIndex);
        const double width = std::max(2.0 * mission::kMinLineSpacing, roundTenth(longSide / kTraceCorridorWidthDivisor));
        patch.insert(QStringLiteral("width"), width);
        patch.insert(QStringLiteral("lineSpacing"), clampSpacing(roundTenth(width / kCorridorSpacingDivisor)));
    }
    if (!id.isEmpty()) {
        plan_->updateItem(id, patch);
    }
    plan_->endTransaction(true);
    setTool(ToolNone);
    if (!id.isEmpty()) {
        select(id, -1);
    }
}

void MissionController::cancelDraft()
{
    draft_.clear();
    emit draftChanged();
    setTool(ToolNone);
    markDirty();
}

void MissionController::undoDraftVertex()
{
    if (draft_.isEmpty()) {
        return;
    }
    draft_.removeLast();
    emit draftChanged();
    markDirty();
}

bool MissionController::onKey(Qt::Key key)
{
    if (!editing_) {
        return false;
    }
    const bool tracing = tool_ == ToolSurveyTrace || tool_ == ToolCorridorTrace;
    if (tracing && (key == Qt::Key_Return || key == Qt::Key_Enter)) {
        finishDraft();
        return true;
    }
    if (tracing && key == Qt::Key_Backspace && !draft_.isEmpty()) {
        undoDraftVertex();
        return true;
    }
    if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
        deleteSelected();
        return true;
    }
    if (key == Qt::Key_Escape) {
        if (tracing) {
            cancelDraft();
            return true;
        }
        if (tool_ != ToolNone) {
            setTool(ToolNone);
            return true;
        }
        if (!selectedId_.isEmpty()) {
            clearSelection();
            return true;
        }
    }
    return false;
}

void MissionController::onPointerCanceled()
{
    if (dragging_) {
        endDrag();
    }
}

void MissionController::rebuildIfNeeded()
{
    if (!view_ || !view_->m_camera) {
        return;
    }
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

bool MissionController::sceneBounds(QVector3D& minOut, QVector3D& maxOut) const
{
    if (!plan_) {
        return false;
    }
    BoundsAccumulator acc;
    auto consider = [&](const mission::GeoPoint& g) {
        if (g.isValid()) {
            acc.add(toScene(g));
        }
    };
    const auto& m = plan_->expanded().mission;
    if (m.home) {
        consider(*m.home);
    }
    for (const auto& f : m.items) {
        if (f.isNavigation()) {
            consider(mission::GeoPoint(f.lat, f.lon));
        }
    }
    for (const auto& r : m.rally) {
        consider(r);
    }
    for (const auto& item : plan_->plan().items) {
        for (const auto& p : shapeVertices(item)) {
            consider(p);
        }
    }
    minOut = acc.min;
    maxOut = acc.max;
    return acc.init;
}

bool MissionController::itemBounds(const QString& id, QVector3D& minOut, QVector3D& maxOut) const
{
    if (!plan_ || id.isEmpty()) {
        return false;
    }
    BoundsAccumulator acc;
    auto consider = [&](const mission::GeoPoint& g) {
        if (g.isValid()) {
            acc.add(toScene(g));
        }
    };
    const auto& plan = plan_->plan();
    if (id == kHomeId) {
        if (plan.home) {
            consider(*plan.home);
        }
    } else if (const int idx = plan_->indexOfItem(id); idx >= 0) {
        if (const auto* w = std::get_if<mission::WaypointItem>(&plan.items[idx])) {
            consider(w->pos);
        }
        for (const auto& p : shapeVertices(plan.items[idx])) {
            consider(p);
        }
        for (const auto& f : plan_->expanded().mission.items) {
            if (f.sourceId == id && f.isNavigation()) {
                consider(mission::GeoPoint(f.lat, f.lon));
            }
        }
    } else {
        for (const auto& r : plan.rally) {
            if (r.id == id) {
                consider(r.pos);
            }
        }
    }
    minOut = acc.min;
    maxOut = acc.max;
    return acc.init;
}

QVector<mission::GeoPoint> MissionController::shapeVertices(const mission::MissionItem& item)
{
    if (const auto* s = std::get_if<mission::SurveyItem>(&item)) {
        return s->polygon;
    }
    if (const auto* c = std::get_if<mission::CorridorItem>(&item)) {
        return c->axis;
    }
    return {};
}

MissionController::ShapeHandles MissionController::selectedShapeHandles() const
{
    ShapeHandles h;
    if (!plan_ || selectedId_.isEmpty() || !itemHasVertices(selectedId_)) {
        return h;
    }
    h.verts = itemVertices(selectedId_);
    h.closed = itemType(selectedId_) == QStringLiteral("survey");
    h.scene.reserve(h.verts.size());
    for (const auto& v : std::as_const(h.verts)) {
        h.scene.append(toScene(v));
    }
    const int n = h.scene.size();
    const int segCount = h.closed ? n : n - 1;
    for (int i = 0; i < segCount; ++i) {
        h.midpoints.append((h.scene[i] + h.scene[(i + 1) % n]) * 0.5f);
    }
    if (n > 0) {
        h.center = toScene(mission::centroid(h.verts));
        h.centerOffset = h.closed ? QVector2D() : kCorridorCenterOffset;
        h.valid = true;
    }
    return h;
}

void MissionController::showItem(const QString& id)
{
    QVector3D minB;
    QVector3D maxB;
    if (view_ && itemBounds(id, minB, maxB)) {
        view_->missionFocusBounds(minB, maxB);
        flashId_ = id;
        flashClock_.start();
        flashTimer_.start();
        markDirty();
    }
}

void MissionController::select(const QString& id, int vertex)
{
    if (selectedId_ == id && selectedVertex_ == vertex) {
        return;
    }
    selectedId_ = id;
    selectedVertex_ = vertex;
    emit selectionChanged();
    markDirty();
}

void MissionController::clearSelection()
{
    select(QString(), -1);
}

void MissionController::deleteSelected()
{
    if (!plan_ || selectedId_.isEmpty()) {
        return;
    }
    if (selectedId_ == kHomeId) {
        plan_->clearHome();
        clearSelection();
        return;
    }
    if (selectedVertex_ >= 0 && itemHasVertices(selectedId_)) {
        if (plan_->removeVertex(selectedId_, selectedVertex_)) {
            selectedVertex_ = -1;
            emit selectionChanged();
            markDirty();
            return;
        }
    }
    const QString id = selectedId_;
    clearSelection();
    plan_->removeItem(id);
}

void MissionController::addSurveyShape(const QVector<mission::GeoPoint>& polygon, double spacing)
{
    const int insertIndex = -1;
    plan_->beginTransaction();
    const QString id = plan_->addSurvey(mission::pointsToVariant(polygon), insertIndex);
    if (!id.isEmpty()) {
        QVariantMap patch;
        patch.insert(QStringLiteral("lineSpacing"), roundTenth(clampSpacing(spacing)));
        plan_->updateItem(id, patch);
    }
    plan_->endTransaction(true);
    if (!id.isEmpty()) {
        select(id, -1);
    }
}

void MissionController::placeSurveyTemplate()
{
    if (!plan_) {
        return;
    }
    QVector3D center;
    float halfX = 0.0f;
    float halfY = 0.0f;
    if (!viewExtent(center, halfX, halfY)) {
        return;
    }
    const float hx = halfX * kTemplateSurveyFraction;
    const float hy = halfY * kTemplateSurveyFraction;
    const QVector<mission::GeoPoint> polygon = {
        toGeo(center + QVector3D(hx, -hy, 0.0f)),
        toGeo(center + QVector3D(hx, hy, 0.0f)),
        toGeo(center + QVector3D(-hx, hy, 0.0f)),
        toGeo(center + QVector3D(-hx, -hy, 0.0f))
    };
    addSurveyShape(polygon, static_cast<double>(std::min(hx, hy)) / kTemplateSpacingDivisor);
}

void MissionController::placeSurveyCircle()
{
    if (!plan_) {
        return;
    }
    QVector3D center;
    float halfX = 0.0f;
    float halfY = 0.0f;
    if (!viewExtent(center, halfX, halfY)) {
        return;
    }
    const float r = std::min(halfX, halfY) * kTemplateSurveyFraction;
    QVector<mission::GeoPoint> polygon;
    polygon.reserve(kCircleTemplateSegments);
    for (int i = 0; i < kCircleTemplateSegments; ++i) {
        const float a = static_cast<float>(i) * 2.0f * static_cast<float>(M_PI) / static_cast<float>(kCircleTemplateSegments);
        polygon.append(toGeo(center + QVector3D(std::cos(a) * r, std::sin(a) * r, 0.0f)));
    }
    addSurveyShape(polygon, static_cast<double>(r) / kTemplateSpacingDivisor);
}

void MissionController::placeCorridorTemplate()
{
    if (!plan_) {
        return;
    }
    QVector3D center;
    float halfX = 0.0f;
    float halfY = 0.0f;
    if (!viewExtent(center, halfX, halfY)) {
        return;
    }
    const float hy = halfY * kTemplateCorridorFraction;
    const QVector<mission::GeoPoint> axis = {
        toGeo(center + QVector3D(0.0f, -hy, 0.0f)),
        toGeo(center + QVector3D(0.0f, hy, 0.0f))
    };
    const int insertIndex = -1;
    plan_->beginTransaction();
    const QString id = plan_->addCorridor(mission::pointsToVariant(axis), insertIndex);
    if (!id.isEmpty()) {
        const double width = std::max(2.0 * mission::kMinLineSpacing, static_cast<double>(halfX) * kTemplateCorridorWidthFraction);
        QVariantMap patch;
        patch.insert(QStringLiteral("width"), roundTenth(width));
        patch.insert(QStringLiteral("lineSpacing"), roundTenth(clampSpacing(width / kCorridorSpacingDivisor)));
        plan_->updateItem(id, patch);
    }
    plan_->endTransaction(true);
    if (!id.isEmpty()) {
        select(id, -1);
    }
}

void MissionController::fitToPlan()
{
    if (view_) {
        view_->missionFitInView();
    }
}

MissionController::Hit MissionController::hitTest(qreal x, qreal y) const
{
    Hit best;
    if (!plan_) {
        return best;
    }
    const QPointF target(x, y);
    const double radius = hitRadius();
    double bestDist = radius;

    auto consider = [&](HitKind kind, const QString& id, int index, int insertIndex, const QVector3D& world, double r, const QVector2D& offsetPx = QVector2D()) {
        const QPointF screen = toScreen(world) + QPointF(offsetPx.x(), offsetPx.y()) * renderScale();
        const double d = QLineF(screen, target).length();
        if (d <= r && d < bestDist) {
            bestDist = d;
            best.kind = kind;
            best.id = id;
            best.index = index;
            best.insertIndex = insertIndex;
            best.world = world;
        }
    };

    const ShapeHandles handles = selectedShapeHandles();
    if (handles.valid) {
        if (selectedVertex_ >= 0 && selectedVertex_ < handles.scene.size()) {
            consider(HitKind::DeleteVertex, selectedId_, selectedVertex_, -1, handles.scene[selectedVertex_], radius, kDeleteBadgeOffset);
            if (best.kind != HitKind::None) {
                return best;
            }
        }
        for (int i = 0; i < handles.scene.size(); ++i) {
            consider(HitKind::Vertex, selectedId_, i, -1, handles.scene[i], radius);
        }
        if (best.kind != HitKind::None) {
            return best;
        }
        consider(HitKind::Center, selectedId_, -1, -1, handles.center, radius, handles.centerOffset);
        if (best.kind != HitKind::None) {
            return best;
        }
        for (int i = 0; i < handles.midpoints.size(); ++i) {
            consider(HitKind::Midpoint, selectedId_, -1, i + 1, handles.midpoints[i], radius * kMidpointHitFactor);
        }
        if (best.kind != HitKind::None) {
            return best;
        }
    }

    if (const std::optional<QVector3D> pointWorld = selectedPointWorld()) {
        consider(HitKind::DeleteVertex, selectedId_, -1, -1, *pointWorld, radius, kDeleteBadgeOffset);
        if (best.kind != HitKind::None) {
            return best;
        }
    }

    const auto& plan = plan_->plan();
    if (plan.home && plan.home->isValid()) {
        consider(HitKind::Vertex, kHomeId, 0, -1, toScene(*plan.home), radius);
    }
    for (const auto& item : plan.items) {
        if (const auto* w = std::get_if<mission::WaypointItem>(&item)) {
            consider(HitKind::Vertex, w->id, 0, -1, toScene(w->pos), radius);
        }
    }
    for (const auto& r : plan.rally) {
        consider(HitKind::Vertex, r.id, 0, -1, toScene(r.pos), radius);
    }
    if (best.kind != HitKind::None) {
        return best;
    }

    const QVector<Anchors> anchors = routeAnchors();
    for (int i = 0; i < anchors.size(); ++i) {
        if (anchors[i].valid) {
            consider(HitKind::RouteMidpoint, QString(), -1, i, anchors[i].mid, radius * kMidpointHitFactor);
        }
    }
    if (best.kind != HitKind::None) {
        return best;
    }

    for (const auto& item : plan.items) {
        std::visit([&](const auto& it) {
            using T = std::decay_t<decltype(it)>;
            if constexpr (std::is_same_v<T, mission::SurveyItem>) {
                QVector<QPointF> poly;
                poly.reserve(it.polygon.size());
                for (const auto& p : it.polygon) {
                    poly.append(toScreen(toScene(p)));
                }
                if (poly.size() >= 3 && pointInPolygon(target, poly) && best.kind == HitKind::None) {
                    best.kind = HitKind::Body;
                    best.id = it.id;
                }
            } else if constexpr (std::is_same_v<T, mission::CorridorItem>) {
                const double halfWidthPx = [&]() {
                    if (it.axis.size() < 2) {
                        return radius;
                    }
                    const QVector3D a = toScene(it.axis[0]);
                    const QVector3D b = toScene(it.axis[1]);
                    const double metres = (b - a).length();
                    const double px = QLineF(toScreen(a), toScreen(b)).length();
                    return metres > 1e-3 ? std::max(radius, px / metres * it.width * 0.5) : radius;
                }();
                for (int i = 0; i + 1 < it.axis.size(); ++i) {
                    const QPointF a = toScreen(toScene(it.axis[i]));
                    const QPointF b = toScreen(toScene(it.axis[i + 1]));
                    if (distPointSegment(target, a, b) <= halfWidthPx && best.kind == HitKind::None) {
                        best.kind = HitKind::Body;
                        best.id = it.id;
                    }
                }
            }
        }, item);
    }
    return best;
}

void MissionController::appendGeodesic(MissionLayer::RenderData& rd, const mission::GeoPoint& a, const mission::GeoPoint& b, const QColor& color, float widthPx, bool arrow) const
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
    const mission::GeoPoint m0 = mission::greatCirclePoint(a, b, 0.5);
    const mission::GeoPoint m1 = mission::greatCirclePoint(a, b, std::min(1.0, 0.5 + 0.5 / steps));
    if (perspective || std::fabs(m1.lon - m0.lon) <= 180.0) {
        rd.arrows.append({toScene(m0), toScene(m1)});
    }
}

void MissionController::appendGeneratedMarkers(MissionLayer::RenderData& rd, const mission::GeneratedPath& g, int ordinal, const QColor& color) const
{
    if (g.points.isEmpty()) {
        return;
    }
    MissionLayer::Marker entry;
    entry.world = toScene(g.points.first());
    entry.color = color;
    entry.sizePx = kEntryMarkerPx;
    entry.shape = MissionLayer::Marker::Shape::Diamond;
    entry.label = QString::number(ordinal);
    rd.markers.append(entry);
    if (g.points.size() > 1) {
        MissionLayer::Marker exitMark;
        exitMark.world = toScene(g.points.last());
        exitMark.color = color;
        exitMark.sizePx = kExitMarkerPx;
        exitMark.shape = MissionLayer::Marker::Shape::Ring;
        exitMark.label = QString::number(ordinal) + QStringLiteral(" out");
        rd.markers.append(exitMark);
    }
}

std::optional<QVector3D> MissionController::selectedPointWorld() const
{
    if (!plan_ || selectedId_.isEmpty()) {
        return std::nullopt;
    }
    const auto& plan = plan_->plan();
    if (selectedId_ == kHomeId) {
        if (plan.home && plan.home->isValid()) {
            return toScene(*plan.home);
        }
        return std::nullopt;
    }
    const int idx = plan_->indexOfItem(selectedId_);
    if (idx >= 0) {
        if (const auto* w = std::get_if<mission::WaypointItem>(&plan.items[idx])) {
            return w->pos.isValid() ? std::optional<QVector3D>(toScene(w->pos)) : std::nullopt;
        }
        return std::nullopt;
    }
    for (const auto& r : plan.rally) {
        if (r.id == selectedId_) {
            return toScene(r.pos);
        }
    }
    return std::nullopt;
}

QVector<MissionController::Anchors> MissionController::routeAnchors() const
{
    QVector<Anchors> out;
    if (!plan_) {
        return out;
    }
    const auto& plan = plan_->plan();
    const auto& generated = plan_->expanded().generated;

    std::optional<mission::GeoPoint> prevExit;
    if (plan.home && plan.home->isValid()) {
        prevExit = *plan.home;
    }

    for (const auto& item : plan.items) {
        std::optional<mission::GeoPoint> entry;
        std::optional<mission::GeoPoint> exit;
        if (const auto* w = std::get_if<mission::WaypointItem>(&item)) {
            if (w->pos.isValid()) {
                entry = w->pos;
                exit = entry;
            }
        } else {
            const auto g = generated.constFind(mission::itemId(item));
            if (g != generated.constEnd() && g->ok() && !g->points.isEmpty()) {
                entry = g->points.first();
                exit = g->points.last();
            }
        }
        Anchors a;
        if (prevExit && entry) {
            a.valid = true;
            a.mid = toScene(mission::greatCirclePoint(*prevExit, *entry, 0.5));
        }
        out.append(a);
        prevExit = exit;
    }
    return out;
}

QPointF MissionController::toScreen(const QVector3D& world) const
{
    const QRect viewport = view_->boundingRect().toRect();
    const QVector3D win = world.project(view_->m_camera->m_view * view_->m_model, view_->m_projection, viewport);
    return QPointF(win.x(), view_->height() - win.y());
}

QVector3D MissionController::toScene(const mission::GeoPoint& p) const
{
    GeoJsonCoord c;
    c.lat = p.lat;
    c.lon = p.lon;
    c.z = 0.0;
    c.hasZ = false;
    return view_->geojsonToScene(c);
}

mission::GeoPoint MissionController::toGeo(const QVector3D& p) const
{
    const GeoJsonCoord c = view_->sceneToGeojson(QVector3D(p.x(), p.y(), 0.0f));
    double lon = c.lon;
    if (view_->m_camera->getIsPerspective()) {
        const double refLon = view_->m_camera->viewLlaRef_.refLla.longitude;
        const double linearLon = refLon + static_cast<double>(p.y()) / CONSTANTS_RADIUS_OF_EARTH * 180.0 / M_PI;
        if (linearLon > 180.0) {
            lon = 180.0;
        } else if (linearLon < -180.0) {
            lon = -180.0;
        }
    }
    return mission::GeoPoint(std::clamp(c.lat, -kLatClampDeg, kLatClampDeg), std::clamp(lon, -180.0, 180.0));
}

QVector<mission::GeoPoint> MissionController::itemVertices(const QString& id) const
{
    const int idx = plan_ ? plan_->indexOfItem(id) : -1;
    if (idx < 0) {
        return {};
    }
    return shapeVertices(plan_->plan().items[idx]);
}

bool MissionController::itemHasVertices(const QString& id) const
{
    const QString type = itemType(id);
    return type == QStringLiteral("survey") || type == QStringLiteral("corridor");
}

QString MissionController::itemType(const QString& id) const
{
    const int idx = plan_ ? plan_->indexOfItem(id) : -1;
    if (idx < 0) {
        return QString();
    }
    return QString::fromLatin1(mission::itemTypeName(plan_->plan().items[idx]));
}

double MissionController::hitRadius() const
{
    return kHitRadiusPx * renderScale();
}

void MissionController::beginDrag(const Hit& hit, const QVector3D& scenePoint)
{
    dragHit_ = hit;
    dragStartScene_ = scenePoint;
    dragStartVertices_.clear();
    dragMoved_ = false;

    if (hit.kind == HitKind::Center) {
        for (const auto& v : itemVertices(hit.id)) {
            dragStartVertices_.append(toScene(v));
        }
    } else if (hit.id == kHomeId) {
        if (plan_->plan().home) {
            dragStartVertices_.append(toScene(*plan_->plan().home));
        }
    } else if (itemHasVertices(hit.id)) {
        const auto verts = itemVertices(hit.id);
        if (hit.index >= 0 && hit.index < verts.size()) {
            dragStartVertices_.append(toScene(verts[hit.index]));
        }
    } else {
        dragStartVertices_.append(hit.world);
    }

    plan_->beginTransaction();
    setDragging(true);
}

void MissionController::endDrag()
{
    if (plan_) {
        plan_->endTransaction(true);
    }
    dragHit_ = Hit();
    dragStartVertices_.clear();
    setDragging(false);
}

void MissionController::setDragging(bool dragging)
{
    if (dragging_ == dragging) {
        return;
    }
    dragging_ = dragging;
    emit draggingChanged();
}

void MissionController::markDirty()
{
    dirty_ = true;
    if (view_) {
        view_->update();
    }
}

bool MissionController::viewExtent(QVector3D& center, float& halfX, float& halfY) const
{
    if (!view_ || !view_->m_camera) {
        return false;
    }
    const auto [minX, maxX, minY, maxY] = view_->getFieldViewDim();
    const bool valid = maxX > minX && maxY > minY && std::isfinite(minX) && std::isfinite(maxX) && std::isfinite(minY) && std::isfinite(maxY)
                       && (maxX - minX) < 1.0e6f && (maxY - minY) < 1.0e6f;
    if (valid) {
        center = QVector3D((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, 0.0f);
        halfX = (maxX - minX) * 0.5f;
        halfY = (maxY - minY) * 0.5f;
        return true;
    }
    const QVector3D lookAt = view_->m_camera->m_lookAt;
    const float dist = std::max(20.0f, view_->m_camera->m_distToFocusPoint);
    center = QVector3D(lookAt.x(), lookAt.y(), 0.0f);
    halfX = dist * 0.35f;
    halfY = dist * 0.35f * static_cast<float>(view_->width() / std::max<qreal>(1.0, view_->height()));
    return true;
}

void MissionController::rebuild()
{
    MissionLayer::RenderData rd;
    if (!plan_ || !layer_) {
        if (layer_) {
            layer_->setRenderData(rd);
        }
        return;
    }

    const auto& plan = plan_->plan();
    const auto& expanded = plan_->expanded();
    const bool hasContent = plan.home.has_value() || !plan.items.isEmpty() || !plan.rally.isEmpty() || !draft_.isEmpty();
    rd.enabled = editing_ && hasContent;
    if (!rd.enabled) {
        layer_->setRenderData(rd);
        return;
    }

    const float handlePx = kVertexHandlePx;
    const bool selectedIsHome = selectedId_ == kHomeId;

    QVector<mission::GeoPoint> routeGeo;
    if (plan.home && plan.home->isValid()) {
        routeGeo.append(*plan.home);
    }
    for (const auto& f : expanded.mission.items) {
        if (f.isNavigation()) {
            routeGeo.append(mission::GeoPoint(f.lat, f.lon));
        }
    }
    for (int i = 0; i + 1 < routeGeo.size(); ++i) {
        appendGeodesic(rd, routeGeo[i], routeGeo[i + 1], kRouteColor, 2.5f, true);
    }
    const bool returnsHome = !expanded.mission.items.isEmpty() && expanded.mission.items.last().command == mission::MavCmd::NavReturnToLaunch;
    if (returnsHome && plan.home && plan.home->isValid() && routeGeo.size() >= 2) {
        appendGeodesic(rd, routeGeo.last(), *plan.home, kReturnColor, 2.0f, true);
    }

    float flashWave = 0.0f;
    if (!flashId_.isEmpty()) {
        const float t = std::clamp(static_cast<float>(flashClock_.elapsed()) / static_cast<float>(kFlashMs), 0.0f, 1.0f);
        flashWave = 0.5f - 0.5f * std::cos(t * kFlashPulses * 2.0f * static_cast<float>(M_PI));
    }
    auto flashColor = [&](const QColor& c) {
        QColor out = c;
        out.setAlphaF(c.alphaF() * (1.0f - 0.9f * flashWave));
        return out;
    };
    auto dimFrom = [&](const QString& id, int fills, int lines, int arrows, int markers) {
        if (id != flashId_ || flashWave <= 0.0f) {
            return;
        }
        for (int i = fills; i < rd.fills.size(); ++i) rd.fills[i].color = flashColor(rd.fills[i].color);
        for (int i = lines; i < rd.lines.size(); ++i) rd.lines[i].color = flashColor(rd.lines[i].color);
        for (int i = markers; i < rd.markers.size(); ++i) rd.markers[i].color = flashColor(rd.markers[i].color);
        rd.arrows.resize(arrows);
    };

    int ordinal = 0;
    for (const auto& item : plan.items) {
        ++ordinal;
        const QString id = mission::itemId(item);
        const bool selected = id == selectedId_;
        const int fillsFrom = rd.fills.size();
        const int linesFrom = rd.lines.size();
        const int arrowsFrom = rd.arrows.size();
        const int markersFrom = rd.markers.size();
        std::visit([&](const auto& it) {
            using T = std::decay_t<decltype(it)>;
            if constexpr (std::is_same_v<T, mission::WaypointItem>) {
                appendHandle(rd, toScene(it.pos), kWaypointColor, kHandleSizePx, QString::number(ordinal), selected);
            } else if constexpr (std::is_same_v<T, mission::SurveyItem>) {
                const auto g = expanded.generated.constFind(id);
                const bool shapeOk = g != expanded.generated.constEnd() && g->ok();
                if (shapeOk) {
                    MissionLayer::Fill fill;
                    for (const auto& p : it.polygon) {
                        fill.ring.append(toScene(p));
                    }
                    fill.color = selected ? QColor(kSurveyFill.red(), kSurveyFill.green(), kSurveyFill.blue(), 70) : kSurveyFill;
                    rd.fills.append(fill);
                }
                const QColor outlineColor = selected ? kSelectedColor : (shapeOk ? kSurveyStroke : kErrorStroke);
                const float outlineWidth = selected ? 2.5f : 1.5f;
                for (int i = 0; i < it.polygon.size(); ++i) {
                    appendGeodesic(rd, it.polygon[i], it.polygon[(i + 1) % it.polygon.size()], outlineColor, outlineWidth, false);
                }
                if (shapeOk) {
                    appendGeneratedMarkers(rd, *g, ordinal, selected ? kSelectedColor : kSurveyStroke);
                }
            } else {
                const auto g = expanded.generated.constFind(id);
                const bool shapeOk = g != expanded.generated.constEnd() && g->ok();
                if (shapeOk && g->band.size() >= 3) {
                    MissionLayer::Fill band;
                    band.ring.reserve(g->band.size());
                    for (const auto& p : g->band) {
                        band.ring.append(toScene(p));
                    }
                    band.color = selected ? QColor(kCorridorFill.red(), kCorridorFill.green(), kCorridorFill.blue(), 70) : kCorridorFill;
                    MissionLayer::Line outline;
                    outline.points = band.ring;
                    outline.loop = true;
                    outline.color = selected ? kSelectedColor : kCorridorStroke;
                    outline.widthPx = selected ? 2.0f : 1.0f;
                    rd.fills.append(band);
                    rd.lines.append(outline);
                }
                const QColor axisColor = selected ? kSelectedColor : (shapeOk ? kCorridorStroke : kErrorStroke);
                for (int i = 0; i + 1 < it.axis.size(); ++i) {
                    appendGeodesic(rd, it.axis[i], it.axis[i + 1], axisColor, 1.5f, false);
                }
                if (shapeOk) {
                    appendGeneratedMarkers(rd, *g, ordinal, selected ? kSelectedColor : kCorridorStroke);
                }
            }
        }, item);
        dimFrom(id, fillsFrom, linesFrom, arrowsFrom, markersFrom);
    }

    if (plan.home && plan.home->isValid()) {
        const int markersFrom = rd.markers.size();
        appendHandle(rd, toScene(*plan.home), kHomeColor, kHandleSizePx + 1.0f, QStringLiteral("H"), selectedIsHome);
        dimFrom(kHomeId, rd.fills.size(), rd.lines.size(), rd.arrows.size(), markersFrom);
    }

    for (const auto& r : plan.rally) {
        const int markersFrom = rd.markers.size();
        appendHandle(rd, toScene(r.pos), kRallyColor, kHandleSizePx, QStringLiteral("R"), r.id == selectedId_);
        dimFrom(r.id, rd.fills.size(), rd.lines.size(), rd.arrows.size(), markersFrom);
    }

    if (editing_) {
        const ShapeHandles handles = selectedShapeHandles();
        if (handles.valid) {
            for (int i = 0; i < handles.scene.size(); ++i) {
                appendHandle(rd, handles.scene[i], kHandleColor, handlePx, QString(), i == selectedVertex_);
            }
            for (const auto& mid : handles.midpoints) {
                MissionLayer::Marker m;
                m.world = mid;
                m.color = kHandleColor;
                m.sizePx = handlePx * 0.8f;
                m.shape = MissionLayer::Marker::Shape::Plus;
                m.topmost = true;
                rd.markers.append(m);
            }
            MissionLayer::Marker bg;
            bg.world = handles.center;
            bg.offsetPx = handles.centerOffset;
            bg.color = kSelectedColor;
            bg.sizePx = handlePx + 6.0f;
            bg.shape = MissionLayer::Marker::Shape::Circle;
            bg.topmost = true;
            rd.markers.append(bg);
            MissionLayer::Marker glyph = bg;
            glyph.color = kHandleOutline;
            glyph.sizePx = handlePx + 1.0f;
            glyph.shape = MissionLayer::Marker::Shape::Move;
            glyph.halo = false;
            rd.markers.append(glyph);
            if (selectedVertex_ >= 0 && selectedVertex_ < handles.scene.size()) {
                appendDeleteBadge(rd, handles.scene[selectedVertex_], handlePx, kDeleteBadge, kDeleteBadgeOffset);
            }
        }

        std::optional<QVector3D> pointWorld = selectedPointWorld();
        if (pointWorld) {
            appendDeleteBadge(rd, *pointWorld, kHandleSizePx, kDeleteBadge, kDeleteBadgeOffset);
        }

        for (const auto& a : routeAnchors()) {
            if (!a.valid) {
                continue;
            }
            MissionLayer::Marker m;
            m.world = a.mid;
            m.color = kHandleColor;
            m.sizePx = handlePx * 0.8f;
            m.shape = MissionLayer::Marker::Shape::Plus;
            m.topmost = true;
            rd.markers.append(m);
        }
    }

    if (editing_ && !draft_.isEmpty()) {
        const bool closed = tool_ == ToolSurveyTrace;
        const QColor draftColor = closed ? kSurveyStroke : kCorridorStroke;
        const int edgeCount = closed && draft_.size() >= 3 ? draft_.size() : draft_.size() - 1;
        for (int i = 0; i < edgeCount; ++i) {
            appendGeodesic(rd, draft_[i], draft_[(i + 1) % draft_.size()], draftColor, 2.0f, false);
        }
        QVector<QVector3D> draftScene;
        draftScene.reserve(draft_.size());
        double minLat = 90.0, maxLat = -90.0, minLon = 180.0, maxLon = -180.0;
        for (const auto& g : std::as_const(draft_)) {
            draftScene.append(toScene(g));
            minLat = std::min(minLat, g.lat);
            maxLat = std::max(maxLat, g.lat);
            minLon = std::min(minLon, g.lon);
            maxLon = std::max(maxLon, g.lon);
        }
        const bool compact = (maxLat - minLat) < kDraftCompactDeg && (maxLon - minLon) < kDraftCompactDeg;
        if (closed && draftScene.size() >= 3 && compact) {
            MissionLayer::Fill f;
            f.ring = draftScene;
            f.color = kSurveyFill;
            rd.fills.append(f);
        }
        for (int i = 0; i < draftScene.size(); ++i) {
            MissionLayer::Marker m;
            m.world = draftScene[i];
            m.color = i == draftScene.size() - 1 ? kSelectedColor : kHandleColor;
            m.topmost = true;
            m.sizePx = handlePx;
            m.shape = MissionLayer::Marker::Shape::Circle;
            m.label = i == 0 ? QStringLiteral("1") : QString();
            rd.markers.append(m);
        }
    }

    layer_->setRenderData(rd);
}
