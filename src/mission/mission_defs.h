#pragma once

#include <cmath>

#include <QtGlobal>

namespace mission {

enum class MavCmd : int {
    NavWaypoint       = 16,
    NavLoiterUnlim    = 17,
    NavReturnToLaunch = 20,
    DoChangeSpeed     = 178
};

enum class MavFrame : int {
    Global               = 0,
    GlobalRelativeAlt    = 3,
    GlobalRelativeAltInt = 6
};

enum class EndAction : int {
    Rtl  = 0,
    Hold = 1
};

struct GeoPoint {
    double lat = NAN;
    double lon = NAN;

    GeoPoint() = default;
    GeoPoint(double la, double lo) : lat(la), lon(lo) {}

    bool isValid() const { return std::isfinite(lat) && std::isfinite(lon) && std::fabs(lat) <= 90.0 && std::fabs(lon) <= 180.0; }
};

constexpr int    kFormatVersion       = 1;
constexpr int    kMaxSurveyLines      = 500;
constexpr int    kMaxCorridorLines    = 200;
constexpr double kMinLineSpacing      = 0.5;
constexpr double kMaxShapeExtentMeters = 1000000.0;
constexpr double kTurnPenaltySeconds  = 2.0;
constexpr double kTurnPenaltyAngleDeg = 45.0;
constexpr int    kUndoDepth           = 50;
constexpr int    kMaxMissionItems       = 700;
constexpr int    kWarnMissionItems      = 500;
constexpr double kWarnMissionHours      = 10.0;
constexpr double kWarnMissionLengthKm   = 100.0;
constexpr double kWarnSurveyAreaKm2     = 10.0;
constexpr double kMinPolygonAreaM2      = 1.0;
constexpr int    kMaxPlanItems          = 2000;
constexpr int    kMaxRallyPoints        = 500;
constexpr int    kMaxShapeVertices      = 10000;
constexpr qint64 kMaxMissionFileBytes   = 16 * 1024 * 1024;

} // namespace mission
