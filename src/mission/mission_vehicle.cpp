#include "mission_vehicle.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "autopilot_messages.h"
#include "mission_geometry.h"

namespace mission {

namespace {

constexpr uint16_t kCmdFenceVertexInclusion = 5001;
constexpr uint16_t kCmdFenceVertexExclusion = 5002;
constexpr uint16_t kCmdRallyPoint = 5100;

int32_t toDegE7(double degrees)
{
    return int32_t(std::llround(degrees * 1.0e7));
}

autopilot::MissionItem fromFlat(const FlatItem& f)
{
    autopilot::MissionItem m;
    m.command = uint16_t(f.command);
    m.frame = uint8_t(f.frame);
    m.param1 = float(f.param1);
    m.param2 = float(f.param2);
    m.param3 = float(f.param3);
    m.param4 = float(f.param4);
    m.x = toDegE7(f.lat);
    m.y = toDegE7(f.lon);
    m.z = float(f.alt);
    return m;
}

} // namespace

autopilot::MissionItems toVehicleMission(const FlatMission& mission)
{
    autopilot::MissionItems out;
    out.reserve(mission.items.size() + 1);

    autopilot::MissionItem home;
    home.command = uint16_t(MavCmd::NavWaypoint);
    home.frame = uint8_t(MavFrame::Global);
    home.current = 1;
    if (mission.home && mission.home->isValid()) {
        home.x = toDegE7(mission.home->lat);
        home.y = toDegE7(mission.home->lon);
    }
    out.append(home);

    for (const auto& f : mission.items) {
        out.append(fromFlat(f));
    }
    return out;
}

autopilot::MissionItems toVehicleFence(const FlatMission& mission)
{
    autopilot::MissionItems out;
    for (const auto& polygon : mission.fence) {
        if (polygon.ring.size() < kFenceMinVertices) {
            continue;
        }
        for (const auto& v : polygon.ring) {
            autopilot::MissionItem m;
            m.command = polygon.inclusion ? kCmdFenceVertexInclusion : kCmdFenceVertexExclusion;
            m.frame = uint8_t(MavFrame::Global);
            m.autocontinue = 0;
            m.param1 = float(polygon.ring.size());
            m.x = toDegE7(v.lat);
            m.y = toDegE7(v.lon);
            out.append(m);
        }
    }
    return out;
}

autopilot::MissionItems toVehicleRally(const FlatMission& mission)
{
    autopilot::MissionItems out;
    out.reserve(mission.rally.size());
    for (const auto& p : mission.rally) {
        autopilot::MissionItem m;
        m.command = kCmdRallyPoint;
        m.frame = uint8_t(MavFrame::GlobalRelativeAlt);
        m.autocontinue = 0;
        m.x = toDegE7(p.lat);
        m.y = toDegE7(p.lon);
        out.append(m);
    }
    return out;
}

autopilot::MissionBatches toVehicleUpload(const FlatMission& mission)
{
    return {
        { MavMissionTypeMission, toVehicleMission(mission) },
        { MavMissionTypeFence, toVehicleFence(mission) },
        { MavMissionTypeRally, toVehicleRally(mission) }
    };
}

namespace {

constexpr double kReturnToStartToleranceM = 0.5;
constexpr int32_t kPositionToleranceE7 = 1;
constexpr float kSpeedTolerance = 0.05f;

GeoPoint geoOf(const autopilot::MissionItem& m)
{
    return GeoPoint(double(m.x) / 1.0e7, double(m.y) / 1.0e7);
}

const autopilot::MissionBatch* batchOf(const autopilot::MissionBatches& batches, int missionType)
{
    for (const auto& b : batches) {
        if (b.missionType == missionType) {
            return &b;
        }
    }
    return nullptr;
}

bool samePosition(const autopilot::MissionItem& a, const autopilot::MissionItem& b)
{
    return std::llabs(int64_t(a.x) - int64_t(b.x)) <= kPositionToleranceE7
        && std::llabs(int64_t(a.y) - int64_t(b.y)) <= kPositionToleranceE7;
}

bool hasPosition(const autopilot::MissionItem& m)
{
    return (m.x != 0 || m.y != 0) && geoOf(m).isValid();
}

bool sameRouteItem(const autopilot::MissionItem& a, const autopilot::MissionItem& b)
{
    if (a.command != b.command) {
        return false;
    }
    switch (MavCmd(a.command)) {
    case MavCmd::NavWaypoint:
        return samePosition(a, b) && std::fabs(a.param1 - b.param1) < 1.0f;
    case MavCmd::DoChangeSpeed:
        return std::fabs(a.param2 - b.param2) < kSpeedTolerance;
    case MavCmd::NavReturnToLaunch:
    case MavCmd::NavLoiterUnlim:
        return true;
    }
    return samePosition(a, b);
}

void importRoute(const autopilot::MissionItems& items, VehicleImport& out)
{
    MissionPlan& plan = out.plan;
    std::optional<double> pendingSpeed;
    bool endSeen = false;
    bool navSeen = false;
    std::optional<GeoPoint> endLoiter;

    int lastCommand = int(items.size()) - 1;
    while (lastCommand > 0 && MavCmd(items.at(lastCommand).command) == MavCmd::DoChangeSpeed) {
        --lastCommand;
    }
    int holdTail = -1;
    if (lastCommand > 0 && MavCmd(items.at(lastCommand).command) == MavCmd::NavLoiterUnlim && !hasPosition(items.at(lastCommand))) {
        holdTail = lastCommand;
        --lastCommand;
        while (lastCommand > 0 && MavCmd(items.at(lastCommand).command) == MavCmd::DoChangeSpeed) {
            --lastCommand;
        }
    }

    for (int i = 1; i < items.size(); ++i) {
        const autopilot::MissionItem& m = items.at(i);
        switch (MavCmd(m.command)) {
        case MavCmd::DoChangeSpeed:
            if (m.param2 > 0.0f) {
                if (!navSeen) {
                    plan.settings.cruiseSpeed = m.param2;
                } else {
                    pendingSpeed = m.param2;
                }
            }
            continue;
        case MavCmd::NavWaypoint: {
            const GeoPoint p = geoOf(m);
            if (!hasPosition(m)) {
                ++out.skippedRouteCommands;
                continue;
            }
            navSeen = true;
            if (!plan.home) {
                plan.home = p;
                pendingSpeed.reset();
                continue;
            }
            WaypointItem w;
            w.pos = p;
            w.holdTime = std::max(0.0, double(m.param1));
            w.acceptRadius = std::max(0.0, double(m.param2));
            w.speed = pendingSpeed;
            pendingSpeed.reset();
            plan.items.append(w);
            continue;
        }
        case MavCmd::NavReturnToLaunch:
        case MavCmd::NavLoiterUnlim:
            if (i == holdTail) {
                continue;
            }
            if (i != lastCommand) {
                ++out.skippedRouteCommands;
                continue;
            }
            plan.settings.endAction = MavCmd(m.command) == MavCmd::NavReturnToLaunch ? EndAction::Rtl : EndAction::Hold;
            if (MavCmd(m.command) == MavCmd::NavLoiterUnlim && hasPosition(m)) {
                endLoiter = geoOf(m);
            }
            endSeen = true;
            continue;
        }
        ++out.skippedRouteCommands;
    }

    const auto lastIsStart = [&plan]() {
        if (!plan.home || plan.items.isEmpty()) {
            return false;
        }
        const auto* last = std::get_if<WaypointItem>(&plan.items.last());
        return last && last->holdTime <= 0.0 && !last->speed && geoDistance(last->pos, *plan.home) <= kReturnToStartToleranceM;
    };
    if (endSeen) {
        if (endLoiter && plan.home && geoDistance(*endLoiter, *plan.home) <= kReturnToStartToleranceM && lastIsStart()) {
            plan.items.removeLast();
            plan.settings.endAction = EndAction::ReturnToStart;
        }
        return;
    }
    if (lastIsStart()) {
        plan.items.removeLast();
        plan.settings.endAction = EndAction::ReturnToStart;
        return;
    }
    plan.settings.endAction = holdTail >= 0 ? EndAction::Hold : EndAction::None;
}

void importFence(const autopilot::MissionItems& items, VehicleImport& out)
{
    for (int i = 0; i < items.size();) {
        const autopilot::MissionItem& m = items.at(i);
        const bool vertex = m.command == kCmdFenceVertexInclusion || m.command == kCmdFenceVertexExclusion;
        const int count = vertex ? int(std::lround(m.param1)) : 0;
        bool run = vertex && count >= kFenceMinVertices && i + count <= items.size();
        for (int k = 1; run && k < count; ++k) {
            const autopilot::MissionItem& v = items.at(i + k);
            run = v.command == m.command && std::lround(v.param1) == count;
        }
        if (!run) {
            ++out.skippedFenceItems;
            ++i;
            continue;
        }
        FencePolygon f;
        f.inclusion = m.command == kCmdFenceVertexInclusion;
        for (int k = 0; k < count; ++k) {
            f.ring.append(geoOf(items.at(i + k)));
        }
        out.plan.fence.append(f);
        i += count;
    }
}

void importRally(const autopilot::MissionItems& items, VehicleImport& out)
{
    for (const auto& m : items) {
        const GeoPoint p = geoOf(m);
        if (m.command != kCmdRallyPoint || !hasPosition(m)) {
            continue;
        }
        RallyItem r;
        r.pos = p;
        out.plan.rally.append(r);
    }
}

bool sameList(const autopilot::MissionBatches& vehicle, const autopilot::MissionBatches& planned, int missionType)
{
    const autopilot::MissionBatch* a = batchOf(vehicle, missionType);
    const autopilot::MissionBatch* b = batchOf(planned, missionType);
    if (!a || !b) {
        return false;
    }
    const bool route = missionType == MavMissionTypeMission;
    const int first = route ? 1 : 0;
    if (a->items.size() != b->items.size() || (route && a->items.isEmpty())) {
        return false;
    }
    for (int i = first; i < a->items.size(); ++i) {
        const auto& x = a->items.at(i);
        const auto& y = b->items.at(i);
        if (route ? !sameRouteItem(x, y)
                  : (x.command != y.command || !samePosition(x, y) || std::lround(x.param1) != std::lround(y.param1))) {
            return false;
        }
    }
    return true;
}

} // namespace

VehicleImport fromVehicle(const autopilot::MissionBatches& batches, double fallbackCruiseSpeed)
{
    VehicleImport out;
    out.plan.settings.cruiseSpeed = fallbackCruiseSpeed;
    if (const auto* route = batchOf(batches, MavMissionTypeMission)) {
        importRoute(route->items, out);
    }
    if (const auto* fence = batchOf(batches, MavMissionTypeFence)) {
        importFence(fence->items, out);
    }
    if (const auto* rally = batchOf(batches, MavMissionTypeRally)) {
        importRally(rally->items, out);
    }
    out.empty = !out.plan.home && out.plan.items.isEmpty() && out.plan.fence.isEmpty() && out.plan.rally.isEmpty();
    return out;
}

bool sameOnVehicle(const autopilot::MissionBatches& vehicle, const autopilot::MissionBatches& planned)
{
    return sameList(vehicle, planned, MavMissionTypeMission)
        && sameList(vehicle, planned, MavMissionTypeFence)
        && sameList(vehicle, planned, MavMissionTypeRally);
}

bool sameRouteOnVehicle(const autopilot::MissionBatches& vehicle, const autopilot::MissionBatches& planned)
{
    return sameList(vehicle, planned, MavMissionTypeMission);
}

bool sameReadOnVehicle(const autopilot::MissionBatches& vehicle, const autopilot::MissionBatches& planned)
{
    if (!sameList(vehicle, planned, MavMissionTypeMission)) {
        return false;
    }
    for (int type : { int(MavMissionTypeFence), int(MavMissionTypeRally) }) {
        if (batchOf(vehicle, type) && !sameList(vehicle, planned, type)) {
            return false;
        }
    }
    return true;
}

} // namespace mission
