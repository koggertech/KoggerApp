#pragma once

#include <vector>
#include <cmath>
#include <math.h>
#include <limits>
#include <type_traits>
#include <functional>
#include <memory>

#include "triangle.h"
#include "point_3d.h"

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
    //! Return - maximum edge length
    T maxEdgeLength() const {return mMaxEdgeLength;};
    //! Return - minimum edge length
    T minEdgeLength() const {return mMinEdgeLength;};

    //! Triangulate input data
    TrianglesPointer trinagulate(const std::vector <Point3D <T>>& points, T edgeLengthLimit = -1.0f)
    {
        if (points.empty())
            return mpTriangles;

        mpTriangles->clear();

        auto super = makeSuperTriangle(points);

        mpTriangles->push_back(super);

        for (auto& point : points) {

            std::vector <Edge <T>> polygon;

            for (auto& triangle : *mpTriangles) {
                if (triangle.circle().contains(point.toPoint2D())) {
                    triangle.setWrong(true);

                    polygon.push_back(triangle.AB());
                    polygon.push_back(triangle.AC());
                    polygon.push_back(triangle.BC());
                }
            }

            for (size_t i = 0; i < polygon.size(); i++) {
                for (size_t j = 0; j < polygon.size(); j++) {
                    if (i != j && polygon[i] == polygon[j]) {
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
                Triangle <T> triangle(point, edge.p1(), edge.p2());

                mpTriangles->push_back(triangle);
            }
        }

        for (auto it = mpTriangles->begin(); it != mpTriangles->end();)
        {   bool comp = it->contains(super.A()) || it->contains(super.B()) || it->contains(super.C());

            if(comp)
                it = mpTriangles->erase(it);
            else
                it++;
        }

        if(edgeLengthLimit != -1){
            for (auto it = mpTriangles->begin(); it != mpTriangles->end();)
            {
                bool comp_2 = it->AB().length() > edgeLengthLimit || it->BC().length() > edgeLengthLimit || it->AC().length() > edgeLengthLimit;

                if (comp_2){
                    it = mpTriangles->erase(it);
                }else{
                    ++it;
                }
            }
        }

        return mpTriangles;
    }

    TrianglesPointer trinagulate_2(const std::vector <Point3D <T>>& points, T edgeLengthLimit = -1.0f)
    {

        Q_UNUSED(edgeLengthLimit)

        mpTriangles->clear();

        if(points.size() < 3){
            return nullptr;
        }

        // Сортируем точки вдоль оси x

        std::sort(points.begin(), points.end(),
                  [](const Point3D <T>& p1, const Point3D <T>& p2){
            return p1.x() < p2.x();
        });

        // Строим стартовый треугольник по первым трем точкам
        Triangle <T> baseTriangle{points.at(0),points.at(1),points.at(2)};

        return nullptr;
    }

private:

    bool isCollinear(Point3D <T>& p1, Point3D <T>& p2, Point3D <T>& p3)
    {
        auto s = 0.5 * abs((p2.x() - p1.x()) * (p3.y() - p1.y()) - (p3.x() - p1.x()) * (p2.y() - p1.y()));
        return s == 0;
    }

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
        const T outerStep = 200.0;
        const T z = 0.0;

        const Point3D <T> p1(midX - outerStep * maxDelta, midY - maxDelta, z);
        const Point3D <T> p2(midX, midY + outerStep * maxDelta, z);
        const Point3D <T> p3(midX + outerStep * maxDelta, midY - maxDelta, z);

        return { p1,p2,p3 };
    }

    TrianglesPointer mpTriangles;

    T mMaxEdgeLength = static_cast <T> (0);
    T mMinEdgeLength = static_cast <T> (0);
};

