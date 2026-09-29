#include "mission_expander.h"

#include <algorithm>
#include <cmath>
#include <functional>

#include <QCoreApplication>
#include <QPointF>

#include "mission_geometry.h"

namespace mission {

namespace {

FlatItem waypointAt(const GeoPoint& p, const QString& sourceId, double hold = 0.0, double radius = 0.0)
{
    FlatItem f;
    f.command = MavCmd::NavWaypoint;
    f.frame = MavFrame::GlobalRelativeAltInt;
    f.param1 = hold;
    f.param2 = radius;
    f.param4 = NAN;
    f.lat = p.lat;
    f.lon = p.lon;
    f.alt = 0.0;
    f.sourceId = sourceId;
    return f;
}

bool pointInLocalPolygon(const QPointF& p, const QVector<QPointF>& poly)
{
    bool inside = false;
    for (int i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const QPointF& a = poly[i];
        const QPointF& b = poly[j];
        const bool crosses = ((a.y() > p.y()) != (b.y() > p.y()))
                             && (p.x() < (b.x() - a.x()) * (p.y() - a.y()) / (b.y() - a.y() + 1e-12) + a.x());
        if (crosses) {
            inside = !inside;
        }
    }
    return inside;
}

FlatItem changeSpeed(double speed, const QString& sourceId)
{
    FlatItem f;
    f.command = MavCmd::DoChangeSpeed;
    f.frame = MavFrame::GlobalRelativeAltInt;
    f.param1 = 1.0;
    f.param2 = speed;
    f.param3 = -1.0;
    f.sourceId = sourceId;
    return f;
}

} // namespace

ExpandResult expandPlan(const MissionPlan& plan)
{
    ExpandResult res;
    FlatMission& m = res.mission;
    m.home = plan.home;

    std::optional<GeoPoint> previous = plan.home;

    if (!plan.items.isEmpty() && plan.settings.cruiseSpeed > 0.0) {
        m.items.append(changeSpeed(plan.settings.cruiseSpeed, QString()));
    }
    const bool homeValid = plan.home && plan.home->isValid();

    auto pushNav = [&](FlatItem f) {
        m.items.append(f);
        previous = GeoPoint(f.lat, f.lon);
    };

    auto pushGenerated = [&](const GeneratedPath& g, const QString& id) {
        const bool allValid = std::all_of(g.points.cbegin(), g.points.cend(), [](const GeoPoint& p) { return p.isValid(); });
        if (!allValid) {
            res.issues.append(PlanIssue{id, QCoreApplication::translate("MissionPlan", "Generated points leave the valid coordinate range"), IssueLevel::Error});
            return;
        }
        for (const auto& p : g.points) {
            pushNav(waypointAt(p, id));
        }
    };

    auto withShapeSpeed = [&](const std::optional<double>& speed, const QString& id, const std::function<void()>& body) {
        if (speed && *speed > 0.0) {
            m.items.append(changeSpeed(*speed, id));
        }
        body();
        if (speed && *speed > 0.0 && plan.settings.cruiseSpeed > 0.0) {
            m.items.append(changeSpeed(plan.settings.cruiseSpeed, id));
        }
    };

    if (homeValid && !plan.items.isEmpty()) {
        pushNav(waypointAt(*plan.home, kHomeItemId));
    }

    for (const auto& item : plan.items) {
        std::visit([&](const auto& it) {
            using T = std::decay_t<decltype(it)>;
            if constexpr (std::is_same_v<T, WaypointItem>) {
                if (!it.pos.isValid()) {
                    res.issues.append(PlanIssue{it.id, QCoreApplication::translate("MissionPlan", "Invalid position"), IssueLevel::Error});
                    return;
                }
                if (it.speed && *it.speed > 0.0) {
                    m.items.append(changeSpeed(*it.speed, it.id));
                }
                pushNav(waypointAt(it.pos, it.id, it.holdTime, it.acceptRadius));
            } else if constexpr (std::is_same_v<T, SurveyItem>) {
                GeneratedPath g = generateSurvey(it, previous);
                if (!g.ok()) {
                    res.issues.append(PlanIssue{it.id, g.error, IssueLevel::Error});
                } else {
                    if (g.spacingClamped) {
                        res.issues.append(PlanIssue{it.id, QCoreApplication::translate("MissionPlan", "Line spacing raised to %1 m to stay within %2 lines").arg(g.effectiveSpacing, 0, 'f', 1).arg(kMaxSurveyLines), IssueLevel::Warning});
                    }
                    if (g.areaSquareMeters > kWarnSurveyAreaKm2 * 1.0e6) {
                        res.issues.append(PlanIssue{it.id, QCoreApplication::translate("MissionPlan", "Survey area %1 km² is unusually large").arg(g.areaSquareMeters / 1.0e6, 0, 'f', 1), IssueLevel::Warning});
                    }
                    withShapeSpeed(it.speed, it.id, [&]() { pushGenerated(g, it.id); });
                }
                res.generated.insert(it.id, g);
            } else {
                GeneratedPath g = generateCorridor(it, previous);
                if (!g.ok()) {
                    res.issues.append(PlanIssue{it.id, g.error, IssueLevel::Error});
                } else {
                    if (g.spacingClamped) {
                        res.issues.append(PlanIssue{it.id, QCoreApplication::translate("MissionPlan", "Line spacing raised to %1 m to stay within %2 lines").arg(g.effectiveSpacing, 0, 'f', 1).arg(kMaxCorridorLines), IssueLevel::Warning});
                    }
                    for (const auto& w : g.warnings) {
                        res.issues.append(PlanIssue{it.id, w, IssueLevel::Warning});
                    }
                    withShapeSpeed(it.speed, it.id, [&]() { pushGenerated(g, it.id); });
                }
                res.generated.insert(it.id, g);
            }
        }, item);
    }

    const bool hasNav = std::any_of(m.items.cbegin(), m.items.cend(), [](const FlatItem& f) { return f.isNavigation() && f.sourceId != kHomeItemId; });
    if (!hasNav) {
        m.items.clear();
    } else if (plan.settings.endAction == EndAction::ReturnToStart) {
        if (homeValid) {
            pushNav(waypointAt(*plan.home, kHomeItemId));
        }
    } else {
        FlatItem end;
        end.frame = MavFrame::GlobalRelativeAltInt;
        if (plan.settings.endAction == EndAction::Rtl) {
            end.command = MavCmd::NavReturnToLaunch;
        } else {
            end.command = MavCmd::NavLoiterUnlim;
            end.lat = previous ? previous->lat : 0.0;
            end.lon = previous ? previous->lon : 0.0;
        }
        m.items.append(end);
    }

    for (const auto& r : plan.rally) {
        if (r.pos.isValid()) {
            m.rally.append(r.pos);
        }
    }

    m.fence = plan.fence;
    int outsideInclusion = 0;
    int insideExclusion = 0;
    bool anyInclusion = false;
    QVector<QVector<QPointF>> inclusionLocal;
    QVector<LocalPlane> inclusionPlanes;
    QVector<QVector<QPointF>> exclusionLocal;
    QVector<LocalPlane> exclusionPlanes;
    for (const auto& f : plan.fence) {
        if (f.ring.size() < kFenceMinVertices) {
            res.issues.append(PlanIssue{f.id, QCoreApplication::translate("MissionPlan", "Fence needs at least %1 vertices").arg(kFenceMinVertices), IssueLevel::Error});
            continue;
        }
        LocalPlane plane(centroid(f.ring));
        QVector<QPointF> local;
        local.reserve(f.ring.size());
        for (const auto& p : f.ring) {
            local.append(plane.toLocal(p));
        }
        if (polygonSelfIntersects(local)) {
            res.issues.append(PlanIssue{f.id, QCoreApplication::translate("MissionPlan", "Fence polygon is self-intersecting"), IssueLevel::Error});
            continue;
        }
        if (f.inclusion) {
            anyInclusion = true;
            inclusionLocal.append(local);
            inclusionPlanes.append(plane);
        } else {
            exclusionLocal.append(local);
            exclusionPlanes.append(plane);
        }
    }
    for (const auto& f : m.items) {
        if (!f.isNavigation()) {
            continue;
        }
        const GeoPoint p(f.lat, f.lon);
        if (anyInclusion) {
            bool inside = false;
            for (int i = 0; i < inclusionLocal.size() && !inside; ++i) {
                inside = pointInLocalPolygon(inclusionPlanes[i].toLocal(p), inclusionLocal[i]);
            }
            if (!inside) {
                ++outsideInclusion;
            }
        }
        for (int i = 0; i < exclusionLocal.size(); ++i) {
            if (pointInLocalPolygon(exclusionPlanes[i].toLocal(p), exclusionLocal[i])) {
                ++insideExclusion;
                break;
            }
        }
    }
    if (outsideInclusion > 0) {
        res.issues.append(PlanIssue{QString(), QCoreApplication::translate("MissionPlan", "%1 waypoints lie outside the inclusion fence").arg(outsideInclusion), IssueLevel::Warning});
    }
    if (insideExclusion > 0) {
        res.issues.append(PlanIssue{QString(), QCoreApplication::translate("MissionPlan", "%1 waypoints lie inside an exclusion zone").arg(insideExclusion), IssueLevel::Warning});
    }

    res.estimates = estimateMission(m, plan.settings);
    const int itemCount = m.items.size() + (m.home ? 1 : 0);
    if (itemCount > kMaxMissionItems) {
        res.issues.append(PlanIssue{QString(), QCoreApplication::translate("MissionPlan", "%1 mission items exceed the flight controller capacity of %2").arg(itemCount).arg(kMaxMissionItems), IssueLevel::Error});
    } else if (itemCount > kWarnMissionItems) {
        res.issues.append(PlanIssue{QString(), QCoreApplication::translate("MissionPlan", "%1 mission items, close to the flight controller capacity of %2").arg(itemCount).arg(kMaxMissionItems), IssueLevel::Warning});
    }
    if (res.estimates.timeSeconds > kWarnMissionHours * 3600.0) {
        res.issues.append(PlanIssue{QString(), QCoreApplication::translate("MissionPlan", "Estimated time %1 h exceeds %2 h").arg(res.estimates.timeSeconds / 3600.0, 0, 'f', 1).arg(kWarnMissionHours, 0, 'f', 0), IssueLevel::Warning});
    }
    if (res.estimates.lengthMeters > kWarnMissionLengthKm * 1000.0) {
        res.issues.append(PlanIssue{QString(), QCoreApplication::translate("MissionPlan", "Route length %1 km exceeds %2 km").arg(res.estimates.lengthMeters / 1000.0, 0, 'f', 1).arg(kWarnMissionLengthKm, 0, 'f', 0), IssueLevel::Warning});
    }
    if (!plan.home && !m.items.isEmpty()) {
        res.issues.append(PlanIssue{QString(), QCoreApplication::translate("MissionPlan", "Start point is not set"), IssueLevel::Error});
    }
    return res;
}

QStringList ExpandResult::warnings() const
{
    QStringList out;
    for (const auto& i : issues) {
        out.append(i.itemId.isEmpty() ? i.text : i.itemId + QStringLiteral(": ") + i.text);
    }
    return out;
}

MissionEstimates estimateMission(const FlatMission& mission, const PlanSettings& settings)
{
    MissionEstimates e;
    e.flatItemCount = mission.items.size();

    std::optional<GeoPoint> prev = mission.home;
    std::optional<GeoPoint> prevPrev;
    double speed = settings.cruiseSpeed > 0.0 ? settings.cruiseSpeed : 1.0;

    for (const auto& f : mission.items) {
        if (f.command == MavCmd::DoChangeSpeed) {
            if (f.param2 > 0.0) {
                speed = f.param2;
            }
            continue;
        }
        if (!f.isNavigation()) {
            continue;
        }
        ++e.waypointCount;
        const GeoPoint here(f.lat, f.lon);
        bool advanced = true;
        if (prev && prev->isValid()) {
            const double d = geoDistance(*prev, here);
            advanced = d > 0.0;
            e.lengthMeters += d;
            e.timeSeconds += d / speed;
            if (advanced && prevPrev && prevPrev->isValid()) {
                double turn = std::fabs(bearingDeg(*prev, here) - bearingDeg(*prevPrev, *prev));
                if (turn > 180.0) {
                    turn = 360.0 - turn;
                }
                if (turn > kTurnPenaltyAngleDeg) {
                    e.timeSeconds += kTurnPenaltySeconds;
                }
            }
        }
        if (f.param1 > 0.0) {
            e.timeSeconds += f.param1;
        }
        if (advanced) {
            prevPrev = prev;
            prev = here;
        }
    }

    return e;
}

} // namespace mission
