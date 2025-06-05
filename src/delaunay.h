#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <QDebug>


namespace delaunay {

constexpr int RESERVE_BAD = 16;
constexpr double SUPER_SIZE = 1e6;
constexpr double COLLINEAR_EPS = 1e-12;
constexpr double ZERO_LEVEL = 0.0;

/// Simple 2D point
struct Point {
    Point() : x(0.0), y(0.0), z(0.0) {};
    Point(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {};
    double x, y, z;

    bool operator==(const Point &other) const {
        return (qFuzzyCompare(x, other.x) && qFuzzyCompare(y, other.y));
    }

    bool operator!=(const Point &other) const {
        return !(*this == other);
    }
};

/// Edge between two point indices (undirected)
struct Edge {
    size_t i1, i2;
    bool operator==(const Edge &other) const {
        return (i1 == other.i1 && i2 == other.i2) ||
               (i1 == other.i2 && i2 == other.i1);
    }
};

struct Triangle {
    size_t a, b, c;          // indices of vertices in points[]
    Point circumcenter;       // Center of circumcircle
    double circumradius2;     // Squared radius of circumcircle

    /// Construct and compute circumcircle from indexed points
    Triangle(size_t ia, size_t ib, size_t ic, const std::vector<Point> &points)
        : a(ia), b(ib), c(ic) {
        computeCircumcircle(points);
    }

    bool operator==(const Triangle& other) const {
        return (a == other.a) && (b == other.b) && (c == other.b);
    }

    bool operator!=(const Triangle &other) const {
        return !(*this == other);
    }

    /// Compute a robust circumcircle; fallback on collinear triangles
    void computeCircumcircle(const std::vector<Point> &pts) {
        const Point &A = pts[a];
        const Point &B = pts[b];
        const Point &C = pts[c];
        // Compute twice signed area of triangle ABC
        double area2 = (B.x - A.x)*(C.y - A.y) - (B.y - A.y)*(C.x - A.x);
        if (std::fabs(area2) < COLLINEAR_EPS) {
            // Collinear fallback: choose the longest edge
            auto sqdist = [&](const Point &P, const Point &Q) {
                double dx = P.x - Q.x;
                double dy = P.y - Q.y;
                return dx*dx + dy*dy;
            };
            double dAB = sqdist(A,B);
            double dBC = sqdist(B,C);
            double dCA = sqdist(C,A);
            if (dAB >= dBC && dAB >= dCA) {
                circumcenter = {(A.x+B.x)/2.0, (A.y+B.y)/2.0, ZERO_LEVEL};
                circumradius2 = dAB/4.0;
            } else if (dBC >= dCA) {
                circumcenter = {(B.x+C.x)/2.0, (B.y+C.y)/2.0, ZERO_LEVEL};
                circumradius2 = dBC/4.0;
            } else {
                circumcenter = {(C.x+A.x)/2.0, (C.y+A.y)/2.0, ZERO_LEVEL};
                circumradius2 = dCA/4.0;
            }
            return;
        }
        // Standard circumcircle via perpendicular bisector intersection
        double A1 = B.x - A.x;
        double B1 = B.y - A.y;
        double C1 = C.x - A.x;
        double D1 = C.y - A.y;
        double E  = A1 * (A.x + B.x) + B1 * (A.y + B.y);
        double F  = C1 * (A.x + C.x) + D1 * (A.y + C.y);
        double G  = 2.0 * (A1 * (C.y - B.y) - B1 * (C.x - B.x));
        // G should not be near zero here
        circumcenter.x = (D1 * E - B1 * F) / G;
        circumcenter.y = (A1 * F - C1 * E) / G;
        double dx = circumcenter.x - A.x;
        double dy = circumcenter.y - A.y;
        circumradius2 = dx*dx + dy*dy;
    }

    /// Check if a point lies inside the circumcircle
    bool containsInCircumcircle(const Point &p) const {
        double dx = p.x - circumcenter.x;
        double dy = p.y - circumcenter.y;
        return (dx * dx + dy * dy) <= circumradius2;
    }
};

inline uint qHash(const Triangle& t, uint seed = 0) noexcept
{
    std::array<size_t,3> v{t.a, t.b, t.c};
    std::sort(v.begin(), v.end());

    seed ^= ::qHash(quint64(v[0])) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    seed ^= ::qHash(quint64(v[1])) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    seed ^= ::qHash(quint64(v[2])) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    return seed;
}

/// Brute‐force incremental Delaunay triangulation (Bowyer–Watson)
class Delaunay {
public:
    Delaunay() {
        // Create a "super‑triangle" rectangle enclosing all points
        points.push_back({-SUPER_SIZE, -SUPER_SIZE, ZERO_LEVEL}); // index 0
        points.push_back({ SUPER_SIZE, -SUPER_SIZE, ZERO_LEVEL}); // index 1
        points.push_back({ SUPER_SIZE,  SUPER_SIZE, ZERO_LEVEL}); // index 2
        points.push_back({-SUPER_SIZE,  SUPER_SIZE, ZERO_LEVEL}); // index 3
        triangles.emplace_back(0, 1, 2, points);
        triangles.emplace_back(0, 2, 3, points);
    }

    /// Add a new point and update triangulation
    void addPoint(const Point &p) {
        //qDebug() << "Delaunay::addPoint";

        updated.clear();

        // 1) Insert point and get its index
        points.push_back(p);
        size_t pi = points.size() - 1;

        // 2) Find all bad triangles by index
        std::vector<size_t> badIdx;
        badIdx.reserve(RESERVE_BAD);
        for (size_t i = 0; i < triangles.size(); ++i) {
            if (triangles[i].containsInCircumcircle(p)) {
                badIdx.push_back(i);
            }
        }

        // 3) Build boundary of hole
        std::vector<Edge> boundary;
        boundary.reserve(RESERVE_BAD * 3);
        for (size_t idx : badIdx) {
            const Triangle &t = triangles[idx];
            addEdge(boundary, {t.a, t.b});
            addEdge(boundary, {t.b, t.c});
            addEdge(boundary, {t.c, t.a});
        }

        // 4) Re‐triangulate: reuse slots, then append
        size_t K = badIdx.size();
        size_t slot = 0;
        for (const auto &e : boundary) {
            if (slot < K) {
                // overwrite bad triangle slot
                triangles[ badIdx[slot] ] = Triangle(e.i1, e.i2, pi, points);

                //qDebug() << "   updated"
                //         << points[e.i1].x << points[e.i1].y << "\t\t"
                //         << points[e.i2].x << points[e.i2].y << "\t\t"
                //         << points[pi].x   << points[pi].y;

                updated.emplace_back(e.i1, e.i2, pi, points);
            }
            else {
                // append new triangle
                triangles.emplace_back(e.i1, e.i2, pi, points);

                //qDebug() << "   appending"
                //         << points[e.i1].x << points[e.i1].y << "\t\t"
                //         << points[e.i2].x << points[e.i2].y << "\t\t"
                //         << points[pi].x   << points[pi].y;

                updated.emplace_back(e.i1, e.i2, pi, points);
            }

            ++slot;
        }
        // no need for free-list: vector only grows when boundary > badIdx
    }

    /// Access current triangle list
    const std::vector<Triangle>& getTriangles() const {
        return triangles;
    }
    /// Access point list
    const std::vector<Point>& getPoints() const {
        return points;
    }

    const std::vector<Triangle>& getUpdated() const {
        return updated;
    }

private:
    std::vector<Point>    points;
    std::vector<Triangle> triangles;
    std::vector<Triangle> updated;

    /// Utility to add or remove edges on boundary
    void addEdge(std::vector<Edge> &edges, const Edge &e) {
        auto it = std::find(edges.begin(), edges.end(), e);
        if (it == edges.end()) {
            edges.push_back(e);
        } else {
            edges.erase(it);
        }
    }
};

} // namespace delaunay

