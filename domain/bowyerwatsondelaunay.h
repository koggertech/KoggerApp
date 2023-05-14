#ifndef BOWYERWATSONDELAUNAY_H
#define BOWYERWATSONDELAUNAY_H

#include <abstractdelaunayprocessor.h>
#include <Triangle.h>
#include <point2d.h>

namespace DelaunayTin
{
    template <typename T>
    class BowyerWatson : public AbstractDelaunayProcessor <T>
    {
    public:

        std::vector <Triangle <T>> build(std::vector <Point3D <T>> points, T edgeLengthLimit = -1.0f) override
        {
            std::vector <Triangle <T>> result;

            if (points.empty())
                return result;

            result.clear();

            auto super = makeSuperTriangle(points);

            result.push_back(super);

            for (auto& point : points) {

                std::vector <Edge <T>> polygon;

                for (auto& triangle : result) {
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

                for (auto it = result.begin(); it != result.end();)
                {
                    if (it->isWrong())
                        it = result.erase(it);
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

                    result.push_back(triangle);
                }
            }

            for (auto it = result.begin(); it != result.end();)
            {
                if (it->contains(super.A()) ||
                    it->contains(super.B()) ||
                    it->contains(super.C()))
                {
                    it = result.erase(it);
                }

                if (it->AB().length() > edgeLengthLimit ||
                    it->BC().length() > edgeLengthLimit ||
                    it->AC().length() > edgeLengthLimit){

                    it = result.erase(it);
                }

                else ++it;

            }

            return result;
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
        const T outerStep = 200.0;
        const T z = 0.0;

        const Point3D <T> p1(midX - outerStep * maxDelta, midY - maxDelta, z);
        const Point3D <T> p2(midX, midY + outerStep * maxDelta, z);
        const Point3D <T> p3(midX + outerStep * maxDelta, midY - maxDelta, z);

        return { p1,p2,p3 };
    }
    };
}

#endif // BOWYERWATSONDELAUNAY_H
