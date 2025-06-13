#pragma once

#include <algorithm>
#include <cmath>
#include <set>
#include <stack>
#include <vector>
#include <unordered_map>
#include <QDebug>


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

/// Brute‐force incremental Delaunay triangulation (Bowyer–Watson)
class Delaunay {
public:
    Delaunay(double super_size = SUPER_SIZE) {
        // Create a "super‑triangle" rectangle enclosing all points
        size_t p0 = insertPoint({-super_size, -super_size, ZERO_LEVEL});
        size_t p1 = insertPoint({ super_size, -super_size, ZERO_LEVEL});
        size_t p2 = insertPoint({ super_size,  super_size, ZERO_LEVEL});
        size_t p3 = insertPoint({-super_size,  super_size, ZERO_LEVEL});

        // triangles.emplace_back(p0, p1, p2, points);
        // triangles.emplace_back(p0, p2, p3, points);

        insertTriangle(p0, p1, p2);
        insertTriangle(p0, p2, p3);
    }

    /// Add a new point and update triangulation
    TriResult addPoint(const Point &p) {
        // 1) Insert point and get its index
        size_t pi = insertPoint(p);

        // 2) Find all bad triangles by index
        std::vector<size_t> removedTriIdx = findBadTriangles(p);

        // 3) Build boundary of hole
        std::vector<Edge> boundary = buildBoundary(removedTriIdx);

        // 4) Re‐triangulate
        std::vector<size_t> newTriIdx = triangulateHoleInPlace(boundary, pi);

        return {pi, removedTriIdx, newTriIdx};
    }

    uint64_t removePoint(size_t p_idx) {
        if(!markPointAsBad(p_idx)) {
            qDebug() << "invalid removing idx";
            return -1;
        }

        // 2) Find all bad triangles by index
        std::vector<size_t> badIdx = findBadTrianglesByVertice(p_idx);

        // 3) Build boundary of hole
        std::vector<Edge> boundary = buildBoundary(badIdx);

        std::vector<Triangle> new_triangles = triangulateHole(boundary);

        for (Triangle &t : new_triangles) {
            insertTriangle(t.a, t.b, t.c);
        }
        int t_dif = int(badIdx.size()) - int(new_triangles.size());

        if(t_dif != 2) {
            qDebug() << "Triangles dif: " << t_dif;
        }

        return p_idx;
    }

    /// Access current triangle list
    const std::vector<Triangle>& getTriangles() const {
        return triangles;
    }
    /// Access point list
    const std::vector<Point>& getPoints() const {
        return points;
    }

    std::vector<Point>& getPointsRef(){
        return points;
    }

private:
    std::vector<Point>    points;
    std::vector<Triangle> triangles;
    std::stack<size_t> freePointSlots;
    std::stack<size_t> freeTriangleSlots;

    std::vector<size_t> findBadTriangles(Point const &p) {
        std::vector<size_t> badIdx;
        badIdx.reserve(RESERVE_BAD);
        for (size_t i = 0; i < triangles.size(); ++i) {
            if (triangles[i].is_bad) continue;
            if (triangles[i].containsInCircumcircle(p)) {
                badIdx.push_back(i);
                markTriangleAsBad(i);
            }
        }
        return badIdx;
    }

    std::vector<size_t> findBadTrianglesByVertice(size_t p_idx) {
        std::vector<size_t> badIdx;
        badIdx.reserve(RESERVE_BAD);
        for (size_t i = 0; i < triangles.size(); ++i) {
            if (triangles[i].is_bad) continue;
            if (triangles[i].containsVertex(p_idx)) {
                badIdx.push_back(i);
                markTriangleAsBad(i);
            }
        }
        return badIdx;
    }

    std::vector<Edge> buildBoundary(std::vector<size_t> t_idx) {
        std::vector<Edge> boundary;
        boundary.reserve(RESERVE_BAD * 3);
        for (size_t idx : t_idx) {
            const Triangle &t = triangles[idx];
            addEdge(boundary, {t.a, t.b});
            addEdge(boundary, {t.b, t.c});
            addEdge(boundary, {t.a, t.c});
        }
        return boundary;
    }

    std::vector<size_t> triangulateHoleInPlace(std::vector<Edge> boundary, size_t p_idx) {
        std::vector<size_t> t_new_idx;
        for (const auto &e : boundary) {
            size_t t_new = insertTriangle(e.i1, e.i2, p_idx);
            t_new_idx.push_back(t_new);
        }
        return t_new_idx;
    }

    std::vector<Triangle> triangulateHole(std::vector<Edge> boundary) {
        std::vector<Triangle> newTriangles;
        if (boundary.empty()) {
            return newTriangles;
        }

        // 1) Collect unique boundary vertex indices
        std::set<size_t> uniqueVerts;
        for (const Edge &e : boundary) {
            uniqueVerts.insert(e.i1);
            uniqueVerts.insert(e.i2);
        }


        std::vector<size_t> verts(uniqueVerts.begin(), uniqueVerts.end());
        size_t n = verts.size();
        if (n < 3) {
            return newTriangles; // Not enough vertices to form a triangle
        }

        Delaunay del(SUPER_SIZE*2);
        std::unordered_map<size_t, size_t> p_p;
        for(size_t p_inx : verts) {
            Point p = points[p_inx];
            TriResult res = del.addPoint(p);
            size_t p_n_idx = res.pointIdx;
            p_p[p_n_idx] = p_inx;
        }

        std::vector<Triangle> row_new_t = del.getTriangles();

        std::vector<Triangle> filtered_t;
        for(Triangle &t : row_new_t) {
            if(t.a > 3 && t.b > 3 && t.c > 3 && !t.is_bad) {
                filtered_t.emplace_back(p_p[t.a], p_p[t.b], p_p[t.c], points);
            }
        }

        for(Triangle &t : filtered_t) {
            std::vector<Edge> edges = t.edges();
            int find_neighbors = 0;

            // for(Triangle &t_test : filtered_t) {
            //     bool is_same = (t == t_test);
            //     if(!is_same && t.isNeighbor(t_test)) {
            //         find_neighbors++;
            //     }
            // }

            for(Edge &e : edges) {
                bool is_find = false;
                for(Edge &b : boundary) {
                    if(e == b) {
                        is_find = true;
                        break;
                    }
                }
                if(!is_find) {
                    for(Triangle &t_test : filtered_t) {
                        if(t != t_test) {
                            std::vector<Edge> edges_test = t_test.edges();
                            for(Edge &e_test : edges_test) {
                                if(e == e_test) {
                                    is_find = true;
                                    break;
                                }
                            }
                        }

                        if(is_find) {
                            break;
                        }
                    }
                }

                if(is_find) {
                    find_neighbors++;
                }
            }

            if(find_neighbors == 3) {
                newTriangles.emplace_back(t);
            }
        }

        int t_dif = int(boundary.size()) - int(newTriangles.size());

        if(t_dif != 2) {
            qDebug() << "Re_Triangles dif: " << t_dif;
        }

        return newTriangles;
    }

    std::size_t insertPoint(Point const &p) {
        std::size_t idx;
        if (!freePointSlots.empty()) {
            idx = freePointSlots.top();
            freePointSlots.pop();
            points[idx] = p;
        } else {
            idx = points.size();
            points.push_back(p);
        }

        // std::size_t idx = points.size();
        // points.push_back(p);
        return idx;
    }

    bool markPointAsBad(std::size_t idx) {
        if(idx < points.size() && (!points[idx].is_bad)) {
            points[idx].is_bad = true;
            freePointSlots.push(idx);
            return true;
        } else {
            return false;
        }
    }

    // Create a triangle by indices (reuse slot if available)
    size_t insertTriangle(size_t i0, size_t i1, size_t i2) {
        size_t tid;
        Triangle tri(i0, i1, i2, points);
        tri.is_bad = false;
        if (!freeTriangleSlots.empty()) {
            tid = freeTriangleSlots.top();
            freeTriangleSlots.pop();
            if(!triangles[tid].is_bad) {
                qDebug() << "bad reuse";
            }
            triangles[tid] = tri;
        } else {
            tid = triangles.size();
            triangles.push_back(tri);
        }
        return tid;
    }

    void markTriangleAsBad(std::size_t idx) {
        triangles[idx].is_bad = true;
        freeTriangleSlots.push(idx);
    }

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
