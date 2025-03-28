#ifndef GRIDGENERATOR_H
#define GRIDGENERATOR_H

#include "triangle.h"
#include "quad.h"

#include <queue>
#include <memory>

template <typename T>
class GridGenerator
{
public:

    GridGenerator();

    static std::shared_ptr <std::vector <Triangle <T>>> generateTriangleGrid(Point3D <T> topLeft, float width, float height, T cellSideSize)
    {
        auto result = std::make_shared <std::vector <Triangle <T>>>();

        T minX = topLeft.x();
        T minY = topLeft.y() + std::max(height,width);
        T maxX = topLeft.x() + std::max(height,width);
        T maxY = topLeft.y();

        const T dx = maxX - minX;
        const T dy = maxY - minY;
        const T maxDelta = std::max(dx, dy);
        const T midX = (minX + maxX) / 2.0;
        const T midY = (minY + maxY) / 2.0;
        const T outerStep = 2.0;
        const T z = 0.0;

        const Point3D <T> p1(midX - outerStep * maxDelta, midY - maxDelta, z);
        const Point3D <T> p2(midX, midY + outerStep * maxDelta, z);
        const Point3D <T> p3(midX + outerStep * maxDelta, midY - maxDelta, z);

        auto queue = std::make_shared <std::queue <Triangle <T>>>();

        Triangle <T> superTriangle(p1,p2,p3);

        queue->push(superTriangle);

        try {
            while (!queue->empty()){
                auto t = queue->front();

                if (t.AB().length() > cellSideSize ||
                    t.AC().length() > cellSideSize ||
                    t.BC().length() > cellSideSize){

                    auto splitted = t.bisectionSplit();

                    for (const auto& _t : splitted){
                        queue->push(_t);
                    }

                }else{
                    result->push_back(t);

                }

                queue->pop();
            }

        } catch (std::bad_alloc& ex) {
        }



        return result;
    }

    static std::shared_ptr <std::vector <Quad <T>>> generateQuadGrid(Point3D <T> topLeft, float width, float height, T cellSideSize)
    {
        auto result = std::make_shared <std::vector <Quad <T>>>();

        int horzCellsCount = std::ceil(width / cellSideSize)/* + 1*/;
        int vertCellsCount = std::ceil(height / cellSideSize)/*+ 1*/;

        for (int row = 0; row < horzCellsCount; row++){
            for (int col = 0; col < vertCellsCount; col++){
                Point3D <T> A(topLeft.x() + col * cellSideSize,
                              topLeft.y() + row * cellSideSize,
                              topLeft.z());

                Point3D <T> B(topLeft.x() + (col+1) * cellSideSize,
                              topLeft.y() + row * cellSideSize,
                              topLeft.z());

                Point3D <T> C(topLeft.x() + (col+1) * cellSideSize,
                              topLeft.y() + (row+1) * cellSideSize,
                              topLeft.z());

                Point3D <T> D(topLeft.x() + (col) * cellSideSize,
                              topLeft.y() + (row+1) * cellSideSize,
                              topLeft.z());

                Quad <T> quad(A,B,C,D);

                result->push_back(quad);
            }
        }

        return result;
    }

private:

    static Triangle <T> makeSuperTriangle(const std::vector <Point3D<T>>& points) {

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

        const Point3D <T> p1(midX - outerStep * maxDelta, midY - maxDelta, z);
        const Point3D <T> p2(midX, midY + outerStep * maxDelta, z);
        const Point3D <T> p3(midX + outerStep * maxDelta, midY - maxDelta, z);

        return { p1,p2,p3 };
    }

};

#endif // GRIDGENERATOR_H
