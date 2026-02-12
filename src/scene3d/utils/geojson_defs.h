#pragma once

#include <QColor>
#include <QJsonObject>
#include <QString>
#include <QVector>

struct GeoJsonCoord
{
    double lon = 0.0;
    double lat = 0.0;
    double z = 0.0;
    bool hasZ = false;
};

inline bool operator==(const GeoJsonCoord& a, const GeoJsonCoord& b)
{
    return a.lon == b.lon && a.lat == b.lat && a.z == b.z && a.hasZ == b.hasZ;
}

inline bool operator!=(const GeoJsonCoord& a, const GeoJsonCoord& b)
{
    return !(a == b);
}

enum class GeoJsonGeometryType {
    Point = 0,
    LineString,
    Polygon
};

struct GeoJsonStyle
{
    QColor stroke{0, 200, 255};
    double strokeWidthPx = 2.0;
    double strokeOpacity = 1.0;

    QColor fill{0, 0, 0};
    double fillOpacity = 0.0;

    QColor markerColor{255, 255, 255};
    double markerSizePx = 10.0;
};

struct GeoJsonFeature
{
    QString id;
    QString name;
    GeoJsonGeometryType geomType{GeoJsonGeometryType::Point};
    bool visible{true};

    // Point: size=1, LineString: n>=2, Polygon: ring vertices (not necessarily closed in memory)
    QVector<GeoJsonCoord> coords;

    QJsonObject properties;  // stored as-is and written back on export
    GeoJsonStyle style;      // parsed from properties (Geojson_CSS subset)
};

struct GeoJsonDocument
{
    QVector<GeoJsonFeature> features;
};
