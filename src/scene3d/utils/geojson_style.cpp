#include "geojson_style.h"

#include <QJsonValue>

static double clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static QColor parseColor(const QJsonValue& v, const QColor& fallback)
{
    if (!v.isString()) {
        return fallback;
    }
    const QString s = v.toString().trimmed();
    const QColor c(s);
    return c.isValid() ? c : fallback;
}

static double parseNumber(const QJsonValue& v, double fallback)
{
    if (v.isDouble()) {
        return v.toDouble();
    }
    if (v.isString()) {
        bool ok = false;
        const double d = v.toString().toDouble(&ok);
        return ok ? d : fallback;
    }
    return fallback;
}

static double parseMarkerSizePx(const QJsonValue& v, double fallbackPx)
{
    if (v.isString()) {
        const QString s = v.toString().trimmed().toLower();
        if (s == "small")  return 7.0;
        if (s == "medium") return 11.0;
        if (s == "large")  return 15.0;
        bool ok = false;
        const double d = s.toDouble(&ok);
        return ok ? d : fallbackPx;
    }

    if (v.isDouble()) {
        return v.toDouble();
    }

    return fallbackPx;
}

namespace GeoJsonCss
{
GeoJsonStyle parseStyle(const QJsonObject& properties)
{
    GeoJsonStyle s;

    s.stroke = parseColor(properties.value("stroke"), s.stroke);
    s.strokeWidthPx = parseNumber(properties.value("stroke-width"), s.strokeWidthPx);
    s.strokeOpacity = clamp01(parseNumber(properties.value("stroke-opacity"), s.strokeOpacity));

    s.fill = parseColor(properties.value("fill"), s.fill);
    s.fillOpacity = clamp01(parseNumber(properties.value("fill-opacity"), s.fillOpacity));

    s.markerColor = parseColor(properties.value("marker-color"), s.markerColor);
    s.markerSizePx = parseMarkerSizePx(properties.value("marker-size"), s.markerSizePx);

    if (s.strokeWidthPx < 0.0) s.strokeWidthPx = 0.0;
    if (s.markerSizePx < 0.0) s.markerSizePx = 0.0;

    return s;
}

void writeStyle(QJsonObject& properties, const GeoJsonStyle& style)
{
    properties.insert("stroke", style.stroke.name(QColor::HexRgb));
    properties.insert("stroke-width", style.strokeWidthPx);
    properties.insert("stroke-opacity", clamp01(style.strokeOpacity));

    properties.insert("fill", style.fill.name(QColor::HexRgb));
    properties.insert("fill-opacity", clamp01(style.fillOpacity));

    properties.insert("marker-color", style.markerColor.name(QColor::HexRgb));
    properties.insert("marker-size", style.markerSizePx);
}
} // namespace GeoJsonCss

