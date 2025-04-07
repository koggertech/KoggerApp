#pragma once

#include "point_3d.h"
#include <math.h>

//! CIRCLE CLASS
template <typename T>
class Circle
{
public:

    Circle() = default;

    Circle(const T& radius, const Point3D <T>& center)
        : mCenter(center)
        , mRadius(radius)
    {}

    //! Returns radius of circle
    T radius() const { return mRadius; };
    //! Returns center point of the triangle
    Point3D <T> center() const { return mCenter; };
    //! Check for circle contains point
    bool contains(const Point3D <T>& point) {

        const T dx = mCenter.x() - point.x();
        const T dy = mCenter.y() - point.y();
        const T dz = mCenter.y() - point.z();
        const T distance = sqrt(pow(dx,2.0f) + pow(dy,2.0f) + pow(dz,2.0f));

        return distance <= mRadius;
    }

    bool contains(const Point2D <T>& point) {

        const T dx = mCenter.x() - point.x();
        const T dy = mCenter.y() - point.y();
        const T distance = sqrt(pow(dx,2.0f) + pow(dy,2.0f));

        return distance <= mRadius;
    }

private:

    //! Center of circle
    Point3D <T> mCenter;
    //! Radius of circle
    T mRadius;
};
