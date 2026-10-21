#pragma once

#include <QPointF>
#include <QVector>

#include "mission_defs.h"

namespace mission {

class LocalPlane {
public:
    explicit LocalPlane(const GeoPoint& origin);

    QPointF toLocal(const GeoPoint& p) const;
    GeoPoint toGeo(const QPointF& p) const;

private:
    GeoPoint origin_;
    double   latRad_ = 0.0;
    double   lonRad_ = 0.0;
    double   sinLat_ = 0.0;
    double   cosLat_ = 1.0;
};

double   geoDistance(const GeoPoint& a, const GeoPoint& b);
double   bearingDeg(const GeoPoint& from, const GeoPoint& to);
GeoPoint centroid(const QVector<GeoPoint>& pts);
double   polygonAreaLocal(const QVector<QPointF>& poly);
bool     polygonSelfIntersects(const QVector<QPointF>& poly);
GeoPoint greatCirclePoint(const GeoPoint& a, const GeoPoint& b, double t);
double   wrapLongitude(double lon);

QVector<double> polygonLineCrossings(const QVector<QPointF>& poly, const QPointF& dirUnit, const QPointF& normalUnit, double offset);

QVector<QPointF> offsetPolyline(const QVector<QPointF>& axis, double distance, double bandHalfWidth, double miterLimit, QVector<int>* pinchedSegments = nullptr);

inline QPointF unit(const QPointF& v)
{
    const double len = std::hypot(v.x(), v.y());
    return len > 0.0 ? QPointF(v.x() / len, v.y() / len) : QPointF(0.0, 0.0);
}

inline double dot(const QPointF& a, const QPointF& b) { return a.x() * b.x() + a.y() * b.y(); }
inline double cross(const QPointF& a, const QPointF& b) { return a.x() * b.y() - a.y() * b.x(); }
inline QPointF rightNormal(const QPointF& dirUnit) { return QPointF(dirUnit.y(), -dirUnit.x()); }

} // namespace mission
