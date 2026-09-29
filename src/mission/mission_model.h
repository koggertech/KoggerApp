#pragma once

#include <optional>
#include <variant>

#include <QJsonObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVector>

#include "mission_defs.h"

namespace mission {

struct WaypointItem {
    QString  id;
    GeoPoint pos;
    double   holdTime = 0.0;
    double   acceptRadius = 0.0;
    std::optional<double> speed;
    QJsonObject extra;
};

struct SurveyItem {
    QString  id;
    QVector<GeoPoint> polygon;
    double   lineSpacing = 10.0;
    double   angleDeg = 0.0;
    double   turnaround = 0.0;
    std::optional<int> entryCorner;
    bool     crosshatch = false;
    std::optional<double> crosshatchSpacing;
    QJsonObject extra;
};

struct CorridorItem {
    QString  id;
    QVector<GeoPoint> axis;
    double   width = 20.0;
    double   lineSpacing = 10.0;
    double   turnaround = 0.0;
    std::optional<int> entryEnd;
    QJsonObject extra;
};

struct RallyItem {
    QString  id;
    GeoPoint pos;
    QJsonObject extra;
};

using MissionItem = std::variant<WaypointItem, SurveyItem, CorridorItem>;

struct PlanSettings {
    double    cruiseSpeed = 1.5;
    EndAction endAction = EndAction::Rtl;
};

struct MissionPlan {
    QString name;
    QString created;
    QString modified;
    PlanSettings settings;
    std::optional<GeoPoint> home;
    QVector<MissionItem> items;
    QVector<RallyItem> rally;
    QJsonObject extra;
};

inline const QString& itemId(const MissionItem& item)
{
    return std::visit([](const auto& it) -> const QString& { return it.id; }, item);
}

inline QString& itemId(MissionItem& item)
{
    return std::visit([](auto& it) -> QString& { return it.id; }, item);
}

inline const char* itemTypeName(const MissionItem& item)
{
    switch (item.index()) {
    case 0:  return "waypoint";
    case 1:  return "survey";
    default: return "corridor";
    }
}

inline QVariantList pointsToVariant(const QVector<GeoPoint>& pts)
{
    QVariantList out;
    out.reserve(pts.size());
    for (const auto& p : pts) {
        out.append(QVariant(QVariantList{p.lat, p.lon}));
    }
    return out;
}

struct FlatItem {
    MavCmd   command = MavCmd::NavWaypoint;
    MavFrame frame = MavFrame::GlobalRelativeAltInt;
    double   param1 = 0.0;
    double   param2 = 0.0;
    double   param3 = 0.0;
    double   param4 = 0.0;
    double   lat = 0.0;
    double   lon = 0.0;
    double   alt = 0.0;
    QString  sourceId;

    bool isNavigation() const { return command == MavCmd::NavWaypoint; }
};

struct FlatMission {
    std::optional<GeoPoint> home;
    QVector<FlatItem> items;
    QVector<GeoPoint> rally;
};

struct MissionEstimates {
    double lengthMeters = 0.0;
    double timeSeconds = 0.0;
    int    waypointCount = 0;
    int    flatItemCount = 0;
};

} // namespace mission
