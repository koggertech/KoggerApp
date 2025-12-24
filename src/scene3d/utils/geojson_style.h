#pragma once

#include <QJsonObject>

#include "geojson_defs.h"

namespace GeoJsonCss
{
GeoJsonStyle parseStyle(const QJsonObject& properties);
void writeStyle(QJsonObject& properties, const GeoJsonStyle& style);
} // namespace GeoJsonCss

