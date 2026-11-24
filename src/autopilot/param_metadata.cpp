#include "param_metadata.h"

#include <cmath>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace autopilot {

namespace {

constexpr int kMavAutopilotArdupilot = 3;
constexpr int kMavTypeGroundRover = 10;
constexpr int kMavTypeSurfaceBoat = 11;
constexpr int kMinorPerMajor = 1000;
constexpr int kMaxBit = 31;
const QString kRoverKeyPrefix = QStringLiteral("Rover ");

int versionRank(const QString& version)
{
    const QStringList parts = version.split(QLatin1Char('.'));
    if (parts.size() < 2) {
        return -1;
    }
    bool majorOk = false;
    bool minorOk = false;
    const int major = parts.at(0).toInt(&majorOk);
    const int minor = parts.at(1).toInt(&minorOk);
    return majorOk && minorOk ? major * kMinorPerMajor + minor : -1;
}

QString pickRelease(const QStringList& releases, const QString& requested)
{
    const int wanted = versionRank(requested);
    QString best;
    int bestRank = -1;
    QString oldest;
    int oldestRank = -1;
    QString newest;
    int newestRank = -1;
    for (const auto& release : releases) {
        const int rank = versionRank(release);
        if (rank < 0) {
            continue;
        }
        if (oldestRank < 0 || rank < oldestRank) {
            oldest = release;
            oldestRank = rank;
        }
        if (rank > newestRank) {
            newest = release;
            newestRank = rank;
        }
        if (wanted >= 0 && rank <= wanted && rank > bestRank) {
            best = release;
            bestRank = rank;
        }
    }
    if (wanted < 0) {
        return newest;
    }
    return best.isEmpty() ? oldest : best;
}

QString unitSymbol(const QString& unit)
{
    if (unit == QLatin1String("deg")) {
        return QString(QChar(0x00B0));
    }
    if (unit == QLatin1String("degC")) {
        return QString(QChar(0x00B0)) + QLatin1Char('C');
    }
    if (unit == QLatin1String("Ohm")) {
        return QString(QChar(0x03A9));
    }
    if (unit == QLatin1String("us")) {
        return QString(QChar(0x00B5)) + QLatin1Char('s');
    }
    return unit;
}

QString withPower(const QString& unit, int power, QChar separator)
{
    switch (power) {
    case 1: return unit;
    case 2: return unit + QChar(0x00B2);
    case 3: return unit + QChar(0x00B3);
    default: return QStringList(power, unit).join(separator);
    }
}

QString displayUnits(const QString& units)
{
    const QStringList parts = units.split(QLatin1Char('/'));
    const QStringList factors = parts.first().split(QLatin1Char('.'));
    QString out = factors.size() > 1 && factors.count(factors.first()) == factors.size()
                      ? withPower(unitSymbol(factors.first()), int(factors.size()), QLatin1Char('.'))
                      : unitSymbol(parts.first());
    for (int i = 1; i < parts.size();) {
        int run = 1;
        while (i + run < parts.size() && parts.at(i + run) == parts.at(i)) {
            ++run;
        }
        out += QLatin1Char('/') + withPower(unitSymbol(parts.at(i)), run, QLatin1Char('/'));
        i += run;
    }
    return out;
}

ParamInfo parseInfo(const QJsonObject& o)
{
    ParamInfo info;
    info.displayName = o.value(QStringLiteral("name")).toString();
    info.description = o.value(QStringLiteral("desc")).toString();
    info.units = displayUnits(o.value(QStringLiteral("units")).toString());
    const QJsonArray range = o.value(QStringLiteral("range")).toArray();
    if (range.size() == 2) {
        info.hasRange = true;
        info.low = range.at(0).toDouble();
        info.high = range.at(1).toDouble();
    }
    info.increment = o.value(QStringLiteral("inc")).toDouble();
    for (const auto& v : o.value(QStringLiteral("values")).toArray()) {
        const QJsonArray pair = v.toArray();
        info.values.append({ pair.at(0).toDouble(), pair.at(1).toString() });
    }
    for (const auto& v : o.value(QStringLiteral("bits")).toArray()) {
        const QJsonArray pair = v.toArray();
        const int bit = pair.at(0).toInt(-1);
        if (bit >= 0 && bit <= kMaxBit) {
            info.bits.append({ bit, pair.at(1).toString() });
        }
    }
    info.rebootRequired = o.value(QStringLiteral("reboot")).toBool();
    info.readOnly = o.value(QStringLiteral("readOnly")).toBool();
    return info;
}

} // namespace

QString ParamMetadata::keyFor(int autopilotKind, int vehicleType, const QString& firmwareVersion)
{
    const bool rover = autopilotKind == kMavAutopilotArdupilot
                       && (vehicleType == kMavTypeGroundRover || vehicleType == kMavTypeSurfaceBoat);
    return rover ? kRoverKeyPrefix + firmwareVersion.section(QLatin1Char('.'), 0, 1) : QString();
}

const ParamInfo* ParamMetadata::find(const QString& name) const
{
    const auto it = table_.constFind(name);
    return it == table_.cend() ? nullptr : &it.value();
}

QString ParamMetadata::source() const
{
    return release_.isEmpty() ? QString() : vehicle_ + QLatin1Char(' ') + release_;
}

QString ParamMetadata::valueLabel(const ParamInfo& info, float value)
{
    for (const auto& v : info.values) {
        if (std::fabs(v.first - double(value)) < 1e-6) {
            return v.second;
        }
    }
    if (info.bits.isEmpty()) {
        return {};
    }
    const qint64 mask = std::llround(value);
    QStringList set;
    for (const auto& b : info.bits) {
        if (mask & (qint64(1) << b.first)) {
            set.append(b.second);
        }
    }
    return set.join(QStringLiteral(", "));
}

ParamMetadata ParamMetadata::load(const QString& key, const std::atomic_bool* cancel)
{
    const auto cancelled = [cancel]() { return cancel && cancel->load(std::memory_order_relaxed); };
    ParamMetadata out;
    out.key_ = key;
    if (!key.startsWith(kRoverKeyPrefix)) {
        return out;
    }
    const QString requested = key.mid(kRoverKeyPrefix.size());
    if (cancelled()) {
        return {};
    }
    QFile file(QStringLiteral(":/autopilot/rover.params.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return out;
    }
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    if (cancelled()) {
        return {};
    }
    QStringList releases;
    for (const auto& v : root.value(QStringLiteral("versions")).toArray()) {
        releases.append(v.toString());
    }
    const QString release = pickRelease(releases, requested);
    if (release.isEmpty()) {
        return out;
    }
    const QJsonValue releaseValue(release);
    const QJsonObject params = root.value(QStringLiteral("params")).toObject();
    out.table_.reserve(params.size());
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        if (cancelled()) {
            return {};
        }
        for (const auto& variant : it.value().toArray()) {
            const QJsonObject o = variant.toObject();
            if (o.value(QStringLiteral("v")).toArray().contains(releaseValue)) {
                out.table_.insert(it.key(), parseInfo(o));
                break;
            }
        }
    }
    out.vehicle_ = root.value(QStringLiteral("vehicle")).toString();
    out.release_ = release;
    out.exact_ = release == requested;
    return out;
}

} // namespace autopilot
