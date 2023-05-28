#pragma once

#include <iostream>
#include <point2d.h>

#ifdef QT_CORE_LIB
#include <QVector3D>
#endif

template <typename T>
class Point3D
{
public:

    //! Constructor 1
    Point3D() = default;
    //! Constructor 2
    Point3D(const T& x, const T& y, const T& z, size_t index = 0)
    :mX(x)
    ,mY(y)
    ,mZ(z)
    ,mIndex(index)
    {};

    //! Returns x - value
    T x() const { return mX; };
    //! Returns y - value
    T y() const { return mY; };

    T z() const { return mZ; };

    size_t index() const {return mIndex;};

    //! Set x - value
    void setX(T value) { mX = value; };
    //! Set y - value
    void setY(T value) { mY = value; };

    void setZ(T value) { mZ = value; };

    //! Returns distance between points
    T distance(const Point3D& point) const {
        return (mX - point.x()) ^ 2 + (mY - point.y()) ^ 2;
    }

    Point2D <T> toPoint2D() const{
        return Point2D <T>(x(), y(), mIndex);
    }
    //! Equal operator
    bool operator==(const Point3D& other) const {
        return mX == other.x() && mY == other.y() /*&& mZ == other.z()*/;
        //return equal(mX,other.x()) && equal(mY,other.y()) && equal(mZ,other.z());
    };

    //! Equal operator
    bool operator<(const Point3D& other) const {
        return mX < other.x() && mY < other.y() /*&& mZ < other.z()*/;
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

#ifdef QT_CORE_LIB
    QVector3D toQVector3D() const{
        return {
            static_cast <float>(mX),
            static_cast <float>(mY),
            static_cast <float>(mZ),
        };
    };
#endif

protected:
    //! X - value
    T mX;
    //! Y - value
    T mY;
    //! Z - value
    T mZ;

    size_t mIndex = 0; ///< Point index in container.
};

#ifdef QT_CORE_LIB
template <typename T>
uint qHash(const Point3D <T> &p, uint seed = 4)
{
    Q_UNUSED(seed)

    auto h1 = qHash(p.x());
    auto h2 = qHash(p.y());
    auto h3 = qHash(p.z());
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}
#endif

template <typename T>
struct std::hash <Point3D <T>>
{
    std::size_t operator()(const Point3D <T>& p) const noexcept
    {
        auto h1 = std::hash<T>{}(p.x());
        auto h2 = std::hash<T>{}(p.y());
        auto h3 = std::hash<T>{}(p.z());
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

