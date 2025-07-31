#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <QtGlobal>


namespace delaunay {

constexpr int RESERVE_BAD = 16;
constexpr double SUPER_SIZE = 1e4;
constexpr double COLLINEAR_EPS = 1e-12;
constexpr double ZERO_LEVEL = 0.0;

/// Simple 2D point
struct Point {
    Point() : x(0.0), y(0.0), z(0.0) {};
    Point(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {};
    double x, y, z;
    bool is_bad = false;

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
    Edge(std::size_t _a = 0, std::size_t _b = 0) {
        if (_a < _b) { i1 = _a; i2 = _b; }
        else { i1 = _b; i2 = _a; }
    }

    bool operator==(const Edge &other) const {
        return (i1 == other.i1 && i2 == other.i2);
    }
};

struct TriResult {
    size_t pointIdx = 0;
    std::vector<size_t> removedTriIdx;
    std::vector<size_t> newTriIdx;
};

struct Triangle {
    size_t a, b, c;          // indices of vertices in points[]
    Point circumcenter;       // Center of circumcircle
    double circumradius2;     // Squared radius of circumcircle
    bool is_bad = false;
    double longest_edge_dist;

    /// Construct and compute circumcircle from indexed points
    Triangle(size_t ia, size_t ib, size_t ic, const std::vector<Point> &points) {
        std::array<size_t,3> v = {ia, ib, ic};
        std::sort(v.begin(), v.end());
        a = v[0]; b = v[1]; c = v[2];
        is_bad = false;
        computeCircumcircle(points);
    }

    bool operator==(const Triangle& other) const {
        return (a == other.a) && (b == other.b) && (c == other.c);
    }

    bool operator!=(const Triangle &other) const {
        return !(*this == other);
    }

    bool isNeighbor(Triangle &t) {
        return (a == t.a && b == t.b) || (b == t.b && c == t.c) || (a == t.a && c == t.c);
    }

    std::vector<Edge> edges() {
        return std::vector<Edge>() = {{a, b}, {b, c}, {a, c}};
    }

    /// Compute a robust circumcircle; fallback on collinear triangles
    void computeCircumcircle(const std::vector<Point> &pts) {
        const Point &A = pts[a];
        const Point &B = pts[b];
        const Point &C = pts[c];

        auto sqdist = [&](const Point &P, const Point &Q) {
            double dx = P.x - Q.x;
            double dy = P.y - Q.y;
            return dx*dx + dy*dy;
        };
        double dAB = sqdist(A,B);
        double dBC = sqdist(B,C);
        double dCA = sqdist(C,A);

        if (dAB >= dBC && dAB >= dCA) {
            longest_edge_dist = sqrt(dAB);
        } else if (dBC >= dCA) {
            longest_edge_dist = sqrt(dBC);
        } else {
            longest_edge_dist = sqrt(dCA);
        }

        // Compute twice signed area of triangle ABC
        double area2 = (B.x - A.x)*(C.y - A.y) - (B.y - A.y)*(C.x - A.x);
        if (std::fabs(area2) < COLLINEAR_EPS) {
            // Collinear fallback: choose the longest edge

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
            is_bad = true;
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

    bool containsVertex(size_t idx) {
        return a == idx || b == idx || c == idx;
    }
};

} // namespace delaunay
