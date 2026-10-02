#include "mission_generators.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QCoreApplication>

#include "mission_geometry.h"

namespace mission {

namespace {

struct SweepLine {
    double offset = 0.0;
    QVector<QPointF> segmentEnds;
};

double localExtent(const QVector<QPointF>& pts)
{
    if (pts.isEmpty()) {
        return 0.0;
    }
    double minX = pts[0].x(), maxX = pts[0].x(), minY = pts[0].y(), maxY = pts[0].y();
    for (const auto& p : pts) {
        minX = std::min(minX, p.x());
        maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y());
        maxY = std::max(maxY, p.y());
    }
    return std::max(maxX - minX, maxY - minY);
}

int nearestCorner(const QVector<QPointF>& corners, const std::optional<QPointF>& previous)
{
    if (!previous || corners.isEmpty()) {
        return 0;
    }
    int best = 0;
    double bestDist = std::numeric_limits<double>::max();
    for (int i = 0; i < corners.size(); ++i) {
        const double d = std::hypot(corners[i].x() - previous->x(), corners[i].y() - previous->y());
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

struct SweepResult {
    QVector<SweepLine> lines;
    double effectiveSpacing = 0.0;
    bool clamped = false;
};

SweepResult sweepPolygon(const QVector<QPointF>& poly, double angleDeg, double spacing, double turnaround, int maxLines)
{
    SweepResult res;
    const double a = angleDeg * M_PI / 180.0;
    const QPointF dir(std::sin(a), std::cos(a));
    const QPointF nrm = rightNormal(dir);

    double tMin = std::numeric_limits<double>::max();
    double tMax = std::numeric_limits<double>::lowest();
    for (const auto& p : poly) {
        const double t = dot(p, nrm);
        tMin = std::min(tMin, t);
        tMax = std::max(tMax, t);
    }
    const double span = tMax - tMin;
    if (span <= 0.0) {
        return res;
    }

    double step = std::max(spacing, kMinLineSpacing);
    int count = static_cast<int>(std::floor((span - step * 0.5) / step)) + 1;
    if (count > maxLines) {
        step = span / maxLines;
        count = maxLines;
        res.clamped = true;
    }
    if (count < 1) {
        count = 1;
    }
    res.effectiveSpacing = step;

    const double start = (count == 1) ? (tMin + span * 0.5) : (tMin + step * 0.5);
    for (int i = 0; i < count; ++i) {
        const double t = start + i * step;
        const QVector<double> s = polygonLineCrossings(poly, dir, nrm, t);
        SweepLine line;
        line.offset = t;
        for (int k = 0; k + 1 < s.size(); k += 2) {
            double s0 = s[k] - turnaround;
            double s1 = s[k + 1] + turnaround;
            if (s1 - s0 <= std::fabs(turnaround) * 2.0 || s1 - s0 <= 0.0) {
                continue;
            }
            line.segmentEnds.append(dir * s0 + nrm * t);
            line.segmentEnds.append(dir * s1 + nrm * t);
        }
        if (!line.segmentEnds.isEmpty()) {
            res.lines.append(line);
        }
    }
    return res;
}

QVector<QPointF> lineEnds(const SweepLine& line, bool fromLow)
{
    QVector<QPointF> pts = line.segmentEnds;
    if (!fromLow) {
        std::reverse(pts.begin(), pts.end());
    }
    return pts;
}

QVector<QVector<QPointF>> orderSweep(const QVector<SweepLine>& lines, int corner)
{
    QVector<QVector<QPointF>> out;
    const bool reverseLines = corner >= 2;
    bool fromLow = (corner % 2) == 0;
    const int n = lines.size();
    for (int i = 0; i < n; ++i) {
        const SweepLine& line = lines[reverseLines ? (n - 1 - i) : i];
        out.append(lineEnds(line, fromLow));
        fromLow = !fromLow;
    }
    return out;
}

QVector<QPointF> sweepCorners(const QVector<SweepLine>& lines)
{
    QVector<QPointF> corners;
    if (lines.isEmpty()) {
        return corners;
    }
    const SweepLine& first = lines.first();
    const SweepLine& last = lines.last();
    corners.append(first.segmentEnds.first());
    corners.append(first.segmentEnds.last());
    corners.append(last.segmentEnds.first());
    corners.append(last.segmentEnds.last());
    return corners;
}

void appendPass(GeneratedPath& out, const LocalPlane& plane, const QVector<QVector<QPointF>>& pass)
{
    for (const auto& line : pass) {
        QVector<GeoPoint> geoLine;
        geoLine.reserve(line.size());
        for (const auto& p : line) {
            const GeoPoint g = plane.toGeo(p);
            geoLine.append(g);
            out.points.append(g);
        }
        out.lines.append(geoLine);
    }
}

double geoPathLength(const QVector<GeoPoint>& pts)
{
    double len = 0.0;
    for (int i = 1; i < pts.size(); ++i) {
        len += geoDistance(pts[i - 1], pts[i]);
    }
    return len;
}

} // namespace

GeneratedPath generateSurvey(const SurveyItem& item, const std::optional<GeoPoint>& previous)
{
    GeneratedPath out;
    if (item.polygon.size() < 3) {
        out.error = QCoreApplication::translate("MissionPlan", "Polygon needs at least 3 vertices");
        return out;
    }
    for (const auto& p : item.polygon) {
        if (!p.isValid()) {
            out.error = QCoreApplication::translate("MissionPlan", "Invalid vertex");
            return out;
        }
    }
    if (!(item.lineSpacing >= kMinLineSpacing)) {
        out.error = QCoreApplication::translate("MissionPlan", "Line spacing below minimum");
        return out;
    }

    const LocalPlane plane(centroid(item.polygon));
    QVector<QPointF> poly;
    poly.reserve(item.polygon.size());
    for (const auto& p : item.polygon) {
        poly.append(plane.toLocal(p));
    }
    if (polygonSelfIntersects(poly)) {
        out.error = QCoreApplication::translate("MissionPlan", "Polygon is self-intersecting");
        return out;
    }
    if (localExtent(poly) > kMaxShapeExtentMeters) {
        out.error = QCoreApplication::translate("MissionPlan", "Area larger than %1 km is not supported").arg(kMaxShapeExtentMeters / 1000.0, 0, 'f', 0);
        return out;
    }
    out.areaSquareMeters = polygonAreaLocal(poly);
    if (out.areaSquareMeters <= kMinPolygonAreaM2) {
        out.error = QCoreApplication::translate("MissionPlan", "Polygon has no area");
        return out;
    }

    std::optional<QPointF> prevLocal;
    if (previous && previous->isValid()) {
        prevLocal = plane.toLocal(*previous);
    }

    const SweepResult first = sweepPolygon(poly, item.angleDeg, item.lineSpacing, item.turnaround, kMaxSurveyLines);
    if (first.lines.isEmpty()) {
        out.error = QCoreApplication::translate("MissionPlan", "No lines fit the polygon");
        return out;
    }
    out.spacingClamped = first.clamped;
    out.effectiveSpacing = first.effectiveSpacing;

    int corner = item.entryCorner.value_or(-1);
    if (corner < 0 || corner > 3) {
        corner = nearestCorner(sweepCorners(first.lines), prevLocal);
    }
    out.resolvedEntry = corner;

    const QVector<QVector<QPointF>> pass1 = orderSweep(first.lines, corner);
    appendPass(out, plane, pass1);
    out.lineCount = first.lines.size();

    if (item.crosshatch) {
        const double spacing2 = item.crosshatchSpacing.value_or(item.lineSpacing);
        const SweepResult second = sweepPolygon(poly, item.angleDeg + 90.0, spacing2, item.turnaround,
                                                std::max(1, kMaxSurveyLines - out.lineCount));
        if (!second.lines.isEmpty()) {
            const QPointF exitPoint = pass1.last().last();
            const int corner2 = nearestCorner(sweepCorners(second.lines), exitPoint);
            appendPass(out, plane, orderSweep(second.lines, corner2));
            out.lineCount += second.lines.size();
            out.spacingClamped = out.spacingClamped || second.clamped;
        }
    }

    out.lengthMeters = geoPathLength(out.points);
    return out;
}

GeneratedPath generateCorridor(const CorridorItem& item, const std::optional<GeoPoint>& previous)
{
    GeneratedPath out;
    if (item.axis.size() < 2) {
        out.error = QCoreApplication::translate("MissionPlan", "Axis needs at least 2 vertices");
        return out;
    }
    for (const auto& p : item.axis) {
        if (!p.isValid()) {
            out.error = QCoreApplication::translate("MissionPlan", "Invalid vertex");
            return out;
        }
    }
    if (!(item.lineSpacing >= kMinLineSpacing) || !(item.width > 0.0)) {
        out.error = QCoreApplication::translate("MissionPlan", "Invalid width or spacing");
        return out;
    }

    const LocalPlane plane(centroid(item.axis));
    QVector<QPointF> axis;
    axis.reserve(item.axis.size());
    for (const auto& p : item.axis) {
        const QPointF l = plane.toLocal(p);
        if (!axis.isEmpty() && std::hypot(l.x() - axis.last().x(), l.y() - axis.last().y()) < 1e-6) {
            continue;
        }
        axis.append(l);
    }
    if (axis.size() < 2) {
        out.error = QCoreApplication::translate("MissionPlan", "Axis vertices coincide");
        return out;
    }
    if (localExtent(axis) > kMaxShapeExtentMeters) {
        out.error = QCoreApplication::translate("MissionPlan", "Axis longer than %1 km is not supported").arg(kMaxShapeExtentMeters / 1000.0, 0, 'f', 0);
        return out;
    }

    double spacing = item.lineSpacing;
    const double rawCount = std::round(item.width / spacing);
    int count = 1;
    if (rawCount > static_cast<double>(kMaxCorridorLines)) {
        count = kMaxCorridorLines;
        spacing = item.width / count;
        out.spacingClamped = true;
    } else if (rawCount > 1.0) {
        count = static_cast<int>(rawCount);
    }
    out.effectiveSpacing = spacing;
    out.lineCount = count;

    const double miterLimit = 2.0 * std::max(spacing, item.width * 0.5);
    QVector<int> pinched;
    QVector<QVector<QPointF>> lines;
    lines.reserve(count);
    for (int k = 0; k < count; ++k) {
        const double d = (k - (count - 1) * 0.5) * spacing;
        QVector<QPointF> line = offsetPolyline(axis, d, item.width * 0.5, miterLimit, &pinched);
        if (line.size() < 2) {
            continue;
        }
        if (item.turnaround != 0.0) {
            const QPointF d0 = unit(line[0] - line[1]);
            const QPointF d1 = unit(line[line.size() - 1] - line[line.size() - 2]);
            line[0] = line[0] + d0 * item.turnaround;
            line[line.size() - 1] = line[line.size() - 1] + d1 * item.turnaround;
        }
        lines.append(line);
    }
    if (lines.isEmpty()) {
        out.error = QCoreApplication::translate("MissionPlan", "No lines generated");
        return out;
    }
    const QVector<QPointF> bandLeft = offsetPolyline(axis, -item.width * 0.5, item.width * 0.5, miterLimit);
    const QVector<QPointF> bandRight = offsetPolyline(axis, item.width * 0.5, item.width * 0.5, miterLimit);
    out.band.reserve(bandLeft.size() + bandRight.size());
    for (const auto& p : bandLeft) {
        out.band.append(plane.toGeo(p));
    }
    for (int i = bandRight.size() - 1; i >= 0; --i) {
        out.band.append(plane.toGeo(bandRight[i]));
    }
    if (!pinched.isEmpty()) {
        std::sort(pinched.begin(), pinched.end());
        QStringList spans;
        for (int s : pinched) {
            spans.append(QStringLiteral("%1–%2").arg(s + 1).arg(s + 2));
        }
        out.warnings.append(QCoreApplication::translate("MissionPlan", "Width %1 m is too large for the bends at vertices %2, lines trimmed there").arg(item.width, 0, 'f', 0).arg(spans.join(QStringLiteral(", "))));
    }

    int entry = item.entryEnd.value_or(-1);
    if (entry < 0 || entry > 3) {
        std::optional<QPointF> prevLocal;
        if (previous && previous->isValid()) {
            prevLocal = plane.toLocal(*previous);
        }
        const QVector<QPointF> corners = {lines.first().first(), lines.first().last(), lines.last().first(), lines.last().last()};
        entry = nearestCorner(corners, prevLocal);
    }
    out.resolvedEntry = entry;

    const bool fromLastLine = entry >= 2;
    bool fromStart = (entry % 2) == 0;
    QVector<QVector<QPointF>> pass;
    pass.reserve(lines.size());
    for (int i = 0; i < lines.size(); ++i) {
        QVector<QPointF> line = lines[fromLastLine ? (lines.size() - 1 - i) : i];
        if (!fromStart) {
            std::reverse(line.begin(), line.end());
        }
        pass.append(line);
        fromStart = !fromStart;
    }

    appendPass(out, plane, pass);
    out.lengthMeters = geoPathLength(out.points);
    return out;
}

} // namespace mission
