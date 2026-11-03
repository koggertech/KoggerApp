#include "autopilot_params.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>
#include <QUrl>

#include "autopilot_messages.h"

namespace autopilot {

namespace {

constexpr int kRebuildDelayMs = 250;
constexpr double kExactFloatInteger = 16777216.0;
constexpr int kQgcColumns = 5;
constexpr int kPlannerColumns = 2;
const QString kOtherGroup = QStringLiteral("*");

bool isOpaqueUri(const QString& pathOrUri)
{
    const int scheme = pathOrUri.indexOf(QStringLiteral("://"));
    return scheme > 1 && !pathOrUri.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive);
}

bool integerRange(uint8_t type, double* low, double* high)
{
    switch (type) {
    case MavParamTypeUint8:  *low = 0.0; *high = 255.0; return true;
    case MavParamTypeInt8:   *low = -128.0; *high = 127.0; return true;
    case MavParamTypeUint16: *low = 0.0; *high = 65535.0; return true;
    case MavParamTypeInt16:  *low = -32768.0; *high = 32767.0; return true;
    case MavParamTypeUint32: *low = 0.0; *high = 4294967295.0; return true;
    case MavParamTypeInt32:  *low = -2147483648.0; *high = 2147483647.0; return true;
    default: return false;
    }
}

} // namespace

AutopilotParamRows::AutopilotParamRows(AutopilotParams* owner)
    : QAbstractListModel(owner)
    , owner_(owner)
{
}

int AutopilotParamRows::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(entries_.size());
}

QVariant AutopilotParamRows::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= entries_.size()) {
        return {};
    }
    const AutopilotParams::Entry& e = owner_->entries_.at(entries_.at(index.row()));
    switch (role) {
    case NameRole:      return e.name;
    case ValueTextRole: return AutopilotParams::formatValue(e.value, e.type);
    case TypeTextRole:  return AutopilotParams::typeText(e.type);
    case PendingRole:   return e.pending;
    case FailedRole:    return e.failed;
    default:            return {};
    }
}

QHash<int, QByteArray> AutopilotParamRows::roleNames() const
{
    return {
        { NameRole, "name" },
        { ValueTextRole, "valueText" },
        { TypeTextRole, "typeText" },
        { PendingRole, "pending" },
        { FailedRole, "failed" }
    };
}

void AutopilotParamRows::reset(const QVector<int>& entries)
{
    beginResetModel();
    entries_ = entries;
    endResetModel();
}

void AutopilotParamRows::entryChanged(int entry)
{
    const int row = int(entries_.indexOf(entry));
    if (row >= 0) {
        const QModelIndex i = index(row);
        emit dataChanged(i, i);
    }
}

AutopilotParams::AutopilotParams(QObject* parent)
    : QObject(parent)
{
    rebuildTimer_.setSingleShot(true);
    rebuildTimer_.setInterval(kRebuildDelayMs);
    connect(&rebuildTimer_, &QTimer::timeout, this, &AutopilotParams::rebuild);

    connect(&transfer_, &ParamTransfer::sendMessage, this, &AutopilotParams::sendMessage);
    connect(&transfer_, &ParamTransfer::valueReceived, this, &AutopilotParams::onValueReceived);
    connect(&transfer_, &ParamTransfer::progressChanged, this, &AutopilotParams::onProgress);
    connect(&transfer_, &ParamTransfer::downloadFinished, this, &AutopilotParams::onDownloadFinished);
    connect(&transfer_, &ParamTransfer::writeFinished, this, &AutopilotParams::onWriteFinished);
}

void AutopilotParams::setCurrentGroup(const QString& group)
{
    if (currentGroup_ == group) {
        return;
    }
    currentGroup_ = group;
    applyFilter();
    emit filterChanged();
}

void AutopilotParams::setSearchText(const QString& text)
{
    if (searchText_ == text) {
        return;
    }
    searchText_ = text;
    applyFilter();
    emit filterChanged();
}

QVariantList AutopilotParams::diffRows() const
{
    QVariantList out;
    out.reserve(diff_.size());
    for (const auto& d : diff_) {
        out.append(QVariantMap{
            { QStringLiteral("name"), d.name },
            { QStringLiteral("current"), d.current },
            { QStringLiteral("file"), d.file },
            { QStringLiteral("selected"), d.selected }
        });
    }
    return out;
}

int AutopilotParams::diffSelected() const
{
    return int(std::count_if(diff_.cbegin(), diff_.cend(), [](const DiffRow& d) { return d.selected; }));
}

void AutopilotParams::refresh()
{
    if (!online_) {
        return;
    }
    setLastError(QString());
    loadFailed_ = false;
    missing_ = 0;
    setLoading(true);
    transfer_.startDownload();
}

void AutopilotParams::ensureLoaded()
{
    if (online_ && !loading_ && (!loaded_ || stale_)) {
        refresh();
    }
}

QString AutopilotParams::validate(const QString& name, const QString& text) const
{
    const auto it = byName_.constFind(name);
    if (it == byName_.cend()) {
        return tr("The vehicle has no parameter %1").arg(name);
    }
    float value = 0.0f;
    QString error;
    parseValue(entries_.at(it.value()), text, &value, &error);
    return error;
}

bool AutopilotParams::setValue(const QString& name, const QString& text)
{
    const auto it = byName_.constFind(name);
    if (it == byName_.cend()) {
        setLastError(tr("The vehicle has no parameter %1").arg(name));
        return false;
    }
    Entry& e = entries_[it.value()];
    float value = 0.0f;
    QString error;
    if (!parseValue(e, text, &value, &error)) {
        setLastError(error);
        return false;
    }
    setLastError(QString());
    e.pending = true;
    e.failed = false;
    rows_.entryChanged(it.value());
    transfer_.write(name, value, e.type);
    emit writesChanged();
    return true;
}

QString AutopilotParams::valueText(const QString& name) const
{
    const auto it = byName_.constFind(name);
    if (it == byName_.cend()) {
        return {};
    }
    const Entry& e = entries_.at(it.value());
    return formatValue(e.value, e.type);
}

bool AutopilotParams::saveToFile(const QString& pathOrUrl)
{
    const QString path = localPath(pathOrUrl);
    if (path.isEmpty()) {
        setLastError(tr("No file chosen"));
        return false;
    }
    QByteArray bytes;
    QTextStream out(&bytes);
    out << "# Onboard parameters for Vehicle " << systemId_ << "\n";
    out << "#\n";
    out << "# Vehicle-Id Component-Id Name Value Type\n";
    if (order_.size() != entries_.size()) {
        rebuild();
    }
    for (const int i : std::as_const(order_)) {
        const Entry& e = entries_.at(i);
        out << systemId_ << '\t' << componentId_ << '\t' << e.name << '\t' << formatValue(e.value, e.type) << '\t' << int(e.type) << "\n";
    }
    out.flush();

    if (isOpaqueUri(path)) {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate) || f.write(bytes) != bytes.size()) {
            setLastError(tr("Cannot write the file (%1)").arg(f.errorString()));
            return false;
        }
        setLastError(QString());
        return true;
    }
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate) || f.write(bytes) != bytes.size() || !f.commit()) {
        setLastError(tr("Cannot write the file (%1)").arg(f.errorString()));
        return false;
    }
    setLastError(QString());
    return true;
}

bool AutopilotParams::compareFile(const QString& pathOrUrl)
{
    clearDiff();
    const QString path = localPath(pathOrUrl);
    QFile f(path);
    if (path.isEmpty() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setLastError(tr("Cannot open the file (%1)").arg(f.errorString()));
        return false;
    }

    static const QRegularExpression separators(QStringLiteral("[\\t ,]+"));
    int parsed = 0;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const QStringList fields = line.split(separators, Qt::SkipEmptyParts);
        QString name;
        QString text;
        if (fields.size() == kQgcColumns) {
            name = fields.at(2);
            text = fields.at(3);
        } else if (fields.size() == kPlannerColumns) {
            name = fields.at(0);
            text = fields.at(1);
        } else {
            continue;
        }
        ++parsed;

        const auto it = byName_.constFind(name);
        if (it == byName_.cend()) {
            diffMissing_.append(name);
            continue;
        }
        const Entry& e = entries_.at(it.value());
        float value = 0.0f;
        QString error;
        if (!parseValue(e, text, &value, &error)) {
            diffInvalid_.append(name);
            continue;
        }
        if (ParamTransfer::sameValue(value, e.value, e.type)) {
            ++diffUnchanged_;
            continue;
        }
        diff_.append(DiffRow{ name, formatValue(e.value, e.type), formatValue(value, e.type), value, e.type, true });
    }

    std::sort(diff_.begin(), diff_.end(), [](const DiffRow& a, const DiffRow& b) { return a.name < b.name; });
    emit diffChanged();
    if (parsed == 0) {
        setLastError(tr("No parameters found in the file"));
        return false;
    }
    setLastError(QString());
    return true;
}

void AutopilotParams::setDiffSelected(int row, bool selected)
{
    if (row < 0 || row >= diff_.size() || diff_.at(row).selected == selected) {
        return;
    }
    diff_[row].selected = selected;
    emit diffChanged();
}

void AutopilotParams::setAllDiffSelected(bool selected)
{
    for (auto& d : diff_) {
        d.selected = selected;
    }
    emit diffChanged();
}

int AutopilotParams::applyDiff()
{
    int queued = 0;
    for (const auto& d : std::as_const(diff_)) {
        if (!d.selected) {
            continue;
        }
        const auto it = byName_.constFind(d.name);
        if (it == byName_.cend()) {
            continue;
        }
        Entry& e = entries_[it.value()];
        e.pending = true;
        e.failed = false;
        rows_.entryChanged(it.value());
        transfer_.write(d.name, d.value, e.type);
        ++queued;
    }
    clearDiff();
    emit writesChanged();
    return queued;
}

void AutopilotParams::clearDiff()
{
    diff_.clear();
    diffUnchanged_ = 0;
    diffMissing_.clear();
    diffInvalid_.clear();
    emit diffChanged();
}

void AutopilotParams::reboot()
{
    if (!online_) {
        return;
    }
    if (loaded_) {
        stale_ = true;
        emit stateChanged();
    }
    emit rebootRequested();
}

void AutopilotParams::setVehicle(bool online, int systemId, int componentId)
{
    const bool otherVehicle = online && (systemId != systemId_ || componentId != componentId_) && systemId_ >= 0;
    if (online) {
        systemId_ = systemId;
        componentId_ = componentId;
        transfer_.setTarget(systemId, componentId);
    }
    if (otherVehicle) {
        transfer_.abort(ParamTransfer::ResultCancelled);
        clearAll();
    }
    if (online_ == online && !otherVehicle) {
        return;
    }
    if (!online && online_) {
        transfer_.abort(ParamTransfer::ResultLinkLost);
        if (loaded_) {
            stale_ = true;
        }
    }
    online_ = online;
    emit stateChanged();
}

void AutopilotParams::handleParamValue(const QByteArray& payload)
{
    transfer_.handleParamValue(payload);
}

void AutopilotParams::onValueReceived(const QString& name, float value, int type, int index, int count)
{
    Q_UNUSED(count)
    const auto it = byName_.constFind(name);
    if (it != byName_.cend()) {
        Entry& e = entries_[it.value()];
        e.value = value;
        e.type = uint8_t(type);
        if (index != ParamTransfer::kUnrequestedIndex) {
            e.index = index;
        }
        rows_.entryChanged(it.value());
        return;
    }
    Entry e;
    e.name = name;
    e.value = value;
    e.type = uint8_t(type);
    e.index = index == ParamTransfer::kUnrequestedIndex ? -1 : index;
    entries_.append(e);
    byName_.insert(name, int(entries_.size()) - 1);
    scheduleRebuild();
}

void AutopilotParams::onProgress(int received, int total)
{
    received_ = received;
    total_ = total;
    emit progressChanged();
}

void AutopilotParams::onDownloadFinished(bool ok, int result, int missing)
{
    Q_UNUSED(result)
    loaded_ = ok;
    stale_ = false;
    loadFailed_ = !ok;
    missing_ = missing;
    setLoading(false);
    rebuild();
}

void AutopilotParams::onWriteFinished(const QString& name, bool ok, int result)
{
    Q_UNUSED(result)
    const auto it = byName_.constFind(name);
    if (it != byName_.cend()) {
        Entry& e = entries_[it.value()];
        e.pending = false;
        e.failed = !ok;
        rows_.entryChanged(it.value());
        const QString actual = formatValue(e.value, e.type);
        if (ok) {
            emit writeConfirmed(name, actual);
        } else {
            emit writeFailed(name, actual);
        }
    }
    emit writesChanged();
}

void AutopilotParams::clearAll()
{
    entries_.clear();
    byName_.clear();
    loaded_ = false;
    stale_ = false;
    loadFailed_ = false;
    missing_ = 0;
    received_ = 0;
    total_ = 0;
    clearDiff();
    rebuild();
    emit progressChanged();
}

void AutopilotParams::scheduleRebuild()
{
    if (!rebuildTimer_.isActive()) {
        rebuildTimer_.start();
    }
}

void AutopilotParams::rebuild()
{
    rebuildTimer_.stop();
    order_.resize(entries_.size());
    for (int i = 0; i < order_.size(); ++i) {
        order_[i] = i;
    }
    std::sort(order_.begin(), order_.end(), [this](int a, int b) { return entries_.at(a).name < entries_.at(b).name; });
    rebuildGroups();
    applyFilter();
    emit listChanged();
}

void AutopilotParams::rebuildGroups()
{
    QHash<QString, int> counts;
    for (const auto& e : std::as_const(entries_)) {
        ++counts[groupOf(e.name)];
    }
    int other = 0;
    for (auto& e : entries_) {
        const QString g = groupOf(e.name);
        e.group = g.isEmpty() || counts.value(g) < 2 ? kOtherGroup : g;
        if (e.group == kOtherGroup) {
            ++other;
        }
    }

    QStringList keys;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        if (!it.key().isEmpty() && it.value() >= 2) {
            keys.append(it.key());
        }
    }
    keys.sort();

    groups_.clear();
    groups_.append(QVariantMap{ { QStringLiteral("key"), QString() }, { QStringLiteral("title"), tr("All") },
                                { QStringLiteral("count"), int(entries_.size()) } });
    for (const auto& k : std::as_const(keys)) {
        groups_.append(QVariantMap{ { QStringLiteral("key"), k }, { QStringLiteral("title"), k },
                                    { QStringLiteral("count"), counts.value(k) } });
    }
    if (other > 0) {
        groups_.append(QVariantMap{ { QStringLiteral("key"), kOtherGroup }, { QStringLiteral("title"), tr("Other") },
                                    { QStringLiteral("count"), other } });
    }
}

void AutopilotParams::applyFilter()
{
    QVector<int> visible;
    visible.reserve(order_.size());
    for (const int i : std::as_const(order_)) {
        if (matchesFilter(entries_.at(i))) {
            visible.append(i);
        }
    }
    rows_.reset(visible);
    emit visibleChanged();
}

bool AutopilotParams::matchesFilter(const Entry& e) const
{
    const QStringList words = searchText_.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (!words.isEmpty()) {
        return std::all_of(words.cbegin(), words.cend(),
                           [&e](const QString& w) { return e.name.contains(w, Qt::CaseInsensitive); });
    }
    return currentGroup_.isEmpty() || e.group == currentGroup_;
}

bool AutopilotParams::parseValue(const Entry& e, const QString& text, float* out, QString* error) const
{
    QString clean = text.trimmed();
    clean.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    const double v = clean.toDouble(&ok);
    if (!ok || !std::isfinite(v)) {
        *error = tr("Not a number");
        return false;
    }
    double low = 0.0;
    double high = 0.0;
    if (integerRange(e.type, &low, &high)) {
        if (std::fabs(v - std::round(v)) > 1e-9) {
            *error = tr("Whole number expected");
            return false;
        }
        if (v < low || v > high) {
            *error = tr("Out of range %1…%2").arg(qint64(low)).arg(qint64(high));
            return false;
        }
        if (std::fabs(v) > kExactFloatInteger) {
            *error = tr("Too large to send exactly (more than %1)").arg(qint64(kExactFloatInteger));
            return false;
        }
        *out = float(std::round(v));
        error->clear();
        return true;
    }
    if (std::fabs(v) > double(FLT_MAX)) {
        *error = tr("Out of range");
        return false;
    }
    *out = float(v);
    error->clear();
    return true;
}

void AutopilotParams::setLastError(const QString& error)
{
    if (lastError_ == error) {
        return;
    }
    lastError_ = error;
    emit lastErrorChanged();
}

void AutopilotParams::setLoading(bool loading)
{
    loading_ = loading;
    emit stateChanged();
}

QString AutopilotParams::groupOf(const QString& name)
{
    static const QRegularExpression trailingDigits(QStringLiteral("[0-9]+$"));
    QString g = name.section(QLatin1Char('_'), 0, 0);
    g.remove(trailingDigits);
    return g;
}

QString AutopilotParams::formatValue(float value, uint8_t type)
{
    if (type != MavParamTypeReal32) {
        return QString::number(qint64(std::llround(value)));
    }
    return QString::number(double(value), 'g', 7);
}

QString AutopilotParams::typeText(uint8_t type)
{
    switch (type) {
    case MavParamTypeUint8:  return QStringLiteral("uint8");
    case MavParamTypeInt8:   return QStringLiteral("int8");
    case MavParamTypeUint16: return QStringLiteral("uint16");
    case MavParamTypeInt16:  return QStringLiteral("int16");
    case MavParamTypeUint32: return QStringLiteral("uint32");
    case MavParamTypeInt32:  return QStringLiteral("int32");
    case MavParamTypeReal32: return QStringLiteral("float");
    default:                 return QString::number(type);
    }
}

QString AutopilotParams::localPath(const QString& pathOrUrl)
{
    if (pathOrUrl.startsWith(QStringLiteral("file:"))) {
        return QUrl(pathOrUrl).toLocalFile();
    }
    return pathOrUrl;
}

} // namespace autopilot
