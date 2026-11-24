#pragma once

#include <atomic>

#include <QHash>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

namespace autopilot {

/** One parameter's description from the bundled ArduPilot parameter metadata. */
struct ParamInfo
{
    QString displayName;
    QString description;
    QString units;
    bool hasRange = false;
    double low = 0.0;
    double high = 0.0;
    double increment = 0.0;
    QVector<QPair<double, QString>> values;
    QVector<QPair<int, QString>> bits;
    bool rebootRequired = false;
    bool readOnly = false;
};

/**
 * Display names, descriptions, units, ranges, value lists and bitmasks of ArduPilot parameters,
 * from the bundled `:/autopilot/rover.params.json` (derived from ArduPilot's per-release
 * apm.pdef.json). Only ArduRover (ground rover or surface boat) has a table. Units are rewritten
 * for display: repeated factors become powers (m/s/s → m/s², kg/m/m/m → kg/m³, m.m → m²), deg →
 * °, degC → °C, Ohm → Ω, us → µs; everything else stays as ArduPilot writes it.
 *
 * load() takes the bundled release closest to the firmware's major.minor without being newer,
 * or the oldest one for older firmware; without a known firmware version the newest. load() is
 * slow and thread-safe; a loaded table is a plain value and keeps only the selected release.
 */
class ParamMetadata
{
public:
    /** The table the vehicle needs ("Rover 4.6" for firmware 4.6.2, "Rover " with no version); empty when none applies. */
    static QString keyFor(int autopilotKind, int vehicleType, const QString& firmwareVersion);
    /** Parses the bundled file for @p key from keyFor(); safe on any thread. Setting @p cancel stops it with an empty key. */
    static ParamMetadata load(const QString& key, const std::atomic_bool* cancel = nullptr);

    /** The keyFor() value this table was loaded for; empty for no table. */
    QString key() const { return key_; }
    const ParamInfo* find(const QString& name) const;
    /** "Rover 4.6"; empty when no table applies. */
    QString source() const;
    /** True when the selected release is the firmware's own major.minor. */
    bool exact() const { return exact_; }

    /** The label of @p value from the value list, or the set bits' labels joined; empty otherwise. */
    static QString valueLabel(const ParamInfo& info, float value);

private:
    QString key_;
    QString vehicle_;
    QString release_;
    bool exact_ = false;
    QHash<QString, ParamInfo> table_;
};

} // namespace autopilot
