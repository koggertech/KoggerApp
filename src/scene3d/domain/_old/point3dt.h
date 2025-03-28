#ifndef POINT3DT_H
#define POINT3DT_H

#include "point_2d.h"

namespace Point
{
    template <typename T>
    class Point3D : public Point2D <T>
    {
    public:

        Point3D() = default;

        Point3D(const T& x, const T& y, const T& z)
        :Point2D <T>(x,y)
        , mZ(z)
        {};

        T z() const { return mZ; };

        void setZ(T value) { mZ = value; };

        bool operator==(const Point3D <T>& other) const {
            return mX == other.x() && mY == other.y() && mZ == other.z();
        };


    private:

        T mZ;

    };
}

template <typename T>
struct std::hash <Point::Point3D <T>>
{
    std::size_t operator()(Point::Point3D <T> const& p) const noexcept
    {
        std::size_t h1 = std::hash<T>{}(p.x());
        std::size_t h2 = std::hash<T>{}(p.y());
        std::size_t h3 = std::hash<T>{}(p.z());
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

#endif // POINT3DT_H
