#pragma once

#include <iostream>
#include <vector>
#include <cmath>
#include <math.h>
#include <limits>
#include <type_traits>
#include <functional>
#include <memory>

#include "Triangle.h"
#include "Point3D.h"
#include "Edge.h"
#include "Equals.h"

#include <thread>

template <typename T>
class Delaunay
{
    using TrianglesPointer = std::shared_ptr <std::vector <Triangle <T>>>;

public:

    Delaunay()
    {
        mpTriangles = std::make_shared <std::vector <Triangle <T>>>();
    }

    //! Return - pointer to triangles vector
    TrianglesPointer triangles() const {return mpTriangles;};

    //! Triangulate input data
    TrianglesPointer trinagulate(const std::vector <Point3D <T>>& points)
    {
        if (points.empty())
            return mpTriangles;

        mpTriangles->clear();

        auto super = makeSuperTriangle(points);

        mpTriangles->push_back(super);

        for (auto& point : points) {

            std::vector <Edge <T>> polygon;

            for (auto& triangle : *mpTriangles) {
                if (triangle.circle().contains(point)) {
                    triangle.setWrong(true);

                    polygon.push_back(triangle.AB());
                    polygon.push_back(triangle.AC());
                    polygon.push_back(triangle.BC());
                }
            }

            for (size_t i = 0; i < polygon.size(); i++) {
                for (size_t j = 0; j < polygon.size(); j++) {
                    if (i != j && equal(polygon[i], polygon[j])) {
                        polygon[i].setWrong(true);
                        break;
                    }
                }
            }

            for (auto it = mpTriangles->begin(); it != mpTriangles->end();)
            {
                if (it->isWrong())
                    it = mpTriangles->erase(it);
                else ++it;
            }

            for (auto it = polygon.begin(); it != polygon.end();)
            {
                if (it->isWrong())
                    it = polygon.erase(it);
                else ++it;
            }

            for (const auto& edge : polygon) {
                Triangle <T> triangle(edge.p1(), edge.p2(), point);
                mpTriangles->push_back(triangle);
            }
        }

        for (auto it = mpTriangles->begin(); it != mpTriangles->end();)
        {
            if (it->contains(super.A()) ||
                it->contains(super.B()) ||
                it->contains(super.C()))
                it = mpTriangles->erase(it);
            else ++it;
        }

        return mpTriangles;
    }

private:


    Triangle <T> makeSuperTriangle(const std::vector <Point3D<T>>& points) {

        T minX = points.front().x();
        T minY = points.front().y();
        T maxX = minX;
        T maxY = minY;

        for (auto& point : points) {
            minX = std::min(minX, point.x());
            maxX = std::max(maxX, point.x());
            minY = std::min(minY, point.y());
            maxY = std::max(maxY, point.y());
        }

        const T dx = maxX - minX;
        const T dy = maxY - minY;
        const T maxDelta = std::max(dx, dy);
        const T midX = (minX + maxX) / 2.0;
        const T midY = (minY + maxY) / 2.0;
        const T outerStep = 20.0;
        const T z = 0.0;

        const Point3D <double> p1(midX - outerStep * maxDelta, midY - maxDelta, z);
        const Point3D <double> p2(midX, midY + outerStep * maxDelta, z);
        const Point3D <double> p3(midX + outerStep * maxDelta, midY - maxDelta, z);

        return { p1,p2,p3 };
    }

    TrianglesPointer mpTriangles;
};

