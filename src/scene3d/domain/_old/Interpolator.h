#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

#include <memory>
#include <optional>


#include "quad.h"
#include "triangle.h"

static const uint8_t GRID_SIDE_CELLS = 100;

template <typename T>
class Interpolator
{
    using TrianglesPointer = std::shared_ptr <std::vector <Triangle <T>>>;
    using QuadsPointer     = std::shared_ptr <std::vector <Quad <T>>>;

public:

    Interpolator()
    : mInterpLevel(1)
    {}

    void setInterpolationLevel(int level)
    {
        mInterpLevel = level;

        if (mInterpLevel <= 0)
            mInterpLevel = 1;
    }

    QuadsPointer interpolate(std::vector <Triangle <T>>* pTriangles)
    {
        const int cells_count = GRID_SIDE_CELLS * mInterpLevel ;

        auto quadPoints = new Point3D <T>*[cells_count];

        for (size_t i = 0; i < cells_count; i++){
            quadPoints[i] = new Point3D <T>[cells_count];
        }

        auto pQuads  = std::make_shared <std::vector <Quad <T>>> ();
        auto pResult = std::make_shared <std::vector <Quad <T>>> ();

        // 1) Находим границы поверхности
        T max_x = pTriangles->front().A().x();
        T min_x = max_x;
        T max_y = pTriangles->front().A().y();
        T min_y = max_y;
        T max_z = pTriangles->front().A().z();
        T min_z = max_z;

        for (const auto& t : *pTriangles){

            max_x = std::max(max_x, t.A().x());
            max_x = std::max(max_x, t.B().x());
            max_x = std::max(max_x, t.C().x());

            min_x = std::min(min_x, t.A().x());
            min_x = std::min(min_x, t.B().x());
            min_x = std::min(min_x, t.C().x());

            max_y = std::max(max_y, t.A().y());
            max_y = std::max(max_y, t.B().y());
            max_y = std::max(max_y, t.C().y());

            min_y = std::min(min_y, t.A().y());
            min_y = std::min(min_y, t.B().y());
            min_y = std::min(min_y, t.C().y());

            max_z = std::max(max_z, t.A().z());
            max_z = std::max(max_z, t.B().z());
            max_z = std::max(max_z, t.C().z());

            min_z = std::min(min_z, t.A().z());
            min_z = std::min(min_z, t.B().z());
            min_z = std::min(min_z, t.C().z());
        }

        mMaximumX = max_x;
        mMaximumY = max_y;
        mMaximumZ = max_z;

        mMinimumX = min_x;
        mMinimumY = min_y;
        mMinimumZ = min_z;

        // 2) Формируем прямоугольную сетку
        T w = max_x - min_x;
        T h = max_y - min_y;

        T side = std::max(w,h);
        T step = side / cells_count;

        // Этап 1 - интерполируем координату Z
        for (int row = 0; row < cells_count; row++){
            for (int col = 0; col < cells_count; col++)
            {
                Point3D <T> p;
                p.setX(min_x + col * step);
                p.setY(min_y + row * step);
                p.setZ((max_z + min_z) / 2.0f);

                auto t = pTriangles->begin();
                while (t != pTriangles->end() && !t->contains(p)){
                    t++;
                }

                if (t != pTriangles->end())
                    calculateZ(*t, p);

                quadPoints[row][col] = p;
            }
        }

        // Этап 2 - Формируем квадраты
        for (int row = 0; row < cells_count - 1; row++){
            for (int col = 0; col < cells_count - 1; col++)
            {
                Point3D <T> A;
                Point3D <T> B;
                Point3D <T> C;
                Point3D <T> D;

                A = quadPoints[row][col];
                B = quadPoints[row + 1][col];
                C = quadPoints[row + 1][col + 1];
                D = quadPoints[row][col + 1];

                // Отсеиваем квадраты, которые выходят за пределы поверхности
                if (A.y() <= max_y &&
                    A.y() >= min_y &&
                    A.x() <= max_x &&
                    A.x() >= min_x){
                    Quad <T> quad(A,B,C,D);
                    pQuads->push_back(quad);
                }
            }
        }

        // Этап 3 - отсекаем квадраты, которые не содержит поверхность.
        // В результирующий массив квадратов попадают только те квадраты,
        // 4 вершины которых полностью лежат в одном из треугольников поверхности.
        for (const auto& quad : *pQuads){
            bool surfaceContainsA = false;
            bool surfaceContainsB = false;
            bool surfaceContainsC = false;
            bool surfaceContainsD = false;

            auto t = pTriangles->begin();

            while (t != pTriangles->end() && (!surfaceContainsA || !surfaceContainsB || !surfaceContainsC || !surfaceContainsD))
            {
                if (!surfaceContainsA) surfaceContainsA = t->contains(quad.A());
                if (!surfaceContainsB) surfaceContainsB = t->contains(quad.B());
                if (!surfaceContainsC) surfaceContainsC = t->contains(quad.C());
                if (!surfaceContainsD) surfaceContainsD = t->contains(quad.D());
                t++;
            }

            if (t != pTriangles->end())
                pResult->push_back(quad);
        }

        if (!quadPoints){
            for (int i = 0; i < cells_count; i++){
                if (!quadPoints[i])
                    delete[] quadPoints[i];
            }
            delete[] quadPoints;
        }

        return pResult;
    }

    T maximumX() const {return mMaximumX;};
    T maximumY() const {return mMaximumY;};
    T maximumZ() const {return mMaximumZ;};
    T minimumX() const {return mMinimumX;};
    T minimumY() const {return mMinimumY;};
    T minimumZ() const {return mMinimumZ;};

private:

    void calculateZ(Triangle <T>& t, Point3D <T>& p)
    {
        auto& c = p;
        T w_1 = ((t.A().y() - t.C().y()) * (c.x() - t.C().x()) + (t.C().x() - t.A().x()) * (c.y() - t.C().y())) /
                     ((t.A().y() - t.C().y()) * (t.B().x() - t.C().x()) + (t.C().x() - t.A().x())*(t.B().y() - t.C().y()));

        T w_2 = ((t.C().y() - t.B().y()) * (c.x() - t.C().x()) + (t.B().x() - t.C().x()) * (c.y() - t.C().y())) /
                     ((t.A().y() - t.C().y()) * (t.B().x() - t.C().x()) + (t.C().x() - t.A().x())*(t.B().y() - t.C().y()));

        T w_3 = 1 - w_1 - w_2;

        T z = w_1 * t.B().z() + w_2 * t.C().z() + w_3 * t.A().z();

        p.setZ(z);
    }

    size_t squareIntersectedByTriangle(Quad <T>& q, Triangle <T>& t) const
    {
        size_t containsCount = 0;

        if (t.contains(q.A())) containsCount++;
        if (t.contains(q.B())) containsCount++;
        if (t.contains(q.C())) containsCount++;
        if (t.contains(q.D())) containsCount++;

        return  containsCount;
    }

    T mMaximumX;
    T mMaximumY;
    T mMaximumZ;

    T mMinimumX;
    T mMinimumY;
    T mMinimumZ;

    int mInterpLevel = 1;
};

#endif // INTERPOLATOR_H
