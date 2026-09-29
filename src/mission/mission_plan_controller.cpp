#include "mission_plan_controller.h"

#include <algorithm>
#include <cmath>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>

#include "mission_export.h"
#include "mission_file_io.h"

namespace mission {

namespace {

const char* kSettingsDirectory = "main/missionDirectory";
const char* kSettingsLastFile = "main/missionLastFile";
const char kIdAlphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
constexpr int kIdLength = 6;

QString readAppVersion()
{
    QFile file(QStringLiteral(":/version.txt"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll()).trimmed();
    }
    return QString();
}

QString localPath(const QString& pathOrUrl)
{
    if (pathOrUrl.startsWith(QStringLiteral("file:"))) {
        return QUrl(pathOrUrl).toLocalFile();
    }
    return pathOrUrl;
}

bool numberFrom(const QVariant& v, double* out)
{
    switch (v.typeId()) {
    case QMetaType::Double:
    case QMetaType::Float:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        break;
    default:
        return false;
    }
    const double d = v.toDouble();
    if (!std::isfinite(d)) {
        return false;
    }
    *out = d;
    return true;
}

QVector<GeoPoint>* vertexList(MissionItem& item)
{
    if (auto* s = std::get_if<SurveyItem>(&item)) {
        return &s->polygon;
    }
    if (auto* c = std::get_if<CorridorItem>(&item)) {
        return &c->axis;
    }
    return nullptr;
}

} // namespace

MissionPlanController::MissionPlanController(QObject* parent)
    : QObject(parent)
    , appVersion_(readAppVersion())
    , rng_(QRandomGenerator::securelySeeded())
{
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    directoryOverride_ = settings.value(QLatin1String(kSettingsDirectory)).toString();
    lastFile_ = settings.value(QLatin1String(kSettingsLastFile)).toString();
    newPlan();
}

void MissionPlanController::setFallbackDirectory(const QString& dir)
{
    if (fallbackDirectory_ == dir) {
        return;
    }
    fallbackDirectory_ = dir;
    emit directoryChanged();
}

bool MissionPlanController::dirty() const
{
    return lastDirty_;
}

QString MissionPlanController::exportBlocker() const
{
    if (expanded_.mission.items.isEmpty()) {
        return tr("The mission has no waypoints");
    }
    if (!expanded_.mission.home || !expanded_.mission.home->isValid()) {
        return tr("Start point is not set");
    }
    for (const auto& i : expanded_.issues) {
        if (i.level == IssueLevel::Error) {
            return i.text;
        }
    }
    return QString();
}

void MissionPlanController::retranslate()
{
    expanded_ = expandPlan(plan_);
    emit planChanged();
}

QString MissionPlanController::suggestedFilePath() const
{
    const QString dir = directory();
    if (dir.isEmpty() || FileIo::isOpaqueUri(dir)) {
        return defaultFileName();
    }
    QDir().mkpath(dir);
    const QString name = defaultFileName();
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    const QString stem = dot > 0 ? name.left(dot) : name;
    const QString ext = dot > 0 ? name.mid(dot) : QString();
    QString candidate = QDir(dir).filePath(name);
    for (int n = 2; QFileInfo::exists(candidate); ++n) {
        candidate = QDir(dir).filePath(QStringLiteral("%1 (%2)%3").arg(stem).arg(n).arg(ext));
    }
    return candidate;
}

void MissionPlanController::rememberDirectoryOf(const QString& fileOrUrl)
{
    const QString clean = localPath(fileOrUrl);
    if (clean.isEmpty() || FileIo::isOpaqueUri(clean)) {
        return;
    }
    setDirectoryOverride(QFileInfo(clean).absolutePath());
}

QVariantList MissionPlanController::issues() const
{
    QVariantList out;
    for (const auto& i : expanded_.issues) {
        QVariantMap m;
        m.insert(QStringLiteral("itemId"), i.itemId);
        m.insert(QStringLiteral("text"), i.text);
        m.insert(QStringLiteral("level"), static_cast<int>(i.level));
        out.append(m);
    }
    return out;
}

QVariantMap MissionPlanController::estimates() const
{
    const MissionEstimates& e = expanded_.estimates;
    QVariantMap m;
    m.insert(QStringLiteral("lengthMeters"), e.lengthMeters);
    m.insert(QStringLiteral("timeSeconds"), e.timeSeconds);
    m.insert(QStringLiteral("waypointCount"), e.waypointCount);
    m.insert(QStringLiteral("flatItemCount"), e.flatItemCount);
    return m;
}

QString MissionPlanController::directory() const
{
    return directoryOverride_.isEmpty() ? fallbackDirectory_ : directoryOverride_;
}

void MissionPlanController::setName(const QString& name)
{
    if (plan_.name == name) {
        return;
    }
    pushUndo();
    plan_.name = name;
    afterChange();
}

void MissionPlanController::setCruiseSpeed(double speed)
{
    if (!(speed > 0.0) || qFuzzyCompare(plan_.settings.cruiseSpeed, speed)) {
        return;
    }
    pushUndo();
    plan_.settings.cruiseSpeed = speed;
    afterChange();
}

void MissionPlanController::setEndAction(int action)
{
    const EndAction a = action == static_cast<int>(EndAction::Hold) ? EndAction::Hold : EndAction::Rtl;
    if (plan_.settings.endAction == a) {
        return;
    }
    pushUndo();
    plan_.settings.endAction = a;
    afterChange();
}

void MissionPlanController::setDirectoryOverride(const QString& dir)
{
    const QString clean = FileIo::isOpaqueUri(dir) ? QString() : localPath(dir);
    if (directoryOverride_ == clean) {
        return;
    }
    directoryOverride_ = clean;
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.setValue(QLatin1String(kSettingsDirectory), directoryOverride_);
    emit directoryChanged();
}

void MissionPlanController::newPlan()
{
    plan_ = MissionPlan();
    plan_.name = QStringLiteral("Mission");
    plan_.created = nowIso();
    plan_.modified = plan_.created;
    expanded_ = expandPlan(plan_);
    undo_.clear();
    redo_.clear();
    markClean();
    emit planChanged();
    emit undoChanged();
    setFilePath(QString());
}

bool MissionPlanController::openFile(const QString& path)
{
    const QString clean = localPath(path);
    const LoadResult r = FileIo::loadFromFile(clean);
    if (!r.ok) {
        setLastError(r.error);
        return false;
    }
    plan_ = r.plan;
    expanded_ = expandPlan(plan_);
    undo_.clear();
    redo_.clear();
    markClean();
    setLastError(QString());
    emit planChanged();
    emit undoChanged();
    setFilePath(clean);
    rememberLastFile(clean);
    return true;
}

bool MissionPlanController::saveFile()
{
    if (filePath_.isEmpty()) {
        setLastError(tr("save: no file path"));
        return false;
    }
    return saveFileAs(filePath_);
}

bool MissionPlanController::saveFileAs(const QString& path)
{
    const QString clean = localPath(path);
    if (clean.isEmpty()) {
        setLastError(tr("save: empty path"));
        return false;
    }
    if (!FileIo::isOpaqueUri(clean)) {
        const QString dir = QFileInfo(clean).absolutePath();
        if (!QDir().mkpath(dir)) {
            setLastError(tr("save: cannot create folder %1").arg(dir));
            return false;
        }
    }
    MissionPlan stamped = plan_;
    stamped.modified = nowIso();
    if (stamped.created.isEmpty()) {
        stamped.created = stamped.modified;
    }
    QString err;
    if (!FileIo::saveToFile(clean, stamped, appVersion_, &err)) {
        setLastError(err);
        return false;
    }
    plan_ = stamped;
    setFilePath(clean);
    rememberLastFile(clean);
    markClean();
    setLastError(QString());
    emit planChanged();
    return true;
}

bool MissionPlanController::exportPlanFile(const QString& path)
{
    const QString blocker = exportBlocker();
    if (!blocker.isEmpty()) {
        setLastError(tr("export: %1").arg(blocker));
        return false;
    }
    QString err;
    if (!Export::savePlanFile(localPath(path), expanded_.mission, plan_.settings, appVersion_, &err)) {
        setLastError(err);
        return false;
    }
    setLastError(QString());
    return true;
}

bool MissionPlanController::exportWplFile(const QString& path)
{
    const QString blocker = exportBlocker();
    if (!blocker.isEmpty()) {
        setLastError(tr("export: %1").arg(blocker));
        return false;
    }
    QString err;
    if (!Export::saveWplFile(localPath(path), expanded_.mission, &err)) {
        setLastError(err);
        return false;
    }
    setLastError(QString());
    return true;
}

QString MissionPlanController::defaultFileName() const
{
    QString base = plan_.name.trimmed();
    static const QRegularExpression illegal(QStringLiteral("[\\\\/:*?\"<>|]+"));
    base.replace(illegal, QStringLiteral("_"));
    if (base.isEmpty()) {
        base = QStringLiteral("mission");
    }
    return base + QStringLiteral(".kmission");
}

QString MissionPlanController::directoryUrl() const
{
    const QString dir = directory();
    if (dir.isEmpty()) {
        return QString();
    }
    if (FileIo::isOpaqueUri(dir)) {
        return dir;
    }
    QDir().mkpath(dir);
    return QUrl::fromLocalFile(dir).toString();
}

QString MissionPlanController::planJson() const
{
    return QString::fromUtf8(QJsonDocument(FileIo::toJson(plan_, appVersion_)).toJson(QJsonDocument::Indented));
}

bool MissionPlanController::loadPlanJson(const QString& json)
{
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &pe);
    if (pe.error != QJsonParseError::NoError) {
        setLastError(tr("json: %1 at offset %2").arg(pe.errorString()).arg(pe.offset));
        return false;
    }
    if (!doc.isObject()) {
        setLastError(tr("json: root must be an object"));
        return false;
    }
    MissionPlan plan;
    QString err;
    if (!FileIo::fromJson(doc.object(), &plan, &err)) {
        setLastError(err);
        return false;
    }
    pushUndo();
    plan_ = plan;
    afterChange();
    setLastError(QString());
    return true;
}

QString MissionPlanController::flatJson() const
{
    QJsonArray items;
    for (const auto& f : expanded_.mission.items) {
        QJsonObject o;
        o.insert(QStringLiteral("command"), static_cast<int>(f.command));
        o.insert(QStringLiteral("frame"), static_cast<int>(f.frame));
        auto param = [](double v) { return std::isfinite(v) ? QJsonValue(v) : QJsonValue(QJsonValue::Null); };
        o.insert(QStringLiteral("params"), QJsonArray{param(f.param1), param(f.param2), param(f.param3), param(f.param4)});
        o.insert(QStringLiteral("lat"), f.lat);
        o.insert(QStringLiteral("lon"), f.lon);
        o.insert(QStringLiteral("alt"), f.alt);
        o.insert(QStringLiteral("sourceId"), f.sourceId);
        items.append(o);
    }
    QJsonObject root;
    if (expanded_.mission.home) {
        root.insert(QStringLiteral("home"), QJsonArray{expanded_.mission.home->lat, expanded_.mission.home->lon});
    } else {
        root.insert(QStringLiteral("home"), QJsonValue::Null);
    }
    root.insert(QStringLiteral("items"), items);
    QJsonArray rally;
    for (const auto& r : expanded_.mission.rally) {
        rally.append(QJsonArray{r.lat, r.lon});
    }
    root.insert(QStringLiteral("rally"), rally);
    root.insert(QStringLiteral("warnings"), QJsonArray::fromStringList(expanded_.warnings()));
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QString MissionPlanController::itemJson(const QString& id) const
{
    for (const auto& it : plan_.items) {
        if (itemId(it) == id) {
            return QString::fromUtf8(QJsonDocument(FileIo::itemToJson(it)).toJson(QJsonDocument::Compact));
        }
    }
    for (const auto& r : plan_.rally) {
        if (r.id == id) {
            QJsonObject o = FileIo::rallyToJson(r);
            o.insert(QStringLiteral("type"), QStringLiteral("rally"));
            return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
        }
    }
    return QString();
}

QVariantMap MissionPlanController::itemInfo(const QString& id) const
{
    QVariantMap m;
    for (const auto& it : plan_.items) {
        if (itemId(it) != id) {
            continue;
        }
        m.insert(QStringLiteral("type"), QString::fromLatin1(itemTypeName(it)));
        int count = 0;
        for (const auto& f : expanded_.mission.items) {
            if (f.sourceId == id && f.isNavigation()) {
                ++count;
            }
        }
        m.insert(QStringLiteral("waypointCount"), count);
        const auto g = expanded_.generated.constFind(id);
        if (g != expanded_.generated.constEnd()) {
            m.insert(QStringLiteral("lineCount"), g->lineCount);
            m.insert(QStringLiteral("resolvedEntry"), g->resolvedEntry);
            m.insert(QStringLiteral("lengthMeters"), g->lengthMeters);
            m.insert(QStringLiteral("areaSquareMeters"), g->areaSquareMeters);
            m.insert(QStringLiteral("effectiveSpacing"), g->effectiveSpacing);
            m.insert(QStringLiteral("spacingClamped"), g->spacingClamped);
            m.insert(QStringLiteral("error"), g->error);
        }
        return m;
    }
    for (const auto& r : plan_.rally) {
        if (r.id == id) {
            m.insert(QStringLiteral("type"), QStringLiteral("rally"));
            return m;
        }
    }
    return m;
}

QVariantList MissionPlanController::generatedPoints(const QString& id) const
{
    const auto g = expanded_.generated.constFind(id);
    return g == expanded_.generated.constEnd() ? QVariantList() : pointsToVariant(g->points);
}

QVariantList MissionPlanController::generatedLines(const QString& id) const
{
    QVariantList out;
    const auto g = expanded_.generated.constFind(id);
    if (g == expanded_.generated.constEnd()) {
        return out;
    }
    for (const auto& line : g->lines) {
        out.append(QVariant(pointsToVariant(line)));
    }
    return out;
}

QStringList MissionPlanController::itemIds() const
{
    QStringList ids;
    for (const auto& it : plan_.items) {
        ids.append(itemId(it));
    }
    return ids;
}

QStringList MissionPlanController::rallyIds() const
{
    QStringList ids;
    for (const auto& r : plan_.rally) {
        ids.append(r.id);
    }
    return ids;
}

int MissionPlanController::indexOfItem(const QString& id) const
{
    for (int i = 0; i < plan_.items.size(); ++i) {
        if (itemId(plan_.items[i]) == id) {
            return i;
        }
    }
    return -1;
}

void MissionPlanController::setHome(double lat, double lon)
{
    const GeoPoint p(lat, lon);
    if (!p.isValid()) {
        setLastError(tr("home: invalid position"));
        return;
    }
    pushUndo();
    plan_.home = p;
    afterChange();
}

void MissionPlanController::clearHome()
{
    if (!plan_.home) {
        return;
    }
    pushUndo();
    plan_.home.reset();
    afterChange();
}

QString MissionPlanController::addWaypoint(double lat, double lon, int insertIndex)
{
    const GeoPoint p(lat, lon);
    if (!p.isValid()) {
        setLastError(tr("waypoint: invalid position"));
        return QString();
    }
    pushUndo();
    WaypointItem w;
    w.id = newId();
    w.pos = p;
    plan_.items.insert(clampInsertIndex(insertIndex), w);
    afterChange();
    return w.id;
}

QString MissionPlanController::addSurvey(const QVariantList& polygon, int insertIndex)
{
    SurveyItem s;
    if (!parsePoints(polygon, &s.polygon, 3)) {
        return QString();
    }
    pushUndo();
    s.id = newId();
    plan_.items.insert(clampInsertIndex(insertIndex), s);
    afterChange();
    return s.id;
}

QString MissionPlanController::addCorridor(const QVariantList& axis, int insertIndex)
{
    CorridorItem c;
    if (!parsePoints(axis, &c.axis, 2)) {
        return QString();
    }
    pushUndo();
    c.id = newId();
    plan_.items.insert(clampInsertIndex(insertIndex), c);
    afterChange();
    return c.id;
}

QString MissionPlanController::addRally(double lat, double lon)
{
    const GeoPoint p(lat, lon);
    if (!p.isValid()) {
        setLastError(tr("rally: invalid position"));
        return QString();
    }
    pushUndo();
    RallyItem r;
    r.id = newId();
    r.pos = p;
    plan_.rally.append(r);
    afterChange();
    return r.id;
}

bool MissionPlanController::removeItem(const QString& id)
{
    const int idx = indexOfItem(id);
    if (idx >= 0) {
        pushUndo();
        plan_.items.removeAt(idx);
        afterChange();
        return true;
    }
    for (int i = 0; i < plan_.rally.size(); ++i) {
        if (plan_.rally[i].id == id) {
            pushUndo();
            plan_.rally.removeAt(i);
            afterChange();
            return true;
        }
    }
    setLastError(tr("remove: unknown id"));
    return false;
}

bool MissionPlanController::moveItem(const QString& id, int newIndex)
{
    const int idx = indexOfItem(id);
    if (idx < 0) {
        setLastError(tr("move: unknown id"));
        return false;
    }
    const int target = std::clamp(newIndex, 0, static_cast<int>(plan_.items.size()) - 1);
    if (target == idx) {
        return true;
    }
    pushUndo();
    MissionItem item = plan_.items[idx];
    plan_.items.removeAt(idx);
    plan_.items.insert(target, item);
    afterChange();
    return true;
}

bool MissionPlanController::updateItem(const QString& id, const QVariantMap& patch)
{
    MissionItem* item = findItem(id);
    if (item) {
        QJsonObject obj = FileIo::itemToJson(*item);
        const QString type = QString::fromLatin1(itemTypeName(*item));
        const QJsonObject p = QJsonObject::fromVariantMap(patch);
        for (auto it = p.constBegin(); it != p.constEnd(); ++it) {
            if (it.key() == QStringLiteral("id") || it.key() == QStringLiteral("type")) {
                continue;
            }
            if (!FileIo::knownItemKey(type, it.key())) {
                setLastError(tr("update: unknown field %1").arg(it.key()));
                return false;
            }
            obj.insert(it.key(), it.value());
        }
        MissionItem updated;
        QString err;
        if (!FileIo::itemFromJson(obj, &updated, &err)) {
            setLastError(err);
            return false;
        }
        pushUndo();
        *item = updated;
        afterChange();
        return true;
    }
    RallyItem* rally = findRally(id);
    if (rally) {
        for (auto it = patch.constBegin(); it != patch.constEnd(); ++it) {
            if (!FileIo::knownItemKey(QStringLiteral("rally"), it.key())) {
                setLastError(tr("update: unknown field %1").arg(it.key()));
                return false;
            }
        }
        double lat = rally->pos.lat;
        double lon = rally->pos.lon;
        const bool latOk = !patch.contains(QStringLiteral("lat")) || numberFrom(patch.value(QStringLiteral("lat")), &lat);
        const bool lonOk = !patch.contains(QStringLiteral("lon")) || numberFrom(patch.value(QStringLiteral("lon")), &lon);
        const GeoPoint p(lat, lon);
        if (!latOk || !lonOk || !p.isValid()) {
            setLastError(tr("rally: invalid position"));
            return false;
        }
        if (rally->pos.lat == p.lat && rally->pos.lon == p.lon) {
            return true;
        }
        pushUndo();
        rally->pos = p;
        afterChange();
        return true;
    }
    setLastError(tr("update: unknown id"));
    return false;
}

bool MissionPlanController::moveVertex(const QString& id, int index, double lat, double lon)
{
    const GeoPoint p(lat, lon);
    if (!p.isValid()) {
        setLastError(tr("vertex: invalid position"));
        return false;
    }
    if (MissionItem* item = findItem(id)) {
        if (auto* w = std::get_if<WaypointItem>(item)) {
            pushUndo();
            w->pos = p;
            afterChange();
            return true;
        }
        QVector<GeoPoint>* pts = vertexList(*item);
        if (!pts || index < 0 || index >= pts->size()) {
            setLastError(tr("vertex: index out of range"));
            return false;
        }
        pushUndo();
        (*pts)[index] = p;
        afterChange();
        return true;
    }
    if (RallyItem* r = findRally(id)) {
        pushUndo();
        r->pos = p;
        afterChange();
        return true;
    }
    setLastError(tr("vertex: unknown id"));
    return false;
}

bool MissionPlanController::insertVertex(const QString& id, int index, double lat, double lon)
{
    const GeoPoint p(lat, lon);
    if (!p.isValid()) {
        setLastError(tr("vertex: invalid position"));
        return false;
    }
    MissionItem* item = findItem(id);
    QVector<GeoPoint>* pts = item ? vertexList(*item) : nullptr;
    if (!pts) {
        setLastError(tr("vertex: item has no vertex list"));
        return false;
    }
    pushUndo();
    pts->insert(std::clamp(index, 0, static_cast<int>(pts->size())), p);
    afterChange();
    return true;
}

bool MissionPlanController::removeVertex(const QString& id, int index)
{
    MissionItem* item = findItem(id);
    QVector<GeoPoint>* pts = item ? vertexList(*item) : nullptr;
    if (!pts) {
        setLastError(tr("vertex: item has no vertex list"));
        return false;
    }
    const int minCount = std::holds_alternative<SurveyItem>(*item) ? 3 : 2;
    if (index < 0 || index >= pts->size() || pts->size() <= minCount) {
        setLastError(tr("vertex: cannot remove"));
        return false;
    }
    pushUndo();
    pts->removeAt(index);
    afterChange();
    return true;
}

void MissionPlanController::undo()
{
    if (undo_.isEmpty()) {
        return;
    }
    redo_.append(snapshot());
    const QByteArray s = undo_.takeLast();
    restore(s);
    afterChange();
}

void MissionPlanController::redo()
{
    if (redo_.isEmpty()) {
        return;
    }
    undo_.append(snapshot());
    const QByteArray s = redo_.takeLast();
    restore(s);
    afterChange();
}

void MissionPlanController::beginTransaction()
{
    if (transactionDepth_++ > 0) {
        return;
    }
    transactionSnapshot_ = snapshot();
}

void MissionPlanController::endTransaction(bool keep)
{
    if (transactionDepth_ == 0) {
        return;
    }
    if (--transactionDepth_ > 0) {
        return;
    }
    if (transactionSnapshot_ == snapshot()) {
        return;
    }
    if (keep) {
        undo_.append(transactionSnapshot_);
        while (undo_.size() > kUndoDepth) {
            undo_.removeFirst();
        }
        redo_.clear();
        emit undoChanged();
    } else {
        restore(transactionSnapshot_);
        afterChange();
    }
}

void MissionPlanController::setIdSeed(uint seed)
{
    rng_.seed(seed);
}

void MissionPlanController::setFrozenTime(const QString& isoUtc)
{
    frozenTime_ = isoUtc;
}

QString MissionPlanController::nowIso() const
{
    if (!frozenTime_.isEmpty()) {
        return frozenTime_;
    }
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QString MissionPlanController::newId()
{
    const int alphabetSize = static_cast<int>(sizeof(kIdAlphabet)) - 1;
    for (;;) {
        QString id;
        id.reserve(kIdLength);
        for (int i = 0; i < kIdLength; ++i) {
            id.append(QLatin1Char(kIdAlphabet[rng_.bounded(alphabetSize)]));
        }
        bool taken = indexOfItem(id) >= 0;
        for (const auto& r : plan_.rally) {
            taken = taken || r.id == id;
        }
        if (!taken) {
            return id;
        }
    }
}

QByteArray MissionPlanController::snapshot() const
{
    return QJsonDocument(FileIo::toJson(plan_, appVersion_)).toJson(QJsonDocument::Compact);
}

void MissionPlanController::pushUndo()
{
    if (transactionDepth_ > 0) {
        return;
    }
    undo_.append(snapshot());
    while (undo_.size() > kUndoDepth) {
        undo_.removeFirst();
    }
    redo_.clear();
}

bool MissionPlanController::restore(const QByteArray& bytes)
{
    MissionPlan plan;
    QString err;
    if (!FileIo::fromJson(QJsonDocument::fromJson(bytes).object(), &plan, &err)) {
        setLastError(err);
        return false;
    }
    plan_ = plan;
    return true;
}

void MissionPlanController::afterChange()
{
    setLastError(QString());
    expanded_ = expandPlan(plan_);
    const bool d = snapshot() != cleanSnapshot_;
    const bool dirtyFlipped = d != lastDirty_;
    lastDirty_ = d;
    emit planChanged();
    emit undoChanged();
    if (dirtyFlipped) {
        emit dirtyChanged();
    }
}

void MissionPlanController::setLastError(const QString& error)
{
    if (error.isEmpty() && lastError_.isEmpty()) {
        return;
    }
    lastError_ = error;
    emit lastErrorChanged();
}

void MissionPlanController::setFilePath(const QString& path)
{
    if (filePath_ == path) {
        return;
    }
    filePath_ = path;
    emit filePathChanged();
}

void MissionPlanController::rememberLastFile(const QString& path)
{
    if (lastFile_ == path) {
        return;
    }
    lastFile_ = path;
    QSettings settings(QStringLiteral("KOGGER"), QStringLiteral("KoggerApp"));
    settings.setValue(QLatin1String(kSettingsLastFile), lastFile_);
    emit lastFileChanged();
}

void MissionPlanController::markClean()
{
    cleanSnapshot_ = snapshot();
    lastDirty_ = false;
    emit dirtyChanged();
}

MissionItem* MissionPlanController::findItem(const QString& id)
{
    const int idx = indexOfItem(id);
    return idx >= 0 ? &plan_.items[idx] : nullptr;
}

RallyItem* MissionPlanController::findRally(const QString& id)
{
    for (auto& r : plan_.rally) {
        if (r.id == id) {
            return &r;
        }
    }
    return nullptr;
}

bool MissionPlanController::parsePoints(const QVariantList& list, QVector<GeoPoint>* out, int minCount)
{
    out->clear();
    for (const auto& v : list) {
        const QVariantList pair = v.toList();
        double lat = NAN;
        double lon = NAN;
        if (pair.size() != 2 || !numberFrom(pair[0], &lat) || !numberFrom(pair[1], &lon)) {
            setLastError(tr("points: each vertex must be [lat, lon]"));
            return false;
        }
        const GeoPoint p(lat, lon);
        if (!p.isValid()) {
            setLastError(tr("points: vertex out of range"));
            return false;
        }
        out->append(p);
    }
    if (out->size() < minCount) {
        setLastError(tr("points: needs at least %1 vertices").arg(minCount));
        return false;
    }
    return true;
}

int MissionPlanController::clampInsertIndex(int index) const
{
    if (index < 0 || index > plan_.items.size()) {
        return plan_.items.size();
    }
    return index;
}

} // namespace mission
