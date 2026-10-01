#pragma once

#include "mission_model.h"
#include "mission_transfer.h"

namespace mission {

/**
 * Flat mission as the MAVLink mission list for ArduPilot: item 0 is the home slot (the start
 * point, as QGroundControl sends the planned home), then every FlatItem in order with its
 * MAV_CMD / MAV_FRAME numbers and params unchanged and coordinates scaled to degE7.
 */
autopilot::MissionItems toVehicleMission(const FlatMission& mission);

/**
 * Fence polygons as MAV_MISSION_TYPE_FENCE items: one NAV_FENCE_POLYGON_VERTEX_INCLUSION /
 * _EXCLUSION per vertex, param1 = vertex count, frame GLOBAL (the QGroundControl layout).
 * Polygons with fewer than kFenceMinVertices vertices are skipped.
 */
autopilot::MissionItems toVehicleFence(const FlatMission& mission);

/** Rally points as MAV_MISSION_TYPE_RALLY items: NAV_RALLY_POINT, frame GLOBAL_RELATIVE_ALT, altitude 0. */
autopilot::MissionItems toVehicleRally(const FlatMission& mission);

/** Route, fence and rally in upload order; empty fence / rally batches clear those lists on the vehicle. */
autopilot::MissionBatches toVehicleUpload(const FlatMission& mission);

struct VehicleImport {
    MissionPlan plan;
    bool empty = true;
    int skippedRouteCommands = 0;
    int skippedFenceItems = 0;
};

/**
 * Lists read from the vehicle as an editable plan without ids: route item 0 (the vehicle's home)
 * is dropped, the first NAV_WAYPOINT becomes the start point, the others waypoints with hold,
 * accept radius and the speed of a DO_CHANGE_SPEED right before them (the one before the first
 * waypoint is the cruise speed). A final RTL / LOITER_UNLIM / waypoint back at the start becomes
 * the end action; without one the end action is None (the autopilot's own end behaviour).
 * Fence vertex runs become polygons, rally points rally items. Commands the editor has no item
 * for are counted, not kept.
 */
VehicleImport fromVehicle(const autopilot::MissionBatches& batches, double fallbackCruiseSpeed);

/**
 * Whether the vehicle holds exactly what @p planned would upload. Normalised comparison: the
 * route home slot is skipped (the vehicle writes its own home there), frames are ignored,
 * positions must match to 1e-7°, hold times to the second, speeds to 0.05 m/s; accept radius and
 * other params are not compared, because ArduPilot does not store every param back unchanged.
 * A list missing on either side counts as different.
 */
bool sameOnVehicle(const autopilot::MissionBatches& vehicle, const autopilot::MissionBatches& planned);

} // namespace mission
