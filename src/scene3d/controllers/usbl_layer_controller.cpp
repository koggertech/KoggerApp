#include "usbl_layer_controller.h"

#include <cmath>

#include "scene3d_view.h"
#include "usbl_layer.h"


UsblLayerController::UsblLayerController(GraphicsScene3dView* view, UsblLayer* layer, QObject* parent) :
    QObject(parent),
    view_(view),
    layer_(layer)
{
}

bool UsblLayerController::enabled() const
{
    return enabled_;
}

void UsblLayerController::setEnabled(bool enabled)
{
    if (enabled_ == enabled) {
        return;
    }
    enabled_ = enabled;
    if (layer_) {
        layer_->setVisible(enabled_);
    }
    Q_EMIT enabledChanged();
}

void UsblLayerController::setNodes(const QVariantList& nodes)
{
    nodes_.clear();
    order_.clear();

    for (const auto& entry : nodes) {
        const QVariantMap m = entry.toMap();
        bool ok = false;
        const int addr = m.value(QStringLiteral("addr"), -1).toInt(&ok);
        if (!ok || addr < 0) {
            continue;
        }
        // A plan can hold two nodes on one address. They are the same beacon and the same track,
        // so the first one named wins and the second is not a second row on the map.
        if (nodes_.contains(addr)) {
            continue;
        }

        Node n;
        n.active = m.value(QStringLiteral("active"), true).toBool();
        const QColor c(m.value(QStringLiteral("color")).toString());
        n.color = c.isValid() ? c : QColor(200, 60, 60);

        nodes_.insert(addr, n);
        order_.append(addr);
    }

    rebuild();
}

void UsblLayerController::clear()
{
    beacons_.clear();
    head_.clear();
    headYaw_ = NAN;
    rebuild();
}

void UsblLayerController::onUsblSolution(const IDBinUsblSolution::UsblSolution& solution)
{
    // 0xFF is the payload's "no address" marker and the protocol only ever uses 0..8, so anything
    // else is not a beacon this can name -- and an unnamed track is a track that can never be
    // matched to a row.
    const int addr = static_cast<int>(solution.id);
    if (addr < 0 || addr > 8) {
        return;
    }

    // The two positions in a solution are INDEPENDENTLY present. v1 carries no beacon N/E and a
    // partial fix can leave either lat/lon pair NAN, so each is recorded only if it arrived --
    // one flag for both would append the previous point again on behalf of the missing one, and
    // a track would grow a stationary tail nobody sailed.
    bool gotBeacon = false;
    bool gotHead = false;

    if (LLA(solution.beacon_latitude, solution.beacon_longitude).isCoordinatesValid()) {
        Fix f;
        f.lat = solution.beacon_latitude;
        f.lon = solution.beacon_longitude;
        f.depth = solution.beacon_depth;
        beacons_[addr].append(f);
        gotBeacon = true;
    }

    if (LLA(solution.usbl_latitude, solution.usbl_longitude).isCoordinatesValid()) {
        Fix f;
        f.lat = solution.usbl_latitude;
        f.lon = solution.usbl_longitude;
        head_.append(f);
        gotHead = true;
    }

    // Carried, not cleared: the head does not stop pointing where it pointed because one payload
    // version left the field out. v1 fills it from `heading`, v2 from `antennaYaw`, v0 off the wire.
    const bool gotYaw = std::isfinite(solution.usbl_yaw);
    if (gotYaw) {
        headYaw_ = solution.usbl_yaw;
    }

    if ((!gotBeacon && !gotHead && !gotYaw) || !view_ || !layer_) {
        return;
    }

    // Appending one projected point, rather than re-projecting the history behind it. The frame
    // this point is projected into is the same one every earlier point used -- rebuildIfNeeded
    // is what notices when that stops being true.
    if (gotBeacon) {
        const auto it = sceneIndex_.constFind(addr);
        if (it != sceneIndex_.constEnd()) {
            UsblLayer::Beacon& b = scene_.beacons[it.value()];
            const Fix& last = beacons_[addr].last();
            const QVector3D surface = toSurface(last.lat, last.lon);
            b.track.append(surface);
            applyFix(b, surface, last.depth);
        }
    }

    if (gotHead) {
        const Fix& last = head_.last();
        scene_.head.pos = toSurface(last.lat, last.lon);
        scene_.head.track.append(scene_.head.pos);
        scene_.head.hasFix = true;
    }

    if (gotYaw) {
        scene_.head.yawDeg = headYaw_;
        scene_.head.hasYaw = true;
    }

    push();
}

void UsblLayerController::rebuildIfNeeded()
{
    if (!view_) {
        return;
    }

    const bool persp = view_->m_camera->getIsPerspective();
    const bool viewRefChanged = (lastViewRef_ != view_->m_camera->viewLlaRef_);
    const bool perspChanged = (lastPerspective_ != persp);
    if (!viewRefChanged && !perspChanged) {
        return;
    }

    lastViewRef_ = view_->m_camera->viewLlaRef_;
    lastPerspective_ = persp;

    if (beacons_.isEmpty() && head_.isEmpty()) {
        return;
    }

    rebuild();
}

void UsblLayerController::rebuild()
{
    if (!view_ || !layer_) {
        return;
    }

    lastViewRef_ = view_->m_camera->viewLlaRef_;
    lastPerspective_ = view_->m_camera->getIsPerspective();

    UsblLayer::RenderData out;
    sceneIndex_.clear();

    // Plan order, so the operator's list order is the draw order: the node they put first is the
    // one on top where two beacons overlap.
    for (int addr : std::as_const(order_)) {
        const Node& node = nodes_[addr];

        UsblLayer::Beacon b;
        b.addr = addr;
        b.color = node.color;
        b.active = node.active;

        const auto it = beacons_.constFind(addr);
        if (it != beacons_.constEnd()) {
            b.track.reserve(it.value().size());
            for (const Fix& f : it.value()) {
                b.track.append(toSurface(f.lat, f.lon));
            }
            if (!b.track.isEmpty()) {
                applyFix(b, b.track.last(), it.value().last().depth);
            }
        }

        sceneIndex_.insert(addr, out.beacons.size());
        out.beacons.append(std::move(b));
    }

    out.head.track.reserve(head_.size());
    for (const Fix& f : head_) {
        out.head.track.append(toSurface(f.lat, f.lon));
    }
    if (!out.head.track.isEmpty()) {
        out.head.pos = out.head.track.last();
        out.head.hasFix = true;
    }
    out.head.yawDeg = headYaw_;
    out.head.hasYaw = std::isfinite(headYaw_);

    scene_ = std::move(out);
    push();
}

void UsblLayerController::push()
{
    if (layer_) {
        layer_->setRenderData(scene_);
    }
}

QVector3D UsblLayerController::toSurface(double latDeg, double lonDeg) const
{
    GeoJsonCoord c;
    c.lat = latDeg;
    c.lon = lonDeg;
    c.z = 0.0;
    c.hasZ = false;

    const QVector3D p = view_->geojsonToScene(c);
    // Pinned rather than trusted: the ball and its track are a CHART symbol, and the drop line
    // below only reads as a depth if the thing it hangs from is on the surface.
    return QVector3D(p.x(), p.y(), 0.0f);
}

void UsblLayerController::applyFix(UsblLayer::Beacon& beacon, const QVector3D& surface, float depth)
{
    beacon.surface = surface;
    beacon.hasFix = true;
    // No depth is not zero depth. Without one there is nothing to hang, so the beacon is just a
    // mark on the chart -- which is the truthful drawing of "we know where, not how deep".
    beacon.hasDeep = std::isfinite(depth);
    beacon.deep = beacon.hasDeep ? QVector3D(surface.x(), surface.y(), -depth) : surface;
}
