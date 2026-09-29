#include "mission_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mission {

namespace {

constexpr double kEarthRadius = 6371000.0;
constexpr double kDegToRad = M_PI / 180.0;
constexpr double kRadToDeg = 180.0 / M_PI;

bool segmentsIntersect(const QPointF& p1, const QPointF& p2, const QPointF& q1, const QPointF& q2)
{
    const double d1 = cross(q2 - q1, p1 - q1);
    const double d2 = cross(q2 - q1, p2 - q1);
    const double d3 = cross(p2 - p1, q1 - p1);
    const double d4 = cross(p2 - p1, q2 - p1);
    return ((d1 > 0.0) != (d2 > 0.0)) && ((d3 > 0.0) != (d4 > 0.0));
}

bool lineIntersection(const QPointF& a, const QPointF& da, const QPointF& b, const QPointF& db, QPointF& out)
{
    const double den = cross(da, db);
    if (std::fabs(den) < 1e-12) {
        return false;
    }
    const double t = cross(b - a, db) / den;
    out = a + da * t;
    return true;
}

} // namespace

LocalPlane::LocalPlane(const GeoPoint& origin)
    : origin_(origin)
    , latRad_(origin.lat * kDegToRad)
    , lonRad_(origin.lon * kDegToRad)
    , sinLat_(std::sin(latRad_))
    , cosLat_(std::cos(latRad_))
{
}

QPointF LocalPlane::toLocal(const GeoPoint& p) const
{
    const double latRad = p.lat * kDegToRad;
    const double lonRad = p.lon * kDegToRad;
    const double sinLat = std::sin(latRad);
    const double cosLat = std::cos(latRad);
    const double cosDLon = std::cos(lonRad - lonRad_);

    double arg = sinLat_ * sinLat + cosLat_ * cosLat * cosDLon;
    arg = std::clamp(arg, -1.0, 1.0);
    const double c = std::acos(arg);
    const double k = (std::fabs(c) < 1e-12) ? 1.0 : (c / std::sin(c));

    const double north = k * (cosLat_ * sinLat - sinLat_ * cosLat * cosDLon) * kEarthRadius;
    const double east = k * cosLat * std::sin(lonRad - lonRad_) * kEarthRadius;
    return QPointF(east, north);
}

GeoPoint LocalPlane::toGeo(const QPointF& p) const
{
    const double east = p.x();
    const double north = p.y();
    const double c = std::hypot(north, east) / kEarthRadius;
    if (c < 1e-15) {
        return origin_;
    }
    const double sinC = std::sin(c);
    const double cosC = std::cos(c);
    const double latRad = std::asin(cosC * sinLat_ + (north * sinC * cosLat_) / (c * kEarthRadius));
    const double lonRad = lonRad_ + std::atan2(east * sinC, c * kEarthRadius * cosLat_ * cosC - north * sinLat_ * sinC);
    return GeoPoint(latRad * kRadToDeg, wrapLongitude(lonRad * kRadToDeg));
}

double geoDistance(const GeoPoint& a, const GeoPoint& b)
{
    if (!a.isValid() || !b.isValid()) {
        return 0.0;
    }
    const double lat1 = a.lat * kDegToRad;
    const double lat2 = b.lat * kDegToRad;
    const double dLat = lat2 - lat1;
    const double dLon = (b.lon - a.lon) * kDegToRad;
    const double h = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) + std::cos(lat1) * std::cos(lat2) * std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    return 2.0 * kEarthRadius * std::asin(std::sqrt(std::clamp(h, 0.0, 1.0)));
}

double bearingDeg(const GeoPoint& from, const GeoPoint& to)
{
    const double lat1 = from.lat * kDegToRad;
    const double lat2 = to.lat * kDegToRad;
    const double dLon = (to.lon - from.lon) * kDegToRad;
    const double y = std::sin(dLon) * std::cos(lat2);
    const double x = std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(dLon);
    double deg = std::atan2(y, x) * kRadToDeg;
    if (deg < 0.0) {
        deg += 360.0;
    }
    return deg;
}

GeoPoint centroid(const QVector<GeoPoint>& pts)
{
    if (pts.isEmpty()) {
        return GeoPoint();
    }
    double lat = 0.0;
    double lon = 0.0;
    for (const auto& p : pts) {
        lat += p.lat;
        lon += p.lon;
    }
    return GeoPoint(lat / pts.size(), lon / pts.size());
}

double polygonAreaLocal(const QVector<QPointF>& poly)
{
    double sum = 0.0;
    const int n = poly.size();
    for (int i = 0; i < n; ++i) {
        const QPointF& a = poly[i];
        const QPointF& b = poly[(i + 1) % n];
        sum += cross(a, b);
    }
    return std::fabs(sum) * 0.5;
}

bool polygonSelfIntersects(const QVector<QPointF>& poly)
{
    const int n = poly.size();
    if (n < 4) {
        return false;
    }
    for (int i = 0; i < n; ++i) {
        const QPointF& p1 = poly[i];
        const QPointF& p2 = poly[(i + 1) % n];
        for (int j = i + 2; j < n; ++j) {
            if (i == 0 && j == n - 1) {
                continue;
            }
            if (segmentsIntersect(p1, p2, poly[j], poly[(j + 1) % n])) {
                return true;
            }
        }
    }
    return false;
}

double wrapLongitude(double lon)
{
    while (lon > 180.0) {
        lon -= 360.0;
    }
    while (lon < -180.0) {
        lon += 360.0;
    }
    return lon;
}

GeoPoint greatCirclePoint(const GeoPoint& a, const GeoPoint& b, double t)
{
    const double lat1 = a.lat * kDegToRad;
    const double lon1 = a.lon * kDegToRad;
    const double lat2 = b.lat * kDegToRad;
    const double lon2 = b.lon * kDegToRad;
    const double ax = std::cos(lat1) * std::cos(lon1);
    const double ay = std::cos(lat1) * std::sin(lon1);
    const double az = std::sin(lat1);
    const double bx = std::cos(lat2) * std::cos(lon2);
    const double by = std::cos(lat2) * std::sin(lon2);
    const double bz = std::sin(lat2);
    const double dot = std::clamp(ax * bx + ay * by + az * bz, -1.0, 1.0);
    const double omega = std::acos(dot);
    if (omega < 1e-9) {
        return GeoPoint(a.lat + (b.lat - a.lat) * t, wrapLongitude(a.lon + (b.lon - a.lon) * t));
    }
    const double s1 = std::sin((1.0 - t) * omega) / std::sin(omega);
    const double s2 = std::sin(t * omega) / std::sin(omega);
    const double x = ax * s1 + bx * s2;
    const double y = ay * s1 + by * s2;
    const double z = az * s1 + bz * s2;
    const double len = std::sqrt(x * x + y * y + z * z);
    return GeoPoint(std::asin(std::clamp(z / len, -1.0, 1.0)) * kRadToDeg, std::atan2(y, x) * kRadToDeg);
}

QVector<double> polygonLineCrossings(const QVector<QPointF>& poly, const QPointF& dirUnit, const QPointF& normalUnit, double offset)
{
    QVector<double> out;
    const int n = poly.size();
    for (int i = 0; i < n; ++i) {
        const QPointF& a = poly[i];
        const QPointF& b = poly[(i + 1) % n];
        const double ta = dot(a, normalUnit);
        const double tb = dot(b, normalUnit);
        const bool crossesUp = ta <= offset && tb > offset;
        const bool crossesDown = tb <= offset && ta > offset;
        if (!crossesUp && !crossesDown) {
            continue;
        }
        const double f = (offset - ta) / (tb - ta);
        const QPointF p = a + (b - a) * f;
        out.append(dot(p, dirUnit));
    }
    std::sort(out.begin(), out.end());
    return out;
}

QVector<QPointF> offsetPolyline(const QVector<QPointF>& axis, double distance, double bandHalfWidth, double miterLimit, QVector<int>* pinchedSegments)
{
    QVector<QPointF> out;
    const int n = axis.size();
    if (n < 2) {
        return out;
    }

    struct Seg {
        QPointF a;
        QPointF b;
        QPointF a0;
        QPointF b0;
        QPointF dir;
        int index;
        bool joinedPrev;
    };

    QVector<Seg> segs;
    segs.reserve(n - 1);
    for (int i = 0; i + 1 < n; ++i) {
        const QPointF dir = unit(axis[i + 1] - axis[i]);
        const QPointF off = rightNormal(dir) * distance;
        const QPointF a = axis[i] + off;
        const QPointF b = axis[i + 1] + off;
        segs.append(Seg{a, b, a, b, dir, i, false});
    }

    auto bevel = [](Seg& prev, Seg& next) {
        prev.b = prev.b0;
        next.a = next.a0;
        next.joinedPrev = false;
    };
    auto miter = [](Seg& prev, Seg& next, const QPointF& p) {
        prev.b = p;
        next.a = p;
        next.joinedPrev = true;
    };

    auto join = [&](Seg& prev, Seg& next) {
        QPointF p;
        if (!lineIntersection(prev.a, prev.dir, next.b, next.dir, p)) {
            bevel(prev, next);
            return;
        }
        if (cross(prev.dir, next.dir) * distance < 0.0) {
            miter(prev, next, p);
            return;
        }
        const QPointF& corner = axis[next.index];
        const double side = distance < 0.0 ? -bandHalfWidth : bandHalfWidth;
        const QPointF bp = corner + rightNormal(prev.dir) * side;
        const QPointF bn = corner + rightNormal(next.dir) * side;
        QPointF bandMiter;
        if (lineIntersection(bp, prev.dir, bn, next.dir, bandMiter)
            && std::hypot(bandMiter.x() - corner.x(), bandMiter.y() - corner.y()) <= miterLimit) {
            miter(prev, next, p);
            return;
        }
        const QPointF edge = bn - bp;
        if (cross(edge, p - bp) * cross(edge, corner - bp) >= 0.0) {
            miter(prev, next, p);
            return;
        }
        QPointF c1;
        QPointF c2;
        if (lineIntersection(prev.a, prev.dir, bp, edge, c1) && lineIntersection(next.b, next.dir, bp, edge, c2)) {
            prev.b = c1;
            next.a = c2;
            next.joinedPrev = false;
            return;
        }
        bevel(prev, next);
    };

    for (int i = 1; i < segs.size(); ++i) {
        join(segs[i - 1], segs[i]);
    }

    for (int pass = 0; pass < n && segs.size() > 1; ++pass) {
        int flipped = -1;
        for (int i = 0; i < segs.size(); ++i) {
            if (dot(segs[i].b - segs[i].a, segs[i].dir) <= 1e-9) {
                flipped = i;
                break;
            }
        }
        if (flipped < 0) {
            break;
        }
        if (pinchedSegments && !pinchedSegments->contains(segs[flipped].index)) {
            pinchedSegments->append(segs[flipped].index);
        }
        segs.removeAt(flipped);
        if (flipped > 0 && flipped < segs.size()) {
            join(segs[flipped - 1], segs[flipped]);
        } else if (flipped == 0) {
            segs[0].a = segs[0].a0;
            segs[0].joinedPrev = false;
        } else {
            segs.last().b = segs.last().b0;
        }
    }

    if (segs.size() == 1 && dot(segs[0].b - segs[0].a, segs[0].dir) <= 1e-9) {
        return out;
    }

    out.append(segs[0].a);
    for (int i = 0; i < segs.size(); ++i) {
        if (i > 0 && !segs[i].joinedPrev) {
            out.append(segs[i].a);
        }
        out.append(segs[i].b);
    }
    return out;
}

} // namespace mission
