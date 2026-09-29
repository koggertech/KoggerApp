#include "mission_file_io.h"

#include <algorithm>
#include <cmath>

#include <QFile>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>

namespace mission {

namespace {

const QString kFileType = QStringLiteral("KoggerMission");

const QSet<QString> kPlanKeys = {
    QStringLiteral("fileType"), QStringLiteral("formatVersion"), QStringLiteral("appVersion"),
    QStringLiteral("name"), QStringLiteral("created"), QStringLiteral("modified"),
    QStringLiteral("settings"), QStringLiteral("home"), QStringLiteral("items"), QStringLiteral("rally"),
    QStringLiteral("fence")
};

const QSet<QString> kWaypointKeys = {
    QStringLiteral("id"), QStringLiteral("type"), QStringLiteral("lat"), QStringLiteral("lon"),
    QStringLiteral("holdTime"), QStringLiteral("acceptRadius"), QStringLiteral("speed")
};

const QSet<QString> kSurveyKeys = {
    QStringLiteral("id"), QStringLiteral("type"), QStringLiteral("polygon"), QStringLiteral("lineSpacing"),
    QStringLiteral("angleDeg"), QStringLiteral("turnaround"), QStringLiteral("entryCorner"),
    QStringLiteral("crosshatch"), QStringLiteral("crosshatchSpacing"), QStringLiteral("speed")
};

const QSet<QString> kCorridorKeys = {
    QStringLiteral("id"), QStringLiteral("type"), QStringLiteral("axis"), QStringLiteral("width"),
    QStringLiteral("lineSpacing"), QStringLiteral("turnaround"), QStringLiteral("entryEnd"), QStringLiteral("speed")
};

const QSet<QString> kRallyKeys = {
    QStringLiteral("id"), QStringLiteral("type"), QStringLiteral("lat"), QStringLiteral("lon")
};

const QSet<QString> kFenceKeys = {
    QStringLiteral("id"), QStringLiteral("type"), QStringLiteral("polygon"), QStringLiteral("inclusion")
};

QJsonObject unknownKeys(const QJsonObject& obj, const QSet<QString>& known)
{
    QJsonObject extra;
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        if (!known.contains(it.key())) {
            extra.insert(it.key(), it.value());
        }
    }
    return extra;
}

QJsonArray pointsToJson(const QVector<GeoPoint>& pts)
{
    QJsonArray arr;
    for (const auto& p : pts) {
        arr.append(QJsonArray{p.lat, p.lon});
    }
    return arr;
}

bool pointsFromJson(const QJsonValue& v, QVector<GeoPoint>* out, int minCount, const QString& what, QString* err)
{
    if (!v.isArray()) {
        if (err) *err = what + QStringLiteral(": expected array of [lat, lon]");
        return false;
    }
    const QJsonArray arr = v.toArray();
    if (arr.size() > kMaxShapeVertices) {
        if (err) *err = what + QStringLiteral(": more than %1 vertices").arg(kMaxShapeVertices);
        return false;
    }
    out->clear();
    out->reserve(arr.size());
    for (const auto& e : arr) {
        const QJsonArray pair = e.toArray();
        if (pair.size() != 2 || !pair[0].isDouble() || !pair[1].isDouble()) {
            if (err) *err = what + QStringLiteral(": vertex must be [lat, lon]");
            return false;
        }
        const GeoPoint p(pair[0].toDouble(), pair[1].toDouble());
        if (!p.isValid()) {
            if (err) *err = what + QStringLiteral(": vertex out of range");
            return false;
        }
        out->append(p);
    }
    if (out->size() < minCount) {
        if (err) *err = what + QStringLiteral(": needs at least %1 vertices").arg(minCount);
        return false;
    }
    return true;
}

QJsonValue optionalDouble(const std::optional<double>& v)
{
    return v ? QJsonValue(*v) : QJsonValue(QJsonValue::Null);
}

QJsonValue optionalInt(const std::optional<int>& v)
{
    return v ? QJsonValue(*v) : QJsonValue(QJsonValue::Null);
}

std::optional<double> readOptionalDouble(const QJsonObject& o, const QString& key)
{
    const QJsonValue v = o.value(key);
    if (v.isDouble()) {
        return v.toDouble();
    }
    return std::nullopt;
}

std::optional<int> readOptionalInt(const QJsonObject& o, const QString& key)
{
    const QJsonValue v = o.value(key);
    if (v.isDouble()) {
        return v.toInt();
    }
    return std::nullopt;
}

double readDouble(const QJsonObject& o, const QString& key, double def)
{
    const QJsonValue v = o.value(key);
    return v.isDouble() && std::isfinite(v.toDouble()) ? v.toDouble() : def;
}

bool checkMagnitude(double v, double limit, const QString& what, const QString& key, QString* err)
{
    if (!std::isfinite(v) || std::fabs(v) > limit) {
        if (err) *err = what + QStringLiteral(": %1 must be finite and within ±%2").arg(key).arg(limit, 0, 'f', 0);
        return false;
    }
    return true;
}

bool readGeo(const QJsonObject& o, GeoPoint* out, const QString& what, QString* err)
{
    const QJsonValue lat = o.value(QStringLiteral("lat"));
    const QJsonValue lon = o.value(QStringLiteral("lon"));
    if (!lat.isDouble() || !lon.isDouble()) {
        if (err) *err = what + QStringLiteral(": lat/lon must be numbers");
        return false;
    }
    *out = GeoPoint(lat.toDouble(), lon.toDouble());
    if (!out->isValid()) {
        if (err) *err = what + QStringLiteral(": lat/lon out of range");
        return false;
    }
    return true;
}

QString endActionName(EndAction a)
{
    switch (a) {
    case EndAction::Hold:          return QStringLiteral("hold");
    case EndAction::ReturnToStart: return QStringLiteral("start");
    case EndAction::Rtl:           return QStringLiteral("rtl");
    }
    return QStringLiteral("start");
}

EndAction endActionFromName(const QString& s)
{
    if (s == QStringLiteral("hold")) {
        return EndAction::Hold;
    }
    if (s == QStringLiteral("rtl")) {
        return EndAction::Rtl;
    }
    return EndAction::ReturnToStart;
}

bool migrate(QJsonObject& root, int fromVersion, QString* err)
{
    if (fromVersion > kFormatVersion) {
        if (err) *err = QStringLiteral("formatVersion %1 is newer than supported %2").arg(fromVersion).arg(kFormatVersion);
        return false;
    }
    if (fromVersion < 2) {
        QJsonObject settings = root.value(QStringLiteral("settings")).toObject();
        if (settings.value(QStringLiteral("endAction")).toString() == QStringLiteral("rtl")) {
            settings.insert(QStringLiteral("endAction"), QStringLiteral("start"));
            root.insert(QStringLiteral("settings"), settings);
        }
    }
    root.insert(QStringLiteral("formatVersion"), kFormatVersion);
    return true;
}

std::optional<double> readSpeed(const QJsonObject& o)
{
    const QJsonValue v = o.value(QStringLiteral("speed"));
    if (v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() > 0.0) {
        return v.toDouble();
    }
    return std::nullopt;
}

} // namespace

namespace FileIo {

QJsonObject itemToJson(const MissionItem& item)
{
    return std::visit([](const auto& it) -> QJsonObject {
        using T = std::decay_t<decltype(it)>;
        QJsonObject o = it.extra;
        o.insert(QStringLiteral("id"), it.id);
        if constexpr (std::is_same_v<T, WaypointItem>) {
            o.insert(QStringLiteral("type"), QStringLiteral("waypoint"));
            o.insert(QStringLiteral("lat"), it.pos.lat);
            o.insert(QStringLiteral("lon"), it.pos.lon);
            o.insert(QStringLiteral("holdTime"), it.holdTime);
            o.insert(QStringLiteral("acceptRadius"), it.acceptRadius);
            o.insert(QStringLiteral("speed"), optionalDouble(it.speed));
        } else if constexpr (std::is_same_v<T, SurveyItem>) {
            o.insert(QStringLiteral("type"), QStringLiteral("survey"));
            o.insert(QStringLiteral("polygon"), pointsToJson(it.polygon));
            o.insert(QStringLiteral("lineSpacing"), it.lineSpacing);
            o.insert(QStringLiteral("angleDeg"), it.angleDeg);
            o.insert(QStringLiteral("turnaround"), it.turnaround);
            o.insert(QStringLiteral("entryCorner"), optionalInt(it.entryCorner));
            o.insert(QStringLiteral("crosshatch"), it.crosshatch);
            o.insert(QStringLiteral("crosshatchSpacing"), optionalDouble(it.crosshatchSpacing));
            o.insert(QStringLiteral("speed"), optionalDouble(it.speed));
        } else {
            o.insert(QStringLiteral("type"), QStringLiteral("corridor"));
            o.insert(QStringLiteral("axis"), pointsToJson(it.axis));
            o.insert(QStringLiteral("width"), it.width);
            o.insert(QStringLiteral("lineSpacing"), it.lineSpacing);
            o.insert(QStringLiteral("turnaround"), it.turnaround);
            o.insert(QStringLiteral("entryEnd"), optionalInt(it.entryEnd));
            o.insert(QStringLiteral("speed"), optionalDouble(it.speed));
        }
        return o;
    }, item);
}

QJsonObject fenceToJson(const FencePolygon& fence)
{
    QJsonObject o = fence.extra;
    o.insert(QStringLiteral("id"), fence.id);
    o.insert(QStringLiteral("type"), QStringLiteral("fence"));
    o.insert(QStringLiteral("polygon"), pointsToJson(fence.ring));
    o.insert(QStringLiteral("inclusion"), fence.inclusion);
    return o;
}

bool fenceFromJson(const QJsonObject& obj, FencePolygon* out, QString* outError)
{
    FencePolygon f;
    f.id = obj.value(QStringLiteral("id")).toString();
    if (f.id.isEmpty()) {
        if (outError) *outError = QStringLiteral("fence: missing id");
        return false;
    }
    if (!pointsFromJson(obj.value(QStringLiteral("polygon")), &f.ring, kFenceMinVertices, QStringLiteral("fence ") + f.id, outError)) {
        return false;
    }
    const QJsonValue inclusion = obj.value(QStringLiteral("inclusion"));
    if (!inclusion.isUndefined() && !inclusion.isBool()) {
        if (outError) *outError = QStringLiteral("fence %1: inclusion must be true or false").arg(f.id);
        return false;
    }
    f.inclusion = inclusion.toBool(true);
    f.extra = unknownKeys(obj, kFenceKeys);
    *out = f;
    return true;
}

QJsonObject rallyToJson(const RallyItem& item)
{
    QJsonObject o = item.extra;
    o.insert(QStringLiteral("id"), item.id);
    o.insert(QStringLiteral("lat"), item.pos.lat);
    o.insert(QStringLiteral("lon"), item.pos.lon);
    return o;
}

bool itemFromJson(const QJsonObject& obj, MissionItem* outItem, QString* outError)
{
    const QString type = obj.value(QStringLiteral("type")).toString();
    const QString id = obj.value(QStringLiteral("id")).toString();
    if (id.isEmpty()) {
        if (outError) *outError = QStringLiteral("item: missing id");
        return false;
    }
    const QString what = type + QStringLiteral(" ") + id;

    if (type == QStringLiteral("waypoint")) {
        WaypointItem w;
        w.id = id;
        if (!readGeo(obj, &w.pos, what, outError)) {
            return false;
        }
        w.holdTime = std::max(0.0, readDouble(obj, QStringLiteral("holdTime"), 0.0));
        w.acceptRadius = std::max(0.0, readDouble(obj, QStringLiteral("acceptRadius"), 0.0));
        w.speed = readOptionalDouble(obj, QStringLiteral("speed"));
        if (w.speed && !(*w.speed > 0.0 && std::isfinite(*w.speed))) {
            w.speed.reset();
        }
        if (!checkMagnitude(w.holdTime, 86400.0, what, QStringLiteral("holdTime"), outError)
            || !checkMagnitude(w.acceptRadius, kMaxShapeExtentMeters, what, QStringLiteral("acceptRadius"), outError)) {
            return false;
        }
        w.extra = unknownKeys(obj, kWaypointKeys);
        *outItem = w;
        return true;
    }
    if (type == QStringLiteral("survey")) {
        SurveyItem s;
        s.id = id;
        if (!pointsFromJson(obj.value(QStringLiteral("polygon")), &s.polygon, 3, what, outError)) {
            return false;
        }
        s.lineSpacing = readDouble(obj, QStringLiteral("lineSpacing"), 10.0);
        if (!(s.lineSpacing >= kMinLineSpacing) || !checkMagnitude(s.lineSpacing, kMaxShapeExtentMeters, what, QStringLiteral("lineSpacing"), outError)) {
            if (outError && outError->isEmpty()) *outError = what + QStringLiteral(": lineSpacing must be >= %1").arg(kMinLineSpacing);
            return false;
        }
        s.angleDeg = std::fmod(readDouble(obj, QStringLiteral("angleDeg"), 0.0), 360.0);
        if (s.angleDeg < 0.0) {
            s.angleDeg += 360.0;
        }
        s.turnaround = readDouble(obj, QStringLiteral("turnaround"), 0.0);
        if (!checkMagnitude(s.turnaround, kMaxShapeExtentMeters, what, QStringLiteral("turnaround"), outError)) {
            return false;
        }
        s.entryCorner = readOptionalInt(obj, QStringLiteral("entryCorner"));
        if (s.entryCorner && (*s.entryCorner < 0 || *s.entryCorner > 3)) {
            s.entryCorner.reset();
        }
        s.crosshatch = obj.value(QStringLiteral("crosshatch")).toBool(false);
        s.crosshatchSpacing = readOptionalDouble(obj, QStringLiteral("crosshatchSpacing"));
        if (s.crosshatchSpacing && !(*s.crosshatchSpacing >= kMinLineSpacing && *s.crosshatchSpacing <= kMaxShapeExtentMeters)) {
            s.crosshatchSpacing.reset();
        }
        s.speed = readSpeed(obj);
        s.extra = unknownKeys(obj, kSurveyKeys);
        *outItem = s;
        return true;
    }
    if (type == QStringLiteral("corridor")) {
        CorridorItem c;
        c.id = id;
        if (!pointsFromJson(obj.value(QStringLiteral("axis")), &c.axis, 2, what, outError)) {
            return false;
        }
        c.width = readDouble(obj, QStringLiteral("width"), 20.0);
        c.lineSpacing = readDouble(obj, QStringLiteral("lineSpacing"), 10.0);
        if (!(c.width > 0.0) || !(c.lineSpacing >= kMinLineSpacing)) {
            if (outError) *outError = what + QStringLiteral(": width must be > 0 and lineSpacing >= %1").arg(kMinLineSpacing);
            return false;
        }
        if (!checkMagnitude(c.width, kMaxShapeExtentMeters, what, QStringLiteral("width"), outError)
            || !checkMagnitude(c.lineSpacing, kMaxShapeExtentMeters, what, QStringLiteral("lineSpacing"), outError)) {
            return false;
        }
        c.turnaround = readDouble(obj, QStringLiteral("turnaround"), 0.0);
        if (!checkMagnitude(c.turnaround, kMaxShapeExtentMeters, what, QStringLiteral("turnaround"), outError)) {
            return false;
        }
        c.entryEnd = readOptionalInt(obj, QStringLiteral("entryEnd"));
        if (c.entryEnd && (*c.entryEnd < 0 || *c.entryEnd > 3)) {
            c.entryEnd.reset();
        }
        c.speed = readSpeed(obj);
        c.extra = unknownKeys(obj, kCorridorKeys);
        *outItem = c;
        return true;
    }

    if (outError) *outError = QStringLiteral("item %1: unknown type '%2'").arg(id, type);
    return false;
}

bool knownItemKey(const QString& type, const QString& key)
{
    if (type == QStringLiteral("waypoint")) {
        return kWaypointKeys.contains(key);
    }
    if (type == QStringLiteral("survey")) {
        return kSurveyKeys.contains(key);
    }
    if (type == QStringLiteral("corridor")) {
        return kCorridorKeys.contains(key);
    }
    if (type == QStringLiteral("rally")) {
        return kRallyKeys.contains(key);
    }
    if (type == QStringLiteral("fence")) {
        return kFenceKeys.contains(key);
    }
    return false;
}

bool isOpaqueUri(const QString& pathOrUri)
{
    const int scheme = pathOrUri.indexOf(QStringLiteral("://"));
    return scheme > 1 && !pathOrUri.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive);
}

QJsonObject toJson(const MissionPlan& plan, const QString& appVersion)
{
    QJsonObject root = plan.extra;
    root.insert(QStringLiteral("fileType"), kFileType);
    root.insert(QStringLiteral("formatVersion"), kFormatVersion);
    root.insert(QStringLiteral("appVersion"), appVersion);
    root.insert(QStringLiteral("name"), plan.name);
    root.insert(QStringLiteral("created"), plan.created);
    root.insert(QStringLiteral("modified"), plan.modified);

    QJsonObject settings;
    settings.insert(QStringLiteral("cruiseSpeed"), plan.settings.cruiseSpeed);
    settings.insert(QStringLiteral("endAction"), endActionName(plan.settings.endAction));
    root.insert(QStringLiteral("settings"), settings);

    if (plan.home && plan.home->isValid()) {
        QJsonObject home;
        home.insert(QStringLiteral("lat"), plan.home->lat);
        home.insert(QStringLiteral("lon"), plan.home->lon);
        root.insert(QStringLiteral("home"), home);
    } else {
        root.insert(QStringLiteral("home"), QJsonValue::Null);
    }

    QJsonArray items;
    for (const auto& it : plan.items) {
        items.append(itemToJson(it));
    }
    root.insert(QStringLiteral("items"), items);

    QJsonArray rally;
    for (const auto& r : plan.rally) {
        rally.append(rallyToJson(r));
    }
    root.insert(QStringLiteral("rally"), rally);

    QJsonArray fence;
    for (const auto& f : plan.fence) {
        fence.append(fenceToJson(f));
    }
    root.insert(QStringLiteral("fence"), fence);
    return root;
}

bool fromJson(const QJsonObject& input, MissionPlan* outPlan, QString* outError)
{
    if (!outPlan) {
        if (outError) *outError = QStringLiteral("parse: output plan is null");
        return false;
    }
    QJsonObject root = input;
    if (root.value(QStringLiteral("fileType")).toString() != kFileType) {
        if (outError) *outError = QStringLiteral("parse: fileType must be '%1'").arg(kFileType);
        return false;
    }
    const int version = root.value(QStringLiteral("formatVersion")).toInt(0);
    if (version < 1) {
        if (outError) *outError = QStringLiteral("parse: missing formatVersion");
        return false;
    }
    if (!migrate(root, version, outError)) {
        return false;
    }

    MissionPlan plan;
    plan.name = root.value(QStringLiteral("name")).toString();
    plan.created = root.value(QStringLiteral("created")).toString();
    plan.modified = root.value(QStringLiteral("modified")).toString();

    const QJsonObject settings = root.value(QStringLiteral("settings")).toObject();
    plan.settings.cruiseSpeed = readDouble(settings, QStringLiteral("cruiseSpeed"), 1.5);
    if (!(plan.settings.cruiseSpeed > 0.0)) {
        plan.settings.cruiseSpeed = 1.5;
    }
    plan.settings.endAction = endActionFromName(settings.value(QStringLiteral("endAction")).toString());

    const QJsonValue homeVal = root.value(QStringLiteral("home"));
    if (homeVal.isObject()) {
        GeoPoint home;
        if (!readGeo(homeVal.toObject(), &home, QStringLiteral("home"), outError)) {
            return false;
        }
        plan.home = home;
    }

    const QJsonValue itemsVal = root.value(QStringLiteral("items"));
    if (!itemsVal.isArray()) {
        if (outError) *outError = QStringLiteral("parse: items must be an array");
        return false;
    }
    const QJsonArray itemsArr = itemsVal.toArray();
    if (itemsArr.size() > kMaxPlanItems) {
        if (outError) *outError = QStringLiteral("parse: more than %1 items").arg(kMaxPlanItems);
        return false;
    }
    QSet<QString> ids{kHomeItemId};
    for (const auto& v : itemsArr) {
        if (!v.isObject()) {
            if (outError) *outError = QStringLiteral("parse: item must be an object");
            return false;
        }
        MissionItem item;
        if (!itemFromJson(v.toObject(), &item, outError)) {
            return false;
        }
        if (ids.contains(itemId(item))) {
            if (outError) *outError = QStringLiteral("parse: duplicate id '%1'").arg(itemId(item));
            return false;
        }
        ids.insert(itemId(item));
        plan.items.append(item);
    }

    const QJsonValue rallyVal = root.value(QStringLiteral("rally"));
    if (rallyVal.isArray()) {
        const QJsonArray rallyArr = rallyVal.toArray();
        if (rallyArr.size() > kMaxRallyPoints) {
            if (outError) *outError = QStringLiteral("parse: more than %1 rally points").arg(kMaxRallyPoints);
            return false;
        }
        for (const auto& v : rallyArr) {
            const QJsonObject o = v.toObject();
            RallyItem r;
            r.id = o.value(QStringLiteral("id")).toString();
            if (r.id.isEmpty() || ids.contains(r.id)) {
                if (outError) *outError = QStringLiteral("parse: rally point with missing or duplicate id");
                return false;
            }
            if (!readGeo(o, &r.pos, QStringLiteral("rally ") + r.id, outError)) {
                return false;
            }
            r.extra = unknownKeys(o, kRallyKeys);
            ids.insert(r.id);
            plan.rally.append(r);
        }
    }

    const QJsonValue fenceVal = root.value(QStringLiteral("fence"));
    if (fenceVal.isArray()) {
        const QJsonArray fenceArr = fenceVal.toArray();
        if (fenceArr.size() > kMaxFencePolygons) {
            if (outError) *outError = QStringLiteral("parse: more than %1 fence polygons").arg(kMaxFencePolygons);
            return false;
        }
        for (const auto& v : fenceArr) {
            FencePolygon f;
            if (!fenceFromJson(v.toObject(), &f, outError)) {
                return false;
            }
            if (ids.contains(f.id)) {
                if (outError) *outError = QStringLiteral("parse: duplicate id '%1'").arg(f.id);
                return false;
            }
            ids.insert(f.id);
            plan.fence.append(f);
        }
    }

    plan.extra = unknownKeys(root, kPlanKeys);
    *outPlan = plan;
    return true;
}

LoadResult loadFromFile(const QString& path)
{
    LoadResult r;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        r.error = QCoreApplication::translate("MissionPlan", "load: cannot open file");
        return r;
    }
    if (f.size() > kMaxMissionFileBytes) {
        r.error = QCoreApplication::translate("MissionPlan", "load: file larger than %1 MB").arg(kMaxMissionFileBytes / (1024 * 1024));
        return r;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (doc.isNull() || pe.error != QJsonParseError::NoError) {
        r.error = QCoreApplication::translate("MissionPlan", "load: invalid JSON: ") + pe.errorString();
        return r;
    }
    if (!doc.isObject()) {
        r.error = QCoreApplication::translate("MissionPlan", "load: root must be object");
        return r;
    }
    QString err;
    if (!fromJson(doc.object(), &r.plan, &err)) {
        r.error = err;
        return r;
    }
    r.ok = true;
    return r;
}

bool saveToFile(const QString& path, const MissionPlan& plan, const QString& appVersion, QString* outError)
{
    const QByteArray bytes = QJsonDocument(toJson(plan, appVersion)).toJson(QJsonDocument::Indented);
    if (isOpaqueUri(path)) {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (outError) *outError = QCoreApplication::translate("MissionPlan", "save: cannot open file for writing (%1)").arg(f.errorString());
            return false;
        }
        if (f.write(bytes) != bytes.size()) {
            if (outError) *outError = QCoreApplication::translate("MissionPlan", "save: write failed (%1)").arg(f.errorString());
            return false;
        }
        return true;
    }
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) *outError = QCoreApplication::translate("MissionPlan", "save: cannot open file for writing (%1)").arg(f.errorString());
        return false;
    }
    if (f.write(bytes) != bytes.size() || !f.commit()) {
        if (outError) *outError = QCoreApplication::translate("MissionPlan", "save: write failed (%1)").arg(f.errorString());
        return false;
    }
    return true;
}

} // namespace FileIo
} // namespace mission
