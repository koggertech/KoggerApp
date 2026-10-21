#pragma once

#include <optional>

#include <QString>
#include <QStringList>
#include <QVector>

#include "mission_model.h"

namespace mission {

struct GeneratedPath {
    QVector<GeoPoint> points;
    QVector<QVector<GeoPoint>> lines;
    QVector<GeoPoint> band;
    int    lineCount = 0;
    int    resolvedEntry = 0;
    double lengthMeters = 0.0;
    double areaSquareMeters = 0.0;
    bool   spacingClamped = false;
    double effectiveSpacing = 0.0;
    QString error;
    QStringList warnings;

    bool ok() const { return error.isEmpty(); }
};

GeneratedPath generateSurvey(const SurveyItem& item, const std::optional<GeoPoint>& previous);
GeneratedPath generateCorridor(const CorridorItem& item, const std::optional<GeoPoint>& previous);

} // namespace mission
