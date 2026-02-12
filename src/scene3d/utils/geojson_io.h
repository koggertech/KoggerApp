#pragma once

#include <QString>
#include <QJsonObject>

#include "geojson_defs.h"

namespace GeoJsonIO
{
struct Result
{
    bool ok = false;
    QString error;
    GeoJsonDocument doc;
};

Result loadFromFile(const QString& path);
bool saveToFile(const QString& path, const GeoJsonDocument& doc, QString* outError = nullptr);
bool parseFeatureCollection(const QJsonObject& root, GeoJsonDocument* outDoc, QString* outError = nullptr);
QJsonObject writeFeatureCollection(const GeoJsonDocument& doc);
} // namespace GeoJsonIO
