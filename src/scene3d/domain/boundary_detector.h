#ifndef BOUNDARYDETECTOR_H
#define BOUNDARYDETECTOR_H

#include <algorithm>
#include <memory>
#include <vector>

#include <unordered_map>

#include "triangle.h"
#include "quad.h"

template <typename T>
class BoundaryDetector
{
public:

    std::vector <Edge <T>> detect(std::vector <Triangle <T>>& triangles)
    {
        std::vector <Edge <T>> prepared;

        for (const auto& triangle : triangles){
            auto edges = triangle.edges();
            for (const auto& edge : edges){
                prepared.push_back(edge);
            }
        }

        detectBoundary(prepared);

        return mBoundary;
    }

    std::vector <Edge <T>> detect(std::vector <Quad <T>>& quads)
    {
        std::vector <Edge <T>> prepared;

        for (const auto& quad : quads){
            auto edges = quad.edges();
            for (const auto& edge : edges){
                prepared.push_back(edge);
            }
        }

        detectBoundary(prepared);

        return mBoundary;
    }

    static std::vector <Edge <T>> simpleTinBoundary(std::shared_ptr <std::vector <Triangle <T>>> triangles)
    {
        std::vector <std::pair <Edge<T>,int>> edges;
        std::vector <Edge <T>> result;

        for (const auto& t : *triangles){
            auto edge_1 = std::pair <Edge<double>,int>(t.AB(),0);
            auto edge_2 = std::pair <Edge<double>,int>(t.BC(),0);
            auto edge_3 = std::pair <Edge<double>,int>(t.AC(),0);
            edges.emplace_back(edge_1);
            edges.emplace_back(edge_2);
            edges.emplace_back(edge_3);
        }

        for (auto& edge_1 : edges){
            for (const auto& triangle : *triangles){
                if (triangle.sharedWithEdge(edge_1.first)){
                    edge_1.second++;
                }
            }
        }

        auto condition = [](const std::pair <Edge<T>,int>& edge){
            return edge.second != 1;
        };

        //edges.erase(std::remove_if(edges.begin(), edges.end(),condition), edges.end());

        for (const auto& edge : edges){
            if(edge.second == 1)
                result.push_back(edge.first);
        }

        return result;
    }

    static std::vector <Point3D <T>>  uniformGridBoundary(std::vector <Point3D <T>> points)
    {
        std::vector <Point3D <T>> result;

        if (points.size() < 3)
            return result;

        auto comparator = [](const Point3D <T>& p1, const Point3D <T>& p2){
            if (p1.x() < p2.x()) return true;
            else if (p1.x() == p2.x()){
                return p1.y() < p2.y();
            }else return false;
        };

        std::sort(points.begin(),points.end(), comparator);

        std::vector <std::vector <Point3D <T>>> hull;

        std::vector <Point3D <T>> column{points.front()};
        for (int i = 1; i < points.size(); i++){
            if (column.back().x() != points[i].x()){
                hull.push_back(column);
                column.clear();
            }
            column.push_back(points[i]);

            if (i == points.size()-1){
                hull.push_back(column);
            }
        }

        for (int i = 0; i < hull.size(); i++){
            //if (hull[i].first().y() != result.last().y() && ){
            //    result.append(hull[i].first());
            //}
            result.push_back(hull[i].front());
        }


        for (const auto& p : hull.back()){
            //if (p.x() != result.last().x())
                result.push_back(p);
        }

        for (int i = hull.size()-1; i > 0; i--){
            //if (hull[i].front().y() != result.back().y()){
                result.push_back(hull[i].back());
            //}
        }

        for (int i = hull.front().size()-1; i > 0; i--){
            result.push_back(hull.front()[i]);
        }

        return result;
    }

private:

    void detectBoundary(std::vector <Edge <T>>& edges)
    {
        mBoundary.clear();

        std::unordered_map <Edge <T>, uint64_t> map;

        for (const auto& edge : edges){
            auto it = map.find(edge);
            it != map.end() ? it->second++ : map[edge] = 1;
        }

        auto it = map.begin();
        while(it != map.end()){
            if (it->second == 1)
                mBoundary.push_back(it->first);

            it++;
        }

    }

    std::vector <Edge <T>> mBoundary;
};

#endif // BOUNDARYDETECTOR_H
