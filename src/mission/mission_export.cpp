#include "mission_export.h"

#include <QFile>

#include "mission_file_io.h"

#include <cmath>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace mission {
namespace Export {

namespace {

constexpr int kFirmwareArduPilot = 3;
constexpr int kVehicleSurfaceBoat = 11;
constexpr int kAltitudeModeRelative = 1;

QJsonValue jsonParam(double v)
{
    return std::isfinite(v) ? QJsonValue(v) : QJsonValue(QJsonValue::Null);
}

QString num(double v, int decimals)
{
    return QString::number(std::isfinite(v) ? v : 0.0, 'f', decimals);
}

bool writeText(const QString& path, const QByteArray& bytes, QString* outError)
{
    if (FileIo::isOpaqueUri(path)) {
        QFile plain(path);
        if (!plain.open(QIODevice::WriteOnly | QIODevice::Truncate) || plain.write(bytes) != bytes.size()) {
            if (outError) *outError = QCoreApplication::translate("MissionPlan", "export: cannot open file for writing (%1)").arg(plain.errorString());
            return false;
        }
        return true;
    }
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) *outError = QCoreApplication::translate("MissionPlan", "export: cannot open file for writing (%1)").arg(f.errorString());
        return false;
    }
    if (f.write(bytes) != bytes.size() || !f.commit()) {
        if (outError) *outError = QCoreApplication::translate("MissionPlan", "export: write failed (%1)").arg(f.errorString());
        return false;
    }
    return true;
}

} // namespace

QJsonObject toPlanJson(const FlatMission& mission, const PlanSettings& settings, const QString& appVersion)
{
    QJsonArray items;
    int seq = 1;
    for (const auto& f : mission.items) {
        QJsonObject o;
        o.insert(QStringLiteral("type"), QStringLiteral("SimpleItem"));
        o.insert(QStringLiteral("command"), static_cast<int>(f.command));
        o.insert(QStringLiteral("frame"), static_cast<int>(f.frame));
        o.insert(QStringLiteral("autoContinue"), true);
        o.insert(QStringLiteral("doJumpId"), seq++);
        o.insert(QStringLiteral("params"), QJsonArray{jsonParam(f.param1), jsonParam(f.param2), jsonParam(f.param3), jsonParam(f.param4),
                                                      f.isNavigation() || f.command == MavCmd::NavLoiterUnlim ? QJsonValue(f.lat) : QJsonValue(0.0),
                                                      f.isNavigation() || f.command == MavCmd::NavLoiterUnlim ? QJsonValue(f.lon) : QJsonValue(0.0),
                                                      QJsonValue(f.alt)});
        o.insert(QStringLiteral("Altitude"), f.alt);
        o.insert(QStringLiteral("AltitudeMode"), kAltitudeModeRelative);
        o.insert(QStringLiteral("AMSLAltAboveTerrain"), QJsonValue::Null);
        items.append(o);
    }

    QJsonObject missionObj;
    missionObj.insert(QStringLiteral("version"), 2);
    missionObj.insert(QStringLiteral("firmwareType"), kFirmwareArduPilot);
    missionObj.insert(QStringLiteral("vehicleType"), kVehicleSurfaceBoat);
    missionObj.insert(QStringLiteral("globalPlanAltitudeMode"), kAltitudeModeRelative);
    missionObj.insert(QStringLiteral("cruiseSpeed"), settings.cruiseSpeed);
    missionObj.insert(QStringLiteral("hoverSpeed"), 0);
    if (mission.home && mission.home->isValid()) {
        missionObj.insert(QStringLiteral("plannedHomePosition"), QJsonArray{mission.home->lat, mission.home->lon, 0.0});
    }
    missionObj.insert(QStringLiteral("items"), items);

    QJsonArray fencePolygons;
    for (const auto& f : mission.fence) {
        if (f.ring.size() < kFenceMinVertices) {
            continue;
        }
        QJsonArray ring;
        for (const auto& p : f.ring) {
            ring.append(QJsonArray{p.lat, p.lon});
        }
        QJsonObject poly;
        poly.insert(QStringLiteral("inclusion"), f.inclusion);
        poly.insert(QStringLiteral("polygon"), ring);
        poly.insert(QStringLiteral("version"), 1);
        fencePolygons.append(poly);
    }
    QJsonObject fence;
    fence.insert(QStringLiteral("version"), 2);
    fence.insert(QStringLiteral("circles"), QJsonArray());
    fence.insert(QStringLiteral("polygons"), fencePolygons);

    QJsonArray rallyPts;
    for (const auto& r : mission.rally) {
        rallyPts.append(QJsonArray{r.lat, r.lon, 0.0});
    }
    QJsonObject rally;
    rally.insert(QStringLiteral("version"), 2);
    rally.insert(QStringLiteral("points"), rallyPts);

    QJsonObject root;
    root.insert(QStringLiteral("fileType"), QStringLiteral("Plan"));
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("groundStation"), QStringLiteral("KoggerApp ") + appVersion);
    root.insert(QStringLiteral("mission"), missionObj);
    root.insert(QStringLiteral("geoFence"), fence);
    root.insert(QStringLiteral("rallyPoints"), rally);
    return root;
}

QString toWpl(const FlatMission& mission)
{
    QString out;
    out += QStringLiteral("QGC WPL 110\n");
    const QChar tab('\t');
    auto line = [&](int seq, int current, const FlatItem& f) {
        out += QString::number(seq) + tab + QString::number(current) + tab
             + QString::number(static_cast<int>(f.frame)) + tab
             + QString::number(static_cast<int>(f.command)) + tab
             + num(f.param1, 6) + tab + num(f.param2, 6) + tab + num(f.param3, 6) + tab + num(f.param4, 6) + tab
             + num(f.lat, 7) + tab + num(f.lon, 7) + tab + num(f.alt, 6) + tab + QStringLiteral("1\n");
    };

    FlatItem home;
    home.command = MavCmd::NavWaypoint;
    home.frame = MavFrame::Global;
    if (mission.home && mission.home->isValid()) {
        home.lat = mission.home->lat;
        home.lon = mission.home->lon;
    }
    line(0, 1, home);

    int seq = 1;
    for (const auto& f : mission.items) {
        FlatItem row = f;
        row.frame = MavFrame::GlobalRelativeAlt;
        line(seq++, 0, row);
    }
    return out;
}

bool savePlanFile(const QString& path, const FlatMission& mission, const PlanSettings& settings, const QString& appVersion, QString* outError)
{
    return writeText(path, QJsonDocument(toPlanJson(mission, settings, appVersion)).toJson(QJsonDocument::Indented), outError);
}

bool saveWplFile(const QString& path, const FlatMission& mission, QString* outError)
{
    return writeText(path, toWpl(mission).toUtf8(), outError);
}

} // namespace Export
} // namespace mission
