#pragma once

#include <iostream>
#include <qhash.h>

//! POINT CLASS
template <typename T>
class Point3D
{
public:

    //! Constructor 1
    Point3D() = default;
    //! Constructor 2
    Point3D(const T& x, const T& y, const T& z)
        : mX(x)
        , mY(y)
        , mZ(z) {

    };

    //! Returns x - value
    T x() const { return mX; };
    //! Returns y - value
    T y() const { return mY; };

    T z() const { return mZ; };
    //! Set x - value
    void setX(T value) { mX = value; };
    //! Set y - value
    void setY(T value) { mY = value; };

    void setZ(T value) { mZ = value; };

    //! Returns distance between points
    T distance(const Point3D& point) const {
        return (mX - point.x()) ^ 2 + (mY - point.y()) ^ 2;
    }

    //! Equal operator
    bool operator==(const Point3D& other) const {
        return mX == other.x() && mY == other.y() && mZ == other.z();
    };

    //! Equal operator
    bool operator<(const Point3D& other) const {
        return mX < other.x() && mY < other.y() && mZ < other.z();
    };


    //! Not-equal operator
    bool operator!=(const Point3D& other) const {
        return mX != other.x() && mY != other.y();
    };

    //! Ostream operator
    friend std::ostream& operator<<(std::ostream& out, const Point3D <T>& point) {
        out << "X: " << point.x() << ", Y: " << point.y() << ", Z: " << point.z() << std::endl;
        return out;
    }

protected:
    //! X - value
    T mX;
    //! Y - value
    T mY;
    //! Z - value
    T mZ;
};

template <>
struct std::hash <Point3D <double>>
{
    std::size_t operator()(Point3D <double> const& p) const noexcept
    {
        std::size_t h1 = std::hash<double>{}(p.x());
        std::size_t h2 = std::hash<double>{}(p.y());
        std::size_t h3 = std::hash<double>{}(p.z());
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
