#ifndef BARYCENTRICINTERPOLATOR_H
#define BARYCENTRICINTERPOLATOR_H

#include "triangle.h"

template <typename T>
class BarycentricInterpolator
{
public:

    /**
    * @brief Выполняет интерполяцию z - координаты у каждой точки из входного набора.
    * @param triangles Набор треугольников, относительно которых происходит интерполяция.
    * @param data Интерполируемые данные.
    */
    void process(std::vector <Triangle <T>>& triangles,
                 std::vector <Point3D <T>>& data)
    {
        for (auto& p : data){
            auto t = triangles.begin();
            while (t != triangles.end() && !t->contains(p)){
                t++;
            }

            if (t != triangles.end())
                calculateZ(*t, p);
        }
    }

    void process(std::vector <Triangle <T>>& triangles,
                 Point3D <T>& point)
    {
        auto t = triangles.begin();
        while (t != triangles.end() && !t->contains(point)){
            t++;
        }

        if (t != triangles.end())
            calculateZ(*t, point);
    }

private:

    /**
     * @brief Выполняет интерполяцию z - координаты.
     * @param t Треугольник, относительно которого происходит интерполяция.
     * @param p Интерполируемая точка.
     */
    void calculateZ(Triangle <T>& t, Point3D <T>& p)
    {
        auto& c = p;
        T w_1 = ((t.B().y() - t.C().y()) * (c.x() - t.C().x()) + (t.C().x() - t.B().x()) * (c.y() - t.C().y())) /
                     ((t.B().y() - t.C().y()) * (t.A().x() - t.C().x()) + (t.C().x() - t.B().x())*(t.A().y() - t.C().y()));

        T w_2 = ((t.C().y() - t.A().y()) * (c.x() - t.C().x()) + (t.A().x() - t.C().x()) * (c.y() - t.C().y())) /
                     ((t.B().y() - t.C().y()) * (t.A().x() - t.C().x()) + (t.C().x() - t.B().x())*(t.A().y() - t.C().y()));

        T w_3 = static_cast <T> (1.0f) - w_1 - w_2;

        T z = w_1 * t.A().z() + w_2 * t.B().z() + w_3 * t.C().z();

        p.setZ(z);
    }
};

#endif // BARYCENTRICINTERPOLATOR_H
